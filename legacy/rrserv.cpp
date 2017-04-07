#include <iostream>
#include "zmqu/zmqu.hpp"

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_REP};
	responder.bind("tcp://*:5555");

	while (true)
	{
		std::string s = zmqu::recv(responder);
		std::cout << s << std::endl;
		zmqu::send(responder, "world");
	}

	return 0;
}
