// request/repeat ping pong blocking fashion implementation sample
#include <string>
#include <stdexcept>
#include <iostream>
#include <zmqu/send.hpp>
#include <zmqu/recv.hpp>

using std::string;
using std::runtime_error;
using std::cout,
	std::endl;
using zmqu::send,
	zmqu::recv;

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t req{ctx, ZMQ_REQ},
		rep{ctx, ZMQ_REP};

	rep.bind("tcp://*:5555");
	req.connect("tcp://localhost:5555");

	while (true)
	{
		send(req, "ping");

		string msg;
		recv(rep, msg);  // blocking
		if (msg == "ping")
		{
			cout << msg << endl;
			send(rep, "pong");
		}
		else
			throw runtime_error{"unexpected message received (" + msg + ")"};

		recv(req, msg);  // blocking
		if (msg == "pong")
			cout << msg << endl;
		else
			throw runtime_error{"unexpected message received (" + msg + ")"};
	}
}
