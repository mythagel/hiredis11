#ifndef HIREDIS11_REPLY_H_
#define HIREDIS11_REPLY_H_
#include <hiredis/hiredis.h>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>
#include "error.hh"

namespace hiredis
{
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
}

#endif /* HIREDIS11_REPLY_H_ */
