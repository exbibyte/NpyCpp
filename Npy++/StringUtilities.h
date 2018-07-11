#pragma once

#include <string>
#include <sstream>
#include <iterator>

namespace strutils
{
	template <class ContainerT>
	ContainerT Tokenize(const std::string& str)
	{
		ContainerT out;

		std::istringstream iss(str);
		std::copy(std::istream_iterator<std::string>(iss),
				  std::istream_iterator<std::string>(),
				  std::back_inserter(out));

		return out;
	}

	template <class ContainerT>
	ContainerT Tokenize(const std::string& str, const char delim)
	{
		ContainerT out;

		std::stringstream ss(str);
		std::string token;
		while (std::getline(ss, token, delim))
			out.push_back(token);

		return out;
	}
}
