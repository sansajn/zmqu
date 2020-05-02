// boost fiber based ping/pong sample
#include <string>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <boost/fiber/all.hpp>
#include <zmqu/send.hpp>
#include <zmqu/recv.hpp>

using std::string;
using std::ref;
using std::runtime_error;
using std::cout,
	std::endl;
using namespace boost::fibers;
namespace this_fiber = boost::this_fiber;

using zmqu::send,
	zmqu::recv, zmqu::try_recv;

void request_job(zmq::socket_t & req)
{
	while (true)
	{
		send(req, "ping");

		string msg;
		while (!try_recv(req, msg))
			this_fiber::yield();

		if (msg == "pong")
			cout << msg << endl;
		else
			throw runtime_error{"unexpected message received (" + msg + ")"};
	}
}

void replay_job(zmq::socket_t & rep)
{
	while (true)
	{
		string msg;
		while (!try_recv(rep, msg))
			this_fiber::yield();

		if (msg == "ping")
		{
			cout << msg << endl;
			send(rep, "pong");
		}
		else
			throw runtime_error{"unexpected message received (" + msg + ")"};
	}
}

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t req{ctx, ZMQ_REQ},
		rep{ctx, ZMQ_REP};

	rep.bind("tcp://*:5555");
	req.connect("tcp://localhost:5555");

	fiber request_fb{request_job, ref(req)},
		replay_fb{replay_job, ref(rep)};

	request_fb.join();
	replay_fb.join();

	return 0;
}


