#include "hiredis.hh"
#include <iostream>

int main()
{
	using namespace hiredis;

	auto db = context{"localhost", 6379};

	// 1. Basic API
	db.command({"GET", "foo"});
	reply::status res = db.command({"SET", "foo", std::string(100, '*')});
	std::cout << "res: " << res.value << "\n";
	
	// With results
	reply::string foo = db.command({"GET", "foo"});
	std::cout << foo.value << "\n";


	// 2. One step higher - wrapped functions
	key::expire(db, "foo", 1);

	auto dump = key::dump(db, "foo");
	
	try
	{
		std::cout << "restore1: " << key::restore(db, "foo", 0, dump) << "\n";
	}
	catch(const error& e)
	{
		std::cout << "restore1: " << e.what() << "\n";
	}
	
	key::persist(db, "foo");

	if(key::exists(db, "foo"))
		std::cout << "del: " << key::del(db, "foo") << "\n";
	else
		std::cout << "No key 'foo'\n";
	
	std::cout << "restore2: " << key::restore(db, "foo", 0, dump) << "\n";
	
	if(key::exists(db, "foo"))
		std::cout << "del: " << key::del(db, "foo") << "\n";
	else
		std::cout << "No key 'foo'\n";
	
	if(key::exists(db, "foo"))
		std::cout << "del: " << key::del(db, "foo") << "\n";
	else
		std::cout << "No key 'foo'\n";
	
	// 3. Higher still - types.

	return 0;
}
