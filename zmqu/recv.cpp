#include "recv.hpp"
#include <sstream>
#include <boost/property_tree/json_parser.hpp>

namespace zmqu {

using std::string;
using std::stringstream;
namespace pt = boost::property_tree;

string recv(zmq::socket_t & sock)
{
	zmq::message_t msg;
	sock.recv(&msg);
	return string{static_cast<char const *>(msg.data()), msg.size()};
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
