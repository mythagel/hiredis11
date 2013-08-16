#ifndef HIREDIS11_PIPELINE_H_
#define HIREDIS11_PIPELINE_H_
#include "context.hh"
#include <vector>
#include <algorithm>
#include "reply.hh"

namespace hiredis
{

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

}

#endif /* HIREDIS11_PIPELINE_H_ */
