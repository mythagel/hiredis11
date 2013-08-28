#ifndef HIREDIS11_COMMANDS_H_
#define HIREDIS11_COMMANDS_H_
#include "context.hh"
#include "reply.hh"
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include <map>
#include <boost/optional.hpp>

namespace hiredis
{
namespace commands
{

// #    #  ######   #   #
// #   #   #         # #
// ####    #####      #
// #  #    #          #
// #   #   #          #
// #    #  ######     #
namespace key
{
// Delete a key
template<typename Key, typename... Keys>
inline auto del(context& c, Key key, Keys... keys) -> long long
{
	return reply::integer{c.command({"DEL", key, keys...})};
}

// Return a serialized version of the value stored at the specified key.
template<typename Key>
inline auto dump(context& c, Key key) -> std::string
{
	return reply::string{c.command({"DUMP", key})};
}

// Determine if a key exists
template<typename Key>
inline auto exists(context& c, Key key) -> bool
{
	return reply::integer{c.command({"EXISTS", key})};
}

// Set a key's time to live in seconds
template<typename Key>
inline auto expire(context& c, Key key, std::chrono::seconds ttl) -> bool
{
	return reply::integer{c.command({"EXPIRE", key, std::to_string(ttl.count())})};
}

// Set the expiration for a key as a UNIX timestamp
template<typename Key>
inline auto expire_at(context& c, Key key, std::time_t timestamp) -> bool
{
	return reply::integer{c.command({"EXPIREAT", key, std::to_string(timestamp)})};
}

// Find all keys matching the given pattern
inline auto keys(context& c, const std::string& pattern) -> std::vector<std::string>
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
inline auto move(context& c, Key key, int db) -> bool
{
	return reply::integer{c.command({"MOVE", key, std::to_string(db)})};
}

//OBJECT subcommand [arguments [arguments ...]]
//Inspect the internals of Redis objects

// Remove the expiration from a key
template<typename Key>
inline auto persist(context& c, Key key) -> bool
{
	return reply::integer{c.command({"PERSIST", key})};
}

// Set a key's time to live in milliseconds
template<typename Key>
inline auto expire(context& c, Key key, std::chrono::milliseconds ttl) -> bool
{
	return reply::integer{c.command({"PEXPIRE", key, std::to_string(ttl.count())})};
}

// Set the expiration for a key as a UNIX timestamp specified in milliseconds
template<typename Key>
inline auto expire_at_ms(context& c, Key key, uint64_t timestamp) -> bool
{
	return reply::integer{c.command({"PEXPIREAT", key, std::to_string(timestamp)})};
}

// Get the time to live for a key in milliseconds
template<typename Key>
inline auto ttl_ms(context& c, Key key) -> std::chrono::milliseconds
{
	return reply::integer{c.command({"PTTL", key})};
}

// Return a random key from the keyspace
inline auto random(context& c) -> std::string
{
	return reply::string{c.command({"RANDOMKEY"})};
}

// Rename a key
template<typename Key>
inline auto rename(context& c, Key key, Key newkey) -> std::string
{
	return reply::status{c.command({"RENAME", key, newkey})};
}

// Rename a key, only if the new key does not exist
template<typename Key>
inline auto renamenx(context& c, Key key, Key newkey) -> bool
{
	return reply::integer{c.command({"RENAMENX", key, newkey})};
}

// Create a key using the provided serialized value, previously obtained using DUMP.
template<typename Key>
inline auto restore(context& c, Key key, int ttl, const std::string& dump) -> std::string
{
	return reply::status{c.command({"RESTORE", key, std::to_string(ttl), dump})};
}

//SORT key [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE destination]
//Sort the elements in a list, set or sorted set

// Get the time to live for a key
template<typename Key>
inline auto ttl(context& c, Key key) -> std::chrono::seconds
{
	return reply::integer{c.command({"TTL", key})};
}

// Determine the type stored at key
template<typename Key>
inline auto type(context& c, Key key) -> std::string
{
	return reply::status{c.command({"TYPE", key})};
}
}

//  ####    #####  #####      #    #    #   ####
// #          #    #    #     #    ##   #  #    #
//  ####      #    #    #     #    # #  #  #
//      #     #    #####      #    #  # #  #  ###
// #    #     #    #   #      #    #   ##  #    #
//  ####      #    #    #     #    #    #   ####
namespace string
{
// Append a value to a key
template<typename Key, typename Value>
inline auto append(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"APPEND", key, value})};
}

//BITCOUNT key [start] [end]
//Count set bits in a string

//BITOP operation destkey key [key ...]
//Perform bitwise operations between strings

// Decrement the integer value of a key by one
template<typename Key>
inline auto decr(context& c, Key key) -> long long
{
	return reply::integer{c.command({"DECR", key})};
}

// Decrement the integer value of a key by the given number
template<typename Key>
inline auto decr_by(context& c, Key key, long long decrement) -> long long
{
	return reply::integer{c.command({"DECRBY", key, std::to_string(decrement)})};
}

// Get the value of a key
template<typename Key>
inline auto get(context& c, Key key) -> boost::optional<std::string>
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
inline auto get_range(context& c, Key key, long long start, long long end) -> boost::optional<std::string>
{
	auto value = c.command({"GETRANGE", key, start, end});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

// Set the string value of a key and return its old value
template<typename Key, typename Value>
inline auto get_set(context& c, Key key, Value value) -> boost::optional<std::string>
{
	auto old_value = c.command({"GETSET", key, value});
	if(reply::is_nill(old_value))
		return {};
	return {true, reply::string{old_value}};
}

// Increment the integer value of a key by one
template<typename Key>
inline auto incr(context& c, Key key) -> long long
{
	return reply::integer{c.command({"INCR", key})};
}

// Increment the integer value of a key by the given amount
template<typename Key>
inline auto incr_by(context& c, Key key, long long increment) -> long long
{
	return reply::integer{c.command({"INCRBY", key, std::to_string(increment)})};
}

// Increment the float value of a key by the given amount
template<typename Key>
inline auto incr_by(context& c, Key key, double increment) -> long long
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
inline auto set(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value})};
}
template<typename Key, typename Value>
inline auto set(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count())})};
}
template<typename Key, typename Value>
inline auto set(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count())})};
}

// Set the value of a key, only if the key already exists
template<typename Key, typename Value>
inline auto setxx(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value, "XX"})};
}
template<typename Key, typename Value>
inline auto setxx(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count()), "XX"})};
}
template<typename Key, typename Value>
inline auto setxx(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count()), "XX"})};
}

//SETBIT key offset value
//Sets or clears the bit at offset in the string value stored at key

// Set the value of a key, only if the key does not exist
template<typename Key, typename Value>
inline auto setnx(context& c, Key key, Value value) -> std::string
{
	return reply::status{c.command({"SET", key, value, "NX"})};
}
template<typename Key, typename Value>
inline auto setnx(context& c, Key key, Value value, std::chrono::seconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "EX", std::to_string(ttl.count()), "NX"})};
}
template<typename Key, typename Value>
inline auto setnx(context& c, Key key, Value value, std::chrono::milliseconds ttl) -> std::string
{
	return reply::status{c.command({"SET", key, value, "PX", std::to_string(ttl.count()), "NX"})};
}

//SETRANGE key offset value
//Overwrite part of a string at key starting at the specified offset

// Get the length of the value stored in a key
template<typename Key>
inline auto strlen(context& c, Key key) -> long long
{
	return reply::integer{c.command({"STRLEN", key})};
}
}

// #    #    ##     ####   #    #
// #    #   #  #   #       #    #
// ######  #    #   ####   ######
// #    #  ######       #  #    #
// #    #  #    #  #    #  #    #
// #    #  #    #   ####   #    #
namespace hash
{
// Delete one or more hash fields
template<typename Key, typename Field, typename... Fields>
inline auto del(context& c, Key key, Field field, Fields... fields) -> long long
{
	return reply::integer{c.command({"HDEL", key, field, fields...})};
}

// Determine if a hash field exists
template<typename Key, typename Field>
inline auto exists(context& c, Key key, Field field) -> bool
{
	return reply::integer{c.command({"HEXISTS", key, field})};
}

// Get the value of a hash field
template<typename Key, typename Field>
inline auto get(context& c, Key key, Field field) -> boost::optional<std::string>
{
	auto value = c.command({"HGET", key, field});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

// Get all the fields and values in a hash
template<typename Key>
inline auto get(context& c, Key key) -> std::map<std::string, std::string>
{
	auto value = c.command({"HGETALL", key});
	std::vector<std::string> data = reply::string_array{value};
	if(!data.size() % 2)
		throw error("HGETALL result not multiple of 2");
	std::map<std::string, std::string> res;
	for(auto it = begin(data); it != end(data); it += 2)
		res.insert(std::make_pair(*it, *(it+1)));
	return res;
}

// Increment the integer value of a hash field by the given number
template<typename Key, typename Field>
inline auto incr_by(context& c, Key key, Field field, long long increment) -> long long
{
	return reply::integer{c.command({"HINCRBY", key, field, std::to_string(increment)})};
}

// Increment the float value of a hash field by the given amount
template<typename Key, typename Field>
inline auto incr_by(context& c, Key key, Field field, double increment) -> long long
{
	return reply::integer{c.command({"HINCRBYFLOAT", key, field, std::to_string(increment)})};
}

// Get all the fields in a hash
template<typename Key>
inline auto keys(context& c, Key key) -> std::vector<std::string>
{
	return reply::string_array{c.command({"HKEYS", key})};
}

// Get the number of fields in a hash
template<typename Key>
inline auto len(context& c, Key key) -> long long
{
	return reply::integer{c.command({"HLEN", key})};
}

// Get the values of all the given hash fields
template<typename Key, typename Field, typename... Fields>
inline auto get(context& c, Key key, Field field, Fields... fields) -> std::map<std::string, std::string>
{
	auto value = c.command({"HMGET", key, field, fields...});
	std::vector<std::string> keys{field, fields...};
	
	std::vector<reply::reply_t> data = reply::array{value};
	if(data.size() != keys.size())
		throw error("HMGET result not equal to key count");
	
	std::map<std::string, std::string> res;
	for(std::size_t i = 0; i < keys.size(); ++i)
	{
		if(!reply::is_nill(data[i]))
			res.insert(std::make_pair(keys[i], reply::string{data[i]}));
	}
	return res;
}

// Set multiple hash fields to multiple values
template<typename Key, typename Field, typename Value>
inline auto set(context& c, Key key, const std::map<Field, Value> h) -> std::string
{
	std::vector<std::string> args{"HMSET", key};
	for(auto& v : h)
		args.insert(args.end(), {v.first, v.second});
	return reply::status{c.command(args)};
}

// Set the string value of a hash field
template<typename Key, typename Field, typename Value>
inline auto set(context& c, Key key, Field field, Value value) -> bool
{
	return reply::integer{c.command({"HSET", key, field, value})};
}

// Set the value of a hash field, only if the field does not exist
template<typename Key, typename Field, typename Value>
inline auto setnx(context& c, Key key, Field field, Value value) -> bool
{
	return reply::integer{c.command({"HSETNX", key, field, value})};
}

//Get all the values in a hash
template<typename Key>
inline auto values(context& c, Key key) -> std::vector<std::string>
{
	return reply::string_array{c.command({"HVALS", key})};
}
}

// #          #     ####    #####
// #          #    #          #
// #          #     ####      #
// #          #         #     #
// #          #    #    #     #
// ######     #     ####      #
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

//  ####   ######   #####
// #       #          #
//  ####   #####      #
//      #  #          #
// #    #  #          #
//  ####   ######     #
namespace set
{
// Add one or more members to a set
template<typename Key, typename Member, typename... Members>
inline auto add(context& c, Key key, Member member, Members... members) -> long long
{
	return reply::integer{c.command({"SADD", key, member, members...})};
}

// Get the number of members in a set
template<typename Key>
inline auto card(context& c, Key key) -> long long
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
inline auto is_member(context& c, Key key, Member member) -> bool
{
	return reply::integer{c.command({"SISMEMBER", key, member})};
}

//SMEMBERS key
//Get all the members in a set

//SMOVE source destination member
//Move a member from one set to another

// Remove and return a random member from a set
template<typename Key>
inline auto pop(context& c, Key key) -> std::string
{
	return reply::string{c.command({"SPOP", key})};
}

//SRANDMEMBER key [count]
//Get one or multiple random members from a set

// Remove one or more members from a set
template<typename Key, typename Member, typename... Members>
inline auto rem(context& c, Key key, Member member, Members... members) -> long long
{
	return reply::integer{c.command({"SREM", key, member, members...})};
}

//SUNION key [key ...]
//Add multiple sets

//SUNIONSTORE destination key [key ...]
//Add multiple sets and store the resulting set in a key
}

//  ####    ####   #####    #####  ######  #####            ####   ######   #####
// #       #    #  #    #     #    #       #    #          #       #          #
//  ####   #    #  #    #     #    #####   #    #           ####   #####      #
//      #  #    #  #####      #    #       #    #               #  #          #
// #    #  #    #  #   #      #    #       #    #          #    #  #          #
//  ####    ####   #    #     #    ######  #####            ####   ######     #
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

// #####   #    #  #####    ####   #    #  #####
// #    #  #    #  #    #  #       #    #  #    #
// #    #  #    #  #####    ####   #    #  #####
// #####   #    #  #    #       #  #    #  #    #
// #       #    #  #    #  #    #  #    #  #    #
// #        ####   #####    ####    ####   #####
namespace pubsub
{
// Listen for messages published to channels matching the given patterns
template<typename Pattern, typename... Patterns>
inline void psubscribe(context& c, Pattern pattern, Patterns... patterns)
{
	c.command({"PSUBSCRIBE", pattern, patterns...});
}

//PUBSUB subcommand [argument [argument ...]]
//Inspect the state of the Pub/Sub subsystem

// Post a message to a channel
template<typename Channel, typename Message>
inline auto publish(context& c, Channel channel, Message message) -> long long
{
	return reply::integer{c.command({"PUBLISH", channel, message})};
}

// Stop listening for messages posted to channels matching the given patterns
template<typename... Patterns>
inline void punsubscribe(context& c, Patterns... patterns)
{
	c.command({"PUNSUBSCRIBE", patterns...});
}

// Listen for messages published to the given channels
template<typename Channel, typename... Channels>
inline void subscribe(context& c, Channel channel, Channels... channels)
{
	c.command({"SUBSCRIBE", channel, channels...});
}

// Stop listening for messages posted to the given channels
template<typename... Channels>
inline void unsubscribe(context& c, Channels... channels)
{
	c.command({"UNSUBSCRIBE", channels...});
}
}

//  #####  #####     ##    #    #   ####     ##     ####    #####     #     ####   #    #
//    #    #    #   #  #   ##   #  #        #  #   #    #     #       #    #    #  ##   #
//    #    #    #  #    #  # #  #   ####   #    #  #          #       #    #    #  # #  #
//    #    #####   ######  #  # #       #  ######  #          #       #    #    #  #  # #
//    #    #   #   #    #  #   ##  #    #  #    #  #    #     #       #    #    #  #   ##
//    #    #    #  #    #  #    #   ####   #    #   ####      #       #     ####   #    #
namespace transaction
{
// Discard all commands issued after MULTI
inline auto discard(context& c) -> std::string
{
	return reply::status{c.command({"DISCARD"})};
}

// Execute all commands issued after MULTI
inline auto exec(context& c) -> std::vector<reply::reply_t>
{
	return reply::array{c.command({"EXEC"})};
}

// Mark the start of a transaction block
inline auto multi(context& c) -> std::string
{
	return reply::status{c.command({"MULTI"})};
}

// Forget about all watched keys
inline auto unwatch(context& c) -> std::string
{
	return reply::status{c.command({"UNWATCH"})};
}

// Watch the given keys to determine execution of the MULTI/EXEC block
template<typename Key, typename... Keys>
inline auto watch(context& c, Key key, Keys... keys) -> std::string
{
	return reply::status{c.command({"WATCH", key, keys...})};
}
}

//  ####    ####   #####      #    #####    #####
// #       #    #  #    #     #    #    #     #
//  ####   #       #    #     #    #    #     #
//      #  #       #####      #    #####      #
// #    #  #    #  #   #      #    #          #
//  ####    ####   #    #     #    #          #
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

//  ####    ####   #    #  #    #  ######   ####    #####     #     ####   #    #
// #    #  #    #  ##   #  ##   #  #       #    #     #       #    #    #  ##   #
// #       #    #  # #  #  # #  #  #####   #          #       #    #    #  # #  #
// #       #    #  #  # #  #  # #  #       #          #       #    #    #  #  # #
// #    #  #    #  #   ##  #   ##  #       #    #     #       #    #    #  #   ##
//  ####    ####   #    #  #    #  ######   ####      #       #     ####   #    #
namespace connection
{
// Authenticate to the server
inline auto auth(context& c, const std::string& password) -> std::string
{
	return reply::status{c.command({"AUTH", password})};
}

// Echo the given string
inline auto echo(context& c, const std::string& message) -> std::string
{
	return reply::string{c.command({"ECHO", message})};
}

// Ping the server
inline auto ping(context& c) -> std::string
{
	return reply::status{c.command({"PING"})};
}

// Close the connection
inline auto quit(context& c) -> std::string
{
	return reply::status{c.command({"QUIT"})};
}

// Change the selected database for the current connection
inline auto select(context& c, int index) -> std::string
{
	return reply::status{c.command({"SELECT", std::to_string(index)})};
}
}

//  ####   ######  #####   #    #  ######  #####
// #       #       #    #  #    #  #       #    #
//  ####   #####   #    #  #    #  #####   #    #
//      #  #       #####   #    #  #       #####
// #    #  #       #   #    #  #   #       #   #
//  ####   ######  #    #    ##    ######  #    #
namespace server
{
// Asynchronously rewrite the append-only file
inline auto bg_rewrite_aof(context& c) -> std::string
{
	return reply::status{c.command({"BGREWRITEAOF"})};
}

// Asynchronously save the dataset to disk
inline auto bg_save(context& c) -> std::string
{
	return reply::status{c.command({"BGSAVE"})};
}

namespace client
{
// Kill the connection of a client
inline auto kill(context& c, const std::string& address) -> std::string
{
	return reply::status{c.command({"CLIENT", "KILL", address})};
}

// Get the list of client connections
inline auto list(context& c) -> std::string
{
	return reply::string{c.command({"CLIENT", "LIST"})};
}

// Get the current connection name
inline auto get_name(context& c) -> boost::optional<std::string>
{
	auto value = c.command({"CLIENT", "GETNAME"});
	if(reply::is_nill(value))
		return {};
	return {true, reply::string{value}};
}

// Set the current connection name
inline auto set_name(context& c, const std::string& name) -> std::string
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
inline auto dbsize(context& c) -> long long
{
	return reply::integer{c.command({"DBSIZE"})};
}

//DEBUG OBJECT key
//Get debugging information about a key

//DEBUG SEGFAULT
//Make the server crash

// Remove all keys from all databases
inline auto flush_all(context& c) -> std::string
{
	return reply::status{c.command({"FLUSHALL"})};
}

// Remove all keys from the current database
inline auto flush_db(context& c) -> std::string
{
	return reply::status{c.command({"FLUSHDB"})};
}

// Get information and statistics about the server
inline auto info(context& c) -> std::string
{
	return reply::string{c.command({"INFO"})};
}
inline auto info(context& c, const std::string& section) -> std::string
{
	return reply::string{c.command({"INFO", section})};
}

// Get the UNIX time stamp of the last successful save to disk
inline auto last_save(context& c, const std::string& section) -> time_t
{
	return reply::integer{c.command({"LASTSAVE", section})};
}

//MONITOR
//Listen for all requests received by the server in real time

// Synchronously save the dataset to disk
inline auto save(context& c) -> std::string
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

}
}

#endif /* HIREDIS11_COMMANDS_H_ */
