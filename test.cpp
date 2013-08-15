#include "hiredis.hh"
#include <iostream>

int main()
{
	using namespace hiredis;

	auto db = context{"localhost", 6379};

	// Basic API
	db.command({"GET", "foo"});
	reply::status res = db.command({"SET", "foo", std::string(100, '*')});
	std::cout << "res: " << res.value << "\n";
	
	// With results
	reply::string foo = db.command({"GET", "foo"});
	std::cout << foo.value << "\n";

	key::expire(db, "foo", 1);

	// One step higher - wrapped functions
	if(key::exists(db, "foo"))
		std::cout << "del: " << key::del(db, "foo") << "\n";
	else
		std::cout << "No key 'foo'\n";
	
	// Higher still - types.

	return 0;
}
