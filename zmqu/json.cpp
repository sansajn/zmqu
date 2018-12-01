#include <sstream>
#include <boost/property_tree/json_parser.hpp>
#include "json.hpp"

using std::string;
using std::vector;
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

void to_json(string const & s, jtree & result)
{
	stringstream ss{s};
	pt::read_json(ss, result);
}

void vector_put(vector<string> const & v, string const & key, jtree & result)
{
	jtree arr;
	for (string const & elem : v)
	{
		jtree local;
		local.put("", elem);
		arr.push_back(make_pair("", local));
	}
	result.add_child(key, arr);
}

void vector_put(vector<size_t> const & v, string const & key, jtree & result)
{
	jtree arr;
	for (size_t const & elem : v)
	{
		jtree local;
		local.put<size_t>("", elem);
		arr.push_back(make_pair("", local));
	}
	result.add_child(key, arr);
}
