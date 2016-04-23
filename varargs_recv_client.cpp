// variable arguments send sample
#include <iostream>
#include "zmqu/zmqu.hpp"

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	// send multipart message (name, age, salary)
	std::string name{"Jane"};
	int age = 32;
	double salary = 2400.0;
	zmq::send(requester, name, age, salary);

	std::string s = zmq::recv(requester);

	std::cout << "done!" << std::endl;
	return 0;
}
