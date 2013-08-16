#ifndef HIREDIS11_CONTEXT_H_
#define HIREDIS11_CONTEXT_H_
#include <hiredis/hiredis.h>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>
#include "reply.hh"

namespace hiredis
{

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

}

#endif /* HIREDIS11_CONTEXT_H_ */
