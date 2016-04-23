#include "json.hpp"
#include <sstream>

using std::stringstream;
namespace pt = boost::property_tree;

namespace std {

string to_string(jtree const & json)
{
	std::stringstream ss;
	pt::write_json(ss, json);
	return ss.str();
}

}  // std

void to_json(std::string const & s, jtree & result)
{
	stringstream ss{s};
	pt::read_json(ss, result);
}
