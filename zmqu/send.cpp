#include "send.hpp"
#include <sstream>
#include <boost/property_tree/json_parser.hpp>

namespace zmqu {

using std::string;
using std::stringstream;
namespace pt = boost::property_tree;


void send_json(zmq::socket_t & sock, jtree & json)
{
	stringstream ss;
	pt::write_json(ss, json);
	string s = ss.str();
	zmq::message_t msg{s.size()};
	memcpy(msg.data(), s.data(), s.size());
	sock.send(msg);
}

}  // zmq
