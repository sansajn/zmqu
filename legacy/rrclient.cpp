#include <thread>
#include <iostream>
#include "zmqu/zmqu.hpp"

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	while (true)
	{
		zmqu::send_sync(requester, "hello");
		std::string s = zmqu::recv(requester);
		std::cout << s << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds{1});
	}

	return 0;
}
