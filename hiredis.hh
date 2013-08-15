#ifndef REDISPP_H_
#define REDISPP_H_
#include <hiredis/hiredis.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/optional.hpp>
#include <ctime>
#include <chrono>

namespace hiredis
{

struct error : std::runtime_error
{
	error(const std::string& what)
	 : std::runtime_error(what)
	{
	}
};

namespace reply
{

typedef std::shared_ptr<redisReply> reply_t;

struct string
{
	std::string value;
	
	string(reply_t reply)
	{
		if(reply->type == REDIS_REPLY_STRING)
			value = {reply->str, static_cast<size_t>(reply->len)};
		else
			throw std::invalid_argument("reply type not string.");
	}
	
	operator std::string() const
	{
		return value;
	}
};

struct integer
{
	long long value;
	
	integer(reply_t reply)
	{
		if(reply->type == REDIS_REPLY_INTEGER)
			value = reply->integer;
		else
			throw std::invalid_argument("reply type not integer.");
	}
	
	operator long long() const
	{
		return value;
	}
};

struct status
{
	std::string value;
	
	status(reply_t reply)
	{
		if(reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR)
			value = {reply->str, static_cast<size_t>(reply->len)};
		else
			throw std::invalid_argument("reply type not status/error.");
		
		if(reply->type == REDIS_REPLY_ERROR)
			throw error(value);
	}
	
	operator std::string() const
	{
		return value;
	}
};

struct array
{
	std::vector<reply_t> elements;
	
	array(reply_t reply)
	{
		if(reply->type == REDIS_REPLY_ARRAY)
		{
			elements.resize(reply->elements);
			std::transform(reply->element, reply->element + reply->elements, begin(elements), [&reply](redisReply* r) -> reply_t { return {reply, r}; });
		}
		else
			throw std::invalid_argument("reply type not array.");
	}
};

struct string_array
{
	std::vector<std::string> value;
	
	string_array(reply_t reply)
	{
		if(reply->type == REDIS_REPLY_ARRAY)
		{
			std::vector<reply_t> elements;
			elements.resize(reply->elements);
			std::transform(reply->element, reply->element + reply->elements, begin(elements), [&reply](redisReply* r) -> reply_t { return {reply, r}; });
			
			for(auto& reply : elements)
				value.push_back(string{reply});
		}
		else
			throw std::invalid_argument("reply type not array.");
	}
	
	operator std::vector<std::string>() const
	{
		return value;
	}
};

bool is_nill(reply_t reply)
{
	return reply->type == REDIS_REPLY_NIL;
}

}

class context
{
private:
	std::shared_ptr<redisContext> c;
	
	void critical_error()
	{
		if(c->err)
		{
			auto err = error(c->errstr);
			// Context is not reusable.
			c.reset();
			throw err;
		}
		throw std::logic_error("critical_error called with no active hiredis error.");
	}
public:
	struct error : std::runtime_error
	{
		error(const std::string& what)
		 : std::runtime_error(what)
		{
		}
	};

	context(const std::string& ip, int port)
	 : c(redisConnect(ip.c_str(), port), redisFree)
	{
		if(!c)
			throw error("Unable to create context");
		if(c->err)
			throw error(c->errstr);
	}
	
	context(const context&) = delete;
	context& operator=(const context&) = delete;
	
	context(context&&) = default;
	context& operator=(context&&) = default;
	
	/*
	 Send a command and get a reply.
	 e.g.
	 context c;
	 auto reply = c.command({"SET", "foo", "bar"});
	*/
	auto command(const std::vector<std::string>& args) -> reply::reply_t
	{
		auto argc = args.size();
		std::vector<const char*> argv(argc);
		std::vector<size_t> argvlen(argc);
		std::transform(begin(args), end(args), begin(argv), [](const std::string& s) -> const char* { return s.c_str(); });
		std::transform(begin(args), end(args), begin(argvlen), [](const std::string& s) -> size_t { return s.size(); });
	
		auto res = redisCommandArgv(c.get(), argc, argv.data(), argvlen.data());
		if(!res)
			critical_error();
	
		return { static_cast<redisReply*>(res), freeReplyObject };
	}
	
	void append_command(const std::vector<std::string>& args)
	{
		auto argc = args.size();
		std::vector<const char*> argv(argc);
		std::vector<size_t> argvlen(argc);
		std::transform(begin(args), end(args), begin(argv), [](const std::string& s) -> const char* { return s.c_str(); });
		std::transform(begin(args), end(args), begin(argvlen), [](const std::string& s) -> size_t { return s.size(); });
	
		redisAppendCommandArgv(c.get(), argc, argv.data(), argvlen.data());
		if(c->err)
			critical_error();
	}
	
	auto get_reply() -> reply::reply_t
	{
		void* reply;
		
		int res = redisGetReply(c.get(), &reply);
		if(res == REDIS_ERR)
			critical_error();
		
		return { static_cast<redisReply*>(reply), freeReplyObject };
	}
};

class pipeline
{
private:
	context& c;
	std::size_t commands;
public:
	pipeline(context& c)
	 : c(c), commands(0)
	{
	}
	
	auto command(const std::vector<std::string>& args) -> void
	{
		c.append_command(args);
		++commands;
	}
	auto execute() -> std::vector<reply::reply_t>
	{
		std::vector<reply::reply_t> replies(commands);
		std::generate_n(begin(replies), commands, [this]() -> reply::reply_t { return c.get_reply(); });
		commands = 0;
		return replies;
	}
	
	~pipeline()
	{
		try
		{
			execute();
		}
		catch(...)
		{
		}
	}
};

namespace key
{

// Delete a key
template<typename Key, typename... Keys>
auto del(context& c, Key key, Keys... keys) -> long long
{
	return reply::integer{c.command({"DEL", key, keys...})};
}

// Return a serialized version of the value stored at the specified key.
template<typename Key>
auto dump(context& c, Key key) -> std::string
{
	return reply::string{c.command({"DUMP", key})};
}

// Determine if a key exists
template<typename Key>
auto exists(context& c, Key key) -> bool
{
	return reply::integer{c.command({"EXISTS", key})};
}

// Set a key's time to live in seconds
template<typename Key>
auto expire(context& c, Key key, std::chrono::seconds ttl) -> bool
{
	return reply::integer{c.command({"EXPIRE", key, std::to_string(ttl.count())})};
}

// Set the expiration for a key as a UNIX timestamp
template<typename Key>
auto expire_at(context& c, Key key, std::time_t timestamp) -> bool
{
	return reply::integer{c.command({"EXPIREAT", key, std::to_string(timestamp)})};
}

// Find all keys matching the given pattern
auto keys(context& c, const std::string& pattern) -> std::vector<std::string>
{
	auto array = reply::array{c.command({"KEYS", pattern})};
	std::vector<std::string> keys;
	for(auto e : array.elements)
		keys.push_back(reply::string(e));
	
	return keys;
}

//MIGRATE host port key destination-db timeout [COPY] [REPLACE]
//Atomically transfer a key from a Redis instance to another one.

// Move a key to another database
template<typename Key>
auto move(context& c, Key key, int db) -> bool
{
	return reply::integer{c.command({"MOVE", key, std::to_string(db)})};
}

//OBJECT subcommand [arguments [arguments ...]]
//Inspect the internals of Redis objects

// Remove the expiration from a key
template<typename Key>
auto persist(context& c, Key key) -> bool
{
	return reply::integer{c.command({"PERSIST", key})};
}

// Set a key's time to live in milliseconds
template<typename Key>
auto expire(context& c, Key key, std::chrono::milliseconds ttl) -> bool
{
	return reply::integer{c.command({"PEXPIRE", key, std::to_string(ttl.count())})};
}

// Set the expiration for a key as a UNIX timestamp specified in milliseconds
template<typename Key>
auto expire_at_ms(context& c, Key key, uint64_t timestamp) -> bool
{
	return reply::integer{c.command({"PEXPIREAT", key, std::to_string(timestamp)})};
}

// Get the time to live for a key in milliseconds
template<typename Key>
auto ttl_ms(context& c, Key key) -> std::chrono::milliseconds
{
	return reply::integer{c.command({"PTTL", key})};
}

// Return a random key from the keyspace
auto random(context& c) -> std::string
{
	return reply::string{c.command({"RANDOMKEY"})};
}

// Rename a key
template<typename Key>
auto rename(context& c, Key key, Key newkey) -> std::string
{
	return reply::status{c.command({"RENAME", key, newkey})};
}

// Rename a key, only if the new key does not exist
template<typename Key>
auto renamenx(context& c, Key key, Key newkey) -> bool
{
	return reply::integer{c.command({"RENAMENX", key, newkey})};
}

// Create a key using the provided serialized value, previously obtained using DUMP.
template<typename Key>
auto restore(context& c, Key key, int ttl, const std::string& dump) -> std::string
{
	return reply::status{c.command({"RESTORE", key, std::to_string(ttl), dump})};
}

//SORT key [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE destination]
//Sort the elements in a list, set or sorted set

// Get the time to live for a key
template<typename Key>
auto ttl(context& c, Key key) -> std::chrono::seconds
{
	return reply::integer{c.command({"TTL", key})};
}

// Determine the type stored at key
template<typename Key>
auto type(context& c, Key key) -> std::string
{
	return reply::status{c.command({"TYPE", key})};
}

}

namespace string
{

// Append a value to a key
template<typename Key, typename Value>
auto append(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"APPEND", key, value})};
}

//BITCOUNT key [start] [end]
//Count set bits in a string

//BITOP operation destkey key [key ...]
//Perform bitwise operations between strings

// Decrement the integer value of a key by one
template<typename Key>
auto decr(context& c, Key key) -> long long
{
	return reply::integer{c.command({"DECR", key})};
}

// Decrement the integer value of a key by the given number
template<typename Key>
auto decr_by(context& c, Key key, long long decrement) -> long long
{
	return reply::integer{c.command({"DECRBY", key, std::to_string(decrement)})};
}

// Get the value of a key
template<typename Key>
auto get(context& c, Key key) -> boost::optional<std::string>
{
	auto value = c.command({"GET", key});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

//GETBIT key offset
//Returns the bit value at offset in the string value stored at key

// Get a substring of the string stored at a key
template<typename Key>
auto get_range(context& c, Key key, long long start, long long end) -> boost::optional<std::string>
{
	auto value = c.command({"GETRANGE", key, start, end});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

// Set the string value of a key and return its old value
template<typename Key, typename Value>
auto get_set(context& c, Key key, Value value) -> boost::optional<std::string>
{
	auto old_value = c.command({"GETSET", key, value});
	if(reply::is_nill(old_value))
		return {};
	return {true, reply::string{old_value}};
}

// Increment the integer value of a key by one
template<typename Key>
auto incr(context& c, Key key) -> long long
{
	return reply::integer{c.command({"INCR", key})};
}

// Increment the integer value of a key by the given amount
template<typename Key>
auto incr_by(context& c, Key key, long long increment) -> long long
{
	return reply::integer{c.command({"INCRBY", key, std::to_string(increment)})};
}

// Increment the float value of a key by the given amount
template<typename Key>
auto incr_by(context& c, Key key, double increment) -> long long
{
	return reply::integer{c.command({"INCRBYFLOAT", key, std::to_string(increment)})};
}

//MGET key [key ...]
//Get the values of all the given keys

//MSET key value [key value ...]
//Set multiple keys to multiple values

//MSETNX key value [key value ...]
//Set multiple keys to multiple values, only if none of the keys exist

//PSETEX key milliseconds value
//Set the value and expiration in milliseconds of a key

// Set the string value of a key
template<typename Key, typename Value>
auto set(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value})};
}
template<typename Key, typename Value>
auto set(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count())})};
}
template<typename Key, typename Value>
auto set(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count())})};
}

// Set the value of a key, only if the key already exists
template<typename Key, typename Value>
auto setxx(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value, "XX"})};
}
template<typename Key, typename Value>
auto setxx(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count()), "XX"})};
}
template<typename Key, typename Value>
auto setxx(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count()), "XX"})};
}

//SETBIT key offset value
//Sets or clears the bit at offset in the string value stored at key

// Set the value of a key, only if the key does not exist
template<typename Key, typename Value>
auto setnx(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value, "NX"})};
}
template<typename Key, typename Value>
auto setnx(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count()), "NX"})};
}
template<typename Key, typename Value>
auto setnx(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count()), "NX"})};
}

//SETRANGE key offset value
//Overwrite part of a string at key starting at the specified offset

// Get the length of the value stored in a key
template<typename Key>
auto strlen(context& c, Key key) -> long long
{
	return reply::integer{c.command({"STRLEN", key})};
}

}

namespace hash
{
//HDEL key field [field ...]
//Delete one or more hash fields

//HEXISTS key field
//Determine if a hash field exists

//HGET key field
//Get the value of a hash field

//HGETALL key
//Get all the fields and values in a hash

//HINCRBY key field increment
//Increment the integer value of a hash field by the given number

//HINCRBYFLOAT key field increment
//Increment the float value of a hash field by the given amount

//HKEYS key
//Get all the fields in a hash

//HLEN key
//Get the number of fields in a hash

//HMGET key field [field ...]
//Get the values of all the given hash fields

//HMSET key field value [field value ...]
//Set multiple hash fields to multiple values

//HSET key field value
//Set the string value of a hash field

//HSETNX key field value
//Set the value of a hash field, only if the field does not exist

//HVALS key
//Get all the values in a hash
}

namespace list
{
//BLPOP key [key ...] timeout
//Remove and get the first element in a list, or block until one is available

//BRPOP key [key ...] timeout
//Remove and get the last element in a list, or block until one is available

//BRPOPLPUSH source destination timeout
//Pop a value from a list, push it to another list and return it; or block until one is available

//LINDEX key index
//Get an element from a list by its index

//LINSERT key BEFORE|AFTER pivot value
//Insert an element before or after another element in a list

//LLEN key
//Get the length of a list

//LPOP key
//Remove and get the first element in a list

//LPUSH key value [value ...]
//Prepend one or multiple values to a list

//LPUSHX key value
//Prepend a value to a list, only if the list exists

//LRANGE key start stop
//Get a range of elements from a list

//LREM key count value
//Remove elements from a list

//LSET key index value
//Set the value of an element in a list by its index

//LTRIM key start stop
//Trim a list to the specified range

//RPOP key
//Remove and get the last element in a list

//RPOPLPUSH source destination
//Remove the last element in a list, append it to another list and return it

//RPUSH key value [value ...]
//Append one or multiple values to a list

//RPUSHX key value
//Append a value to a list, only if the list exists
}

namespace set
{

// Add one or more members to a set
template<typename Key, typename Member, typename... Members>
auto add(context& c, Key key, Member member, Members... members) -> long long
{
	return reply::integer{c.command({"SADD", key, member, members...})};
}

// Get the number of members in a set
template<typename Key>
auto card(context& c, Key key) -> long long
{
	return reply::integer{c.command({"SCARD", key})};
}

//SDIFF key [key ...]
//Subtract multiple sets

//SDIFFSTORE destination key [key ...]
//Subtract multiple sets and store the resulting set in a key

//SINTER key [key ...]
//Intersect multiple sets

//SINTERSTORE destination key [key ...]
//Intersect multiple sets and store the resulting set in a key

// Determine if a given value is a member of a set
template<typename Key, typename Member>
auto is_member(context& c, Key key, Member member) -> bool
{
	return reply::integer{c.command({"SISMEMBER", key, member})};
}

//SMEMBERS key
//Get all the members in a set

//SMOVE source destination member
//Move a member from one set to another

// Remove and return a random member from a set
template<typename Key>
auto pop(context& c, Key key) -> std::string
{
	return reply::string{c.command({"SPOP", key})};
}

//SRANDMEMBER key [count]
//Get one or multiple random members from a set

// Remove one or more members from a set
template<typename Key, typename Member, typename... Members>
auto rem(context& c, Key key, Member member, Members... members) -> long long
{
	return reply::integer{c.command({"SREM", key, member, members...})};
}

//SUNION key [key ...]
//Add multiple sets

//SUNIONSTORE destination key [key ...]
//Add multiple sets and store the resulting set in a key
}

namespace sorted_set
{
//ZADD key score member [score member ...]
//Add one or more members to a sorted set, or update its score if it already exists

//ZCARD key
//Get the number of members in a sorted set

//ZCOUNT key min max
//Count the members in a sorted set with scores within the given values

//ZINCRBY key increment member
//Increment the score of a member in a sorted set

//ZINTERSTORE destination numkeys key [key ...] [WEIGHTS weight [weight ...]] [AGGREGATE SUM|MIN|MAX]
//Intersect multiple sorted sets and store the resulting sorted set in a new key

//ZRANGE key start stop [WITHSCORES]
//Return a range of members in a sorted set, by index

//ZRANGEBYSCORE key min max [WITHSCORES] [LIMIT offset count]
//Return a range of members in a sorted set, by score

//ZRANK key member
//Determine the index of a member in a sorted set

//ZREM key member [member ...]
//Remove one or more members from a sorted set

//ZREMRANGEBYRANK key start stop
//Remove all members in a sorted set within the given indexes

//ZREMRANGEBYSCORE key min max
//Remove all members in a sorted set within the given scores

//ZREVRANGE key start stop [WITHSCORES]
//Return a range of members in a sorted set, by index, with scores ordered from high to low

//ZREVRANGEBYSCORE key max min [WITHSCORES] [LIMIT offset count]
//Return a range of members in a sorted set, by score, with scores ordered from high to low

//ZREVRANK key member
//Determine the index of a member in a sorted set, with scores ordered from high to low

//ZSCORE key member
//Get the score associated with the given member in a sorted set

//ZUNIONSTORE destination numkeys key [key ...] [WEIGHTS weight [weight ...]] [AGGREGATE SUM|MIN|MAX]
//Add multiple sorted sets and store the resulting sorted set in a new key
}

namespace pubsub
{

// Listen for messages published to channels matching the given patterns
template<typename Pattern, typename... Patterns>
void psubscribe(context& c, Pattern pattern, Patterns... patterns)
{
	c.command({"PSUBSCRIBE", pattern, patterns...});
}

//PUBSUB subcommand [argument [argument ...]]
//Inspect the state of the Pub/Sub subsystem

// Post a message to a channel
template<typename Channel, typename Message>
auto publish(context& c, Channel channel, Message message) -> long long
{
	return reply::integer{c.command({"PUBLISH", channel, message})};
}

// Stop listening for messages posted to channels matching the given patterns
template<typename... Patterns>
void punsubscribe(context& c, Patterns... patterns)
{
	c.command({"PUNSUBSCRIBE", patterns...});
}

// Listen for messages published to the given channels
template<typename Channel, typename... Channels>
void subscribe(context& c, Channel channel, Channels... channels)
{
	c.command({"SUBSCRIBE", channel, channels...});
}

// Stop listening for messages posted to the given channels
template<typename... Channels>
void unsubscribe(context& c, Channels... channels)
{
	c.command({"UNSUBSCRIBE", channels...});
}
}

namespace transaction
{

// Discard all commands issued after MULTI
auto discard(context& c) -> std::string
{
	return reply::status{c.command({"DISCARD"})};
}

//EXEC
//Execute all commands issued after MULTI
// TODO array reply

// Mark the start of a transaction block
auto multi(context& c) -> std::string
{
	return reply::status{c.command({"MULTI"})};
}

// Forget about all watched keys
auto unwatch(context& c) -> std::string
{
	return reply::status{c.command({"UNWATCH"})};
}

// Watch the given keys to determine execution of the MULTI/EXEC block
template<typename Key, typename... Keys>
auto watch(context& c, Key key, Keys... keys) -> std::string
{
	return reply::status{c.command({"WATCH", key, keys...})};
}

}

namespace script
{
//EVAL script numkeys key [key ...] arg [arg ...]
//Execute a Lua script server side

//EVALSHA sha1 numkeys key [key ...] arg [arg ...]
//Execute a Lua script server side

//SCRIPT EXISTS script [script ...]
//Check existence of scripts in the script cache.

//SCRIPT FLUSH
//Remove all the scripts from the script cache.

//SCRIPT KILL
//Kill the script currently in execution.

//SCRIPT LOAD script
//Load the specified Lua script into the script cache.
}

namespace connection
{

// Authenticate to the server
auto auth(context& c, const std::string& password) -> std::string
{
	return reply::status{c.command({"AUTH", password})};
}

// Echo the given string
auto echo(context& c, const std::string& message) -> std::string
{
	return reply::string{c.command({"ECHO", message})};
}

// Ping the server
auto ping(context& c) -> std::string
{
	return reply::status{c.command({"PING"})};
}

// Close the connection
auto quit(context& c) -> std::string
{
	return reply::status{c.command({"QUIT"})};
}

// Change the selected database for the current connection
auto select(context& c, int index) -> std::string
{
	return reply::status{c.command({"SELECT", std::to_string(index)})};
}

}

namespace server
{

// Asynchronously rewrite the append-only file
auto bg_rewrite_aof(context& c) -> std::string
{
	return reply::status{c.command({"BGREWRITEAOF"})};
}

// Asynchronously save the dataset to disk
auto bg_save(context& c) -> std::string
{
	return reply::status{c.command({"BGSAVE"})};
}

namespace client
{

// Kill the connection of a client
auto kill(context& c, const std::string& address) -> std::string
{
	return reply::status{c.command({"CLIENT", "KILL", address})};
}

// Get the list of client connections
auto list(context& c) -> std::string
{
	return reply::string{c.command({"CLIENT", "LIST"})};
}

// Get the current connection name
auto get_name(context& c) -> boost::optional<std::string>
{
	auto value = c.command({"CLIENT", "GETNAME"});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

// Set the current connection name
auto set_name(context& c, const std::string& name) -> std::string
{
	return reply::status{c.command({"CLIENT", "SETNAME", name})};
}

}

namespace config
{
//CONFIG GET parameter
//Get the value of a configuration parameter

//CONFIG REWRITE
//Rewrite the configuration file with the in memory configuration

//CONFIG SET parameter value
//Set a configuration parameter to the given value

//CONFIG RESETSTAT
//Reset the stats returned by INFO
}

// Return the number of keys in the selected database
auto dbsize(context& c) -> long long
{
	return reply::integer{c.command({"DBSIZE"})};
}

//DEBUG OBJECT key
//Get debugging information about a key

//DEBUG SEGFAULT
//Make the server crash

// Remove all keys from all databases
auto flush_all(context& c) -> std::string
{
	return reply::status{c.command({"FLUSHALL"})};
}

// Remove all keys from the current database
auto flush_db(context& c) -> std::string
{
	return reply::status{c.command({"FLUSHDB"})};
}

// Get information and statistics about the server
auto info(context& c) -> std::string
{
	return reply::string{c.command({"INFO"})};
}
auto info(context& c, const std::string& section) -> std::string
{
	return reply::string{c.command({"INFO", section})};
}

// Get the UNIX time stamp of the last successful save to disk
auto last_save(context& c, const std::string& section) -> time_t
{
	return reply::integer{c.command({"LASTSAVE", section})};
}

//MONITOR
//Listen for all requests received by the server in real time

// Synchronously save the dataset to disk
auto save(context& c) -> std::string
{
	return reply::status{c.command({"SAVE"})};
}

//SHUTDOWN [NOSAVE] [SAVE]
//Synchronously save the dataset to disk and then shut down the server

//SLAVEOF host port
//Make the server a slave of another instance, or promote it as master

//SLOWLOG subcommand [argument]
//Manages the Redis slow queries log

//SYNC
//Internal command used for replication

//TIME
//Return the current server time
}

namespace types
{

template <typename T>
class unordered_set
{
private:
	std::string key;
	context* c;
public:
	unordered_set(const std::string& key)
	 : key(key)
	{
	}
	
	bool empty()
	{
		return set::card(c, key) == 0;
	}
	std::size_t size()
	{
		return set::card(c, key);
	}
	
};

}

}

#endif /* REDISPP_H_ */
