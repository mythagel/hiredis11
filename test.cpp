#include "redis.hh"

int main()
{
	using namespace redis;
	
	auto db = context{"localhost", 6379};

	return 0;
}
