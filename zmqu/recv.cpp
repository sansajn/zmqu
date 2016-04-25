#include "recv.hpp"
#include <sstream>
#include <boost/property_tree/json_parser.hpp>

namespace zmq {

using std::string;
using std::stringstream;
namespace pt = boost::property_tree;


std::string recv(zmq::socket_t & sock)
{
	zmq::message_t msg;
	sock.recv(&msg);
	return string{static_cast<char const *>(msg.data()), msg.size()};
}

void recv_json(zmq::socket_t & sock, jtree & json)
{
	zmq::message_t msg;
	sock.recv(&msg);
	string s(static_cast<char const *>(msg.data()), msg.size());
	stringstream ss{s};
	pt::read_json(ss, json);
}

void recv_one(zmq::socket_t & sock, std::vector<std::string> & vec)
{
	zmq::message_t msg;

	do
	{
		sock.recv(&msg);
		vec.emplace_back(static_cast<char const *>(msg.data()), msg.size());
	}
	while (msg.more());
}

}  // zmq
