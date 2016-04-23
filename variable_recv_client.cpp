#include <iostream>
#include "zmqu/zmqu.hpp"

using std::string;
using std::cout;

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	// send multipart message (name, age, salary)
	string name{"Jane"};
	int age = 32;
	double salary = 2400.0;
//	zmq::send(requester, name, true);
//	send(requester, age, true);
//	send(requester, salary);
	zmq::send(requester, name, age, salary);

	string s = zmq::recv(requester);


	cout << "done!" << std::endl;
	return 0;
}
