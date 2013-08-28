#include "hiredis.hh"
#include <iostream>

int main()
{
	using namespace hiredis;
	using namespace hiredis::commands;

	auto c = std::make_shared<context>("localhost", 6379);
	auto& db = *c;

	// 1. Basic API
	db.command({"GET", "foo"});
	reply::status res = db.command({"SET", "foo", std::string(100, '*')});
	std::cout << "res: " << res.value << "\n";
	
	// With results
	reply::string foo = db.command({"GET", "foo"});
	std::cout << foo.value << "\n";

	pipeline p(db);
	p.command({"SET", "a", "1"});
	p.command({"SET", "b", "1"});
	p.command({"SET", "c", "1"});
	p.command({"SET", "d", "1"});
	p.command({"SET", "e", "1"});
	p.command({"SET", "f", "1"});
	p.command({"SET", "g", "1"});
	auto replies = p.execute();
	std::cout << "replies.size(): " << replies.size() << "\n";

	// 2. One step higher - wrapped functions
	
	//connection::auth(db, "a password");
	
	connection::select(db, 0);
	
	std::cout << server::client::list(db) << "\n";
	
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
	
	std::map<std::string, std::string> h;
	h["hello"] = "world";
	hash::set(db, "foo_hash", h);
	std::cout << "foo_hash.hello: " << *hash::get(db, "foo_hash", "hello") << "\n";
	
	auto h2 = hash::get(db, "foo_hash");
	std::cout << "h == h2: " << (h == h2) << "\n";
	auto h3 = hash::get(db, "foo_hash", "hello", "non_existing");
	std::cout << "h == h3: " << (h == h3) << "\n";
	
	//connection::quit(db);
	
	// 3. Higher still - types.

	auto set = types::unordered_set<uint64_t>(c, "testset1");
	
	std::cout << set.size() << "\n";
	
	for(int i = 0; i < 10; ++i)
		set.insert(i, i+1, i+2, i+3, i+4);
	
	std::cout << "type(testset1): " << key::type(db, "testset1") << "\n";
	std::cout << set.size() << "\n";
	
	return 0;
}
