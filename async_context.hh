#ifndef HIREDIS11_ASYNC_CONTEXT_H_
#define HIREDIS11_ASYNC_CONTEXT_H_
#include <hiredis/async.h>
#include <boost/asio.hpp>
#include <memory>
#include <stdexcept>
#include "reply.hh"

namespace hiredis
{

class async_context
{
private:
	std::shared_ptr<redisAsyncContext> ac;
	boost::asio::io_service& io_service;
	
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

	async_context(const std::string& ip, int port, boost::asio::io_service& io_service)
	 : ac(redisAsyncConnect(ip.c_str(), port), redisAsyncDisconnect), io_service(io_service)
	{
		if(!c)
			throw error("Unable to create context");
		if(c->err)
			throw error(c->errstr);
		
		// init asio context shit
		c->ev.data = this;
		
		
		
	    redisContext *c = &(ac->c);
		   
		redisLibevEvents *e;

		/* Nothing should be attached when something is already attached */
		if (ac->ev.data != NULL)
		    return REDIS_ERR;

		/* Create container for context and r/w events */
		e = (redisLibevEvents*)malloc(sizeof(*e));
		e->context = ac;
	#if EV_MULTIPLICITY
		e->loop = loop;
	#else
		e->loop = NULL;
	#endif
		e->reading = e->writing = 0;
		e->rev.data = e;
		e->wev.data = e;

		/* Register functions to start/stop listening for events */
		ac->ev.addRead = redisLibevAddRead;
		ac->ev.delRead = redisLibevDelRead;
		ac->ev.addWrite = redisLibevAddWrite;
		ac->ev.delWrite = redisLibevDelWrite;
		ac->ev.cleanup = redisLibevCleanup;
		ac->ev.data = e;

		/* Initialize read/write events */
		ev_io_init(&e->rev,redisLibevReadEvent,c->fd,EV_READ);
		ev_io_init(&e->wev,redisLibevWriteEvent,c->fd,EV_WRITE);
		return REDIS_OK;
	}
	
	operator redisAsyncContext*() const
	{
		return c.get();
	}
	
	async_context(const async_context&) = delete;
	async_context& operator=(const async_context&) = delete;
	
	async_context(async_context&&) = default;
	async_context& operator=(async_context&&) = default;
};

}

#endif /* HIREDIS11_ASYNC_CONTEXT_H_ */
