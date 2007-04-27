#include "explode.h"

std::vector<std::string>
split (const std::string &inString, const std::string &separator,
		unsigned int maxsplit)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0;
	std::string::size_type end = 0;
	unsigned int count = maxsplit;

	while ( ((end=inString.find (separator, start)) != std::string::npos) &&
			(maxsplit ? count-- : true) )
	{
		returnVector.push_back (inString.substr (start, end-start));
		start = end+separator.size();
	}

	returnVector.push_back (inString.substr (start));

	return returnVector;

} 

