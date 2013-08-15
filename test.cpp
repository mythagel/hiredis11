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
	
	db.command({"SET", "a", "1"});
	db.command({"SET", "b", "1"});
	db.command({"SET", "c", "1"});
	db.command({"SET", "d", "1"});
	db.command({"SET", "e", "1"});
	db.command({"SET", "f", "1"});
	db.command({"SET", "g", "1"});
	
	// With results
	reply::string foo = db.command({"GET", "foo"});
	std::cout << foo.value << "\n";


	// 2. One step higher - wrapped functions
	
	//connection::auth(db, "a password");
	
	connection::select(db, 0);
	
	std::cout << connection::echo(db, "hello") << "\n";
	for(int i = 0; i < 10; ++i)
		std::cout << connection::ping(db) << "\n";

	std::cout << "get(foo)   : " << string::get(db, "foo") << "\n";
	std::cout << "get(foofoo): " << string::get(db, "foofoo") << "\n"; // nil
	
	key::expire(db, "foo", std::chrono::seconds{1});

	auto ks = key::keys(db, "*");
	for(auto& k : ks)
		std::cout << "keys: " << k << "\n";

	std::cout << "random: " << key::random(db) << "\n";
	
	std::cout << "type(foo): " << key::type(db, "foo") << "\n";
	std::cout << "type(a): " << key::type(db, "a") << "\n";

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
	
	//connection::quit(db);
	
	// 3. Higher still - types.

	return 0;
}
