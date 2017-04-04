// variable argument receive sample
#include <iostream>
#include <thread>
#include <gtest/gtest.h>
#include "zmqu/zmqu.hpp"

using std::string;
using std::cout;


void simple_varargs_client(zmq::context_t & ctx)
{
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	// send multipart message (name, age, salary)
	string name{"Jane"};
	int age = 32;
	double salary = 2400.0;
	zmq::send_multipart(requester, name);
	zmq::send_multipart(requester, age);
	zmq::send(requester, salary);

	string s = zmq::recv(requester);

	// done
}

void simple_varargs_client_with_varargs_send(zmq::context_t & ctx)
{
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	// send multipart message (name, age, salary)
	string name{"Jane"};
	int age = 32;
	double salary = 2400.0;
	zmq::send(requester, name, age, salary);

	string s = zmq::recv(requester);

	// done
}


TEST(variable_arguments, receive_test)
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_REP};
	responder.bind("tcp://*:5555");

	// install client
	std::thread client_thread{simple_varargs_client, std::ref(ctx)};
	std::this_thread::sleep_for(std::chrono::milliseconds{20});  // wait for client

	string name;
	int age;
	double salary;
	recv(responder, name, age, salary);

	EXPECT_EQ(string{"Jane"}, name);
	EXPECT_EQ(32, age);
	EXPECT_EQ(2400.0, salary);

	responder.send("ok", 2);

	client_thread.join();
}


TEST(variable_arguments, send_receive_test)
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_REP};
	responder.bind("tcp://*:5555");

	// install client
	std::thread client_thread{simple_varargs_client_with_varargs_send, std::ref(ctx)};
	std::this_thread::sleep_for(std::chrono::milliseconds{20});  // wait for client

	string name;
	int age;
	double salary;
	recv(responder, name, age, salary);

	EXPECT_EQ(string{"Jane"}, name);
	EXPECT_EQ(32, age);
	EXPECT_EQ(2400.0, salary);

	responder.send("ok", 2);

	client_thread.join();
}
