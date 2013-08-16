#ifndef HIREDIS11_ERROR_H_
#define HIREDIS11_ERROR_H_
#include <stdexcept>
#include <string>

namespace hiredis
{

struct error : std::runtime_error
{
	error(const std::string& what)
	 : std::runtime_error(what)
	{
	}
};

}

#endif /* HIREDIS11_ERROR_H_ */
