// async send should not hang
#include <string>
#include <boost/log/trivial.hpp>
#include "zmqu/send.hpp"

using std::string;

int main(int argc, char * argv[])
{
	string const msg = "hello Patrick";

	zmq::context_t ctx;

	zmq::socket_t sock{ctx, ZMQ_PUSH};
	sock.setsockopt(ZMQ_LINGER, 0);
	sock.connect("tcp://localhost:13333");

	BOOST_LOG_TRIVIAL(info) << "sending ...";

	size_t total_bytes = 0;

	while (true)
	{
		size_t bytes = zmqu::send(sock, msg);
		if (bytes == 0)
			break;

		total_bytes += bytes;
		BOOST_LOG_TRIVIAL(info) << total_bytes << " bytes send";
	}

	BOOST_LOG_TRIVIAL(info) << "done";

	sock.close();

	return 0;
}
