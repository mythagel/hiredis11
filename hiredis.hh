#ifndef REDISPP_H_
#define REDISPP_H_
#include <hiredis/hiredis.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

namespace hiredis
{

struct error : std::runtime_error
{
	error(const std::string& what)
	 : std::runtime_error(what)
	{
	}
};

typedef std::shared_ptr<redisReply> reply_t;

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
	auto command(const std::vector<std::string>& args) -> reply_t
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
};

namespace reply
{

struct string
{
	std::string value;
	
	string(const reply_t& reply)
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
	
	integer(const reply_t& reply)
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
	
	status(const reply_t& reply)
	{
		if(reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR)
			value = {reply->str, static_cast<size_t>(reply->len)};
		else
			throw std::invalid_argument("reply type not status/error.");
	}
	
	operator std::string() const
	{
		return value;
	}
};

// TODO how to cleanly represent REDIS_REPLY_ARRAY?
bool is_nill(const reply_t& reply)
{
	return reply->type == REDIS_REPLY_NIL;
}

}

namespace key
{

//DEL key [key ...]
//Delete a key
template<typename... Keys>
auto del(context& c, Keys... keys) -> long long
{
	return reply::integer{c.command({"DEL", keys...})};
}

//DUMP key
//Return a serialized version of the value stored at the specified key.
template<typename Key>
auto dump(context& c, Key key) -> std::string
{
	return reply::string{c.command({"DUMP", key})};
}

//EXISTS key
//Determine if a key exists
template<typename Key>
auto exists(context& c, Key key) -> bool
{
	return reply::integer{c.command({"EXISTS", key})};
}

//EXPIRE key seconds
//Set a key's time to live in seconds
template<typename Key>
auto expire(context& c, Key key, int ttl) -> bool
{
	return reply::integer{c.command({"EXPIRE", key, std::to_string(ttl)})};
}

//EXPIREAT key timestamp
//Set the expiration for a key as a UNIX timestamp

//KEYS pattern
//Find all keys matching the given pattern

//MIGRATE host port key destination-db timeout [COPY] [REPLACE]
//Atomically transfer a key from a Redis instance to another one.

//MOVE key db
//Move a key to another database

//OBJECT subcommand [arguments [arguments ...]]
//Inspect the internals of Redis objects

//PERSIST key
//Remove the expiration from a key

//PEXPIRE key milliseconds
//Set a key's time to live in milliseconds

//PEXPIREAT key milliseconds-timestamp
//Set the expiration for a key as a UNIX timestamp specified in milliseconds

//PTTL key
//Get the time to live for a key in milliseconds

//RANDOMKEY
//Return a random key from the keyspace

//RENAME key newkey
//Rename a key

//RENAMENX key newkey
//Rename a key, only if the new key does not exist

//RESTORE key ttl serialized-value
//Create a key using the provided serialized value, previously obtained using DUMP.

//SORT key [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE destination]
//Sort the elements in a list, set or sorted set

//TTL key
//Get the time to live for a key

//TYPE key
//Determine the type stored at key
}

namespace string
{
//APPEND key value
//Append a value to a key

//BITCOUNT key [start] [end]
//Count set bits in a string

//BITOP operation destkey key [key ...]
//Perform bitwise operations between strings

//DECR key
//Decrement the integer value of a key by one

//DECRBY key decrement
//Decrement the integer value of a key by the given number

//GET key
//Get the value of a key

//GETBIT key offset
//Returns the bit value at offset in the string value stored at key

//GETRANGE key start end
//Get a substring of the string stored at a key

//GETSET key value
//Set the string value of a key and return its old value

//INCR key
//Increment the integer value of a key by one

//INCRBY key increment
//Increment the integer value of a key by the given amount

//INCRBYFLOAT key increment
//Increment the float value of a key by the given amount

//MGET key [key ...]
//Get the values of all the given keys

//MSET key value [key value ...]
//Set multiple keys to multiple values

//MSETNX key value [key value ...]
//Set multiple keys to multiple values, only if none of the keys exist

//PSETEX key milliseconds value
//Set the value and expiration in milliseconds of a key

//SET key value [EX seconds] [PX milliseconds] [NX|XX]
//Set the string value of a key

//SETBIT key offset value
//Sets or clears the bit at offset in the string value stored at key

//SETEX key seconds value
//Set the value and expiration of a key

//SETNX key value
//Set the value of a key, only if the key does not exist

//SETRANGE key offset value
//Overwrite part of a string at key starting at the specified offset

//STRLEN key
//Get the length of the value stored in a key
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
//SADD key member [member ...]
//Add one or more members to a set

//SCARD key
//Get the number of members in a set

//SDIFF key [key ...]
//Subtract multiple sets

//SDIFFSTORE destination key [key ...]
//Subtract multiple sets and store the resulting set in a key

//SINTER key [key ...]
//Intersect multiple sets

//SINTERSTORE destination key [key ...]
//Intersect multiple sets and store the resulting set in a key

//SISMEMBER key member
//Determine if a given value is a member of a set

//SMEMBERS key
//Get all the members in a set

//SMOVE source destination member
//Move a member from one set to another

//SPOP key
//Remove and return a random member from a set

//SRANDMEMBER key [count]
//Get one or multiple random members from a set

//SREM key member [member ...]
//Remove one or more members from a set

//SUNION key [key ...]
//Add multiple sets

//SUNIONSTORE destination key [key ...]
//Add multiple sets and store the resulting set in a key
}

namespace ordered_set
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
//PSUBSCRIBE pattern [pattern ...]
//Listen for messages published to channels matching the given patterns

//PUBSUB subcommand [argument [argument ...]]
//Inspect the state of the Pub/Sub subsystem

//PUBLISH channel message
//Post a message to a channel

//PUNSUBSCRIBE [pattern [pattern ...]]
//Stop listening for messages posted to channels matching the given patterns

//SUBSCRIBE channel [channel ...]
//Listen for messages published to the given channels

//UNSUBSCRIBE [channel [channel ...]]
//Stop listening for messages posted to the given channels
}

namespace transaction
{
//DISCARD
//Discard all commands issued after MULTI

//EXEC
//Execute all commands issued after MULTI

//MULTI
//Mark the start of a transaction block

//UNWATCH
//Forget about all watched keys

//WATCH key [key ...]
//Watch the given keys to determine execution of the MULTI/EXEC block
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
//AUTH password
//Authenticate to the server

//ECHO message
//Echo the given string

//PING
//Ping the server

//QUIT
//Close the connection

//SELECT index
//Change the selected database for the current connection
}

namespace server
{
//BGREWRITEAOF
//Asynchronously rewrite the append-only file

//BGSAVE
//Asynchronously save the dataset to disk

//CLIENT KILL ip:port
//Kill the connection of a client

//CLIENT LIST
//Get the list of client connections

//CLIENT GETNAME
//Get the current connection name

//CLIENT SETNAME connection-name
//Set the current connection name

//CONFIG GET parameter
//Get the value of a configuration parameter

//CONFIG REWRITE
//Rewrite the configuration file with the in memory configuration

//CONFIG SET parameter value
//Set a configuration parameter to the given value

//CONFIG RESETSTAT
//Reset the stats returned by INFO

//DBSIZE
//Return the number of keys in the selected database

//DEBUG OBJECT key
//Get debugging information about a key

//DEBUG SEGFAULT
//Make the server crash

//FLUSHALL
//Remove all keys from all databases

//FLUSHDB
//Remove all keys from the current database

//INFO [section]
//Get information and statistics about the server

//LASTSAVE
//Get the UNIX time stamp of the last successful save to disk

//MONITOR
//Listen for all requests received by the server in real time

//SAVE
//Synchronously save the dataset to disk

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

}

#endif /* REDISPP_H_ */
