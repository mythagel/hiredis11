hiredis11
=========

**Incomplete.**

Sync interface is stable. Async doesn't exist. Wrapped commands may change with async interface. More commands are yet to be implemented. Pipeline class doesn't work with wrapped commands.

Basic sync interface
--------------------
 * context.hh
 * reply.hh
 * error.hh


Wrapped commands
----------------
 * commands.hh


Examples
--------
 * test.cpp

Example Code
------------

	using namespace hiredis;
	using namespace hiredis::commands;
	
	auto db = context("localhost", 6379);
	
	// 1. Basic API
	db.command({"GET", "foo"});
	
	reply::status res = db.command({"SET", "foo", std::string(100, '*')});
	std::cout << "res: " << res.value << "\n";
	
	// With results
	reply::string foo = db.command({"GET", "foo"});
	std::cout << foo.value << "\n";
	
	// 2. One step higher - wrapped functions
	
	//connection::auth(db, "a password");
	
	connection::select(db, 0);
	
	std::cout << server::client::list(db) << "\n";
	
	std::cout << connection::echo(db, "hello") << "\n";
	for(int i = 0; i < 10; ++i)
		std::cout << connection::ping(db) << "\n";
	
	std::cout << "get(foo)   : " << string::get(db, "foo") << "\n";
	std::cout << "get(foofoo): " << string::get(db, "foofoo") << "\n"; // nil
