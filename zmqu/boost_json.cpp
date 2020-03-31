#include <sstream>
#include <boost/property_tree/json_parser.hpp>
#include "boost_json.hpp"

using std::string;
using std::vector;
using std::stringstream;
namespace pt = boost::property_tree;

namespace std {

string to_string(jtree const & json, bool pretty)
{
	std::stringstream ss;
	pt::write_json(ss, json, pretty);
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

namespace zmqu {

void send_json(zmq::socket_t & sock, jtree & json)
{
	stringstream ss;
	pt::write_json(ss, json);
	string s = ss.str();
	zmq::message_t msg{s.size()};
	memcpy(msg.data(), s.data(), s.size());
	sock.send(msg);
}

void recv_json(zmq::socket_t & sock, jtree & json)
{
	zmq::message_t msg;
	sock.recv(&msg);
	string s(static_cast<char const *>(msg.data()), msg.size());
	stringstream ss{s};
	pt::read_json(ss, json);
}

}  // zmqu
