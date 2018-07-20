// demonstrates ZMQ send hanging in sync mode
#include <string>
#include <boost/log/trivial.hpp>
#include "zmqu/send.hpp"

using std::string;

int main(int argc, char * argv[])
{
	string const msg = "hello Patrick";

	zmq::context_t ctx;

	zmq::socket_t sock{ctx, ZMQ_PUSH};
	sock.connect("tcp://localhost:13333");

	BOOST_LOG_TRIVIAL(info) << "sending ...";

	while (true)
	{
		zmqu::send_sync(sock, msg);  // will hang after some time
		BOOST_LOG_TRIVIAL(info) << msg;
	}

	return 0;
}
