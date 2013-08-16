#include "async_context.hh"

int main()
{
	using namespace hiredis;
	
	boost::asio::io_service io_service;
	
	async_context c("127.0.0.1", 6379, io_service);
	
	try
	{
		io_service.run();
	}
	catch(const std::exception& ex)
	{
		throw;
	}
	
	return 0;
}
