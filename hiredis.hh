#ifndef HIREDIS11_H_
#define HIREDIS11_H_
#include <string>
#include <memory>
#include "context.hh"
#include "commands.hh"

#include "error.hh"
#include "reply.hh"
#include "pipeline.hh"

namespace hiredis
{
namespace types
{

template <typename T>
struct Serialize
{
	static auto encode(T value) -> std::string;
};
template <>
struct Serialize<int>
{
	static auto encode(int value) -> std::string
	{
		return std::to_string(value);
	}
};
template <>
struct Serialize<uint64_t>
{
	static auto encode(uint64_t value) -> std::string
	{
		return std::to_string(value);
	}
};
template <>
struct Serialize<double>
{
	static auto encode(double value) -> std::string
	{
		return std::to_string(value);
	}
};

template <typename T>
class unordered_set
{
private:
	std::shared_ptr<context> c;
	std::string name;
public:
	unordered_set(std::shared_ptr<context> c, const std::string& name)
	 : c(c), name(name)
	{
	}
	
	bool empty()
	{
		return commands::set::card(*c, name) == 0;
	}
	std::size_t size()
	{
		return commands::set::card(*c, name);
	}
	
	template <typename... Keys>
	bool insert(Keys... keys)
	{
		return commands::set::add(*c, name, Serialize<T>::encode(keys)...) > 0;
	}
	
	bool erase(const T& key)
	{
		return commands::set::rem(*c, name, Serialize<T>::encode(key)) > 0;
	}
	
	bool exists(const T& key)
	{
		return commands::set::is_member(*c, name, Serialize<T>::encode(key));
	}
};

}

}

#endif /* HIREDIS11_H_ */
