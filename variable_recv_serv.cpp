// implementacia recv funkcie s variabilnym poctom argumentou
#include <iostream>
#include "zmqu/zmqu.hpp"

using std::string;
using std::cout;




int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_REP};
	responder.bind("tcp://*:5555");

	string name;
	int age;
	double salary;
	recv(responder, name, age, salary);

//	{
//		zmq::message_t msg;
//		responder.recv(&msg);
//		assert(msg.more());
//		name.assign((char *)msg.data(), msg.size());
//	}

//	{
//		zmq::message_t msg;
//		responder.recv(&msg);
//		assert(msg.more());
//		age = *(int *)msg.data();
//	}

//	{
//		zmq::message_t msg;
//		responder.recv(&msg);
//		assert(!msg.more());
//		salary = *(double *)msg.data();
//	}


	cout << "name:" << name << ", age:" << age << ", salary:" << salary << std::endl;

	responder.send("ok", 2);

	cout << "done!" << std::endl;
	return 0;
}
