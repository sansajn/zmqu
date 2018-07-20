// variable argument receive sample
#include <vector>
#include <thread>
#include <iostream>
#include <catch.hpp>
#include <zmqu/recv.hpp>
#include <zmqu/send.hpp>

using std::vector;
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
	zmqu::send_multipart_sync(requester, name);
	zmqu::send_multipart_sync(requester, age);
	zmqu::send_sync(requester, salary);

	string s = zmqu::recv(requester);

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
	zmqu::send_sync(requester, name, age, salary);

	string s = zmqu::recv(requester);

	// done
}

TEST_CASE("receive more variables with multipart send", "[recv,send]")
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
	zmqu::recv(responder, name, age, salary);

	REQUIRE(name == string{"Jane"});
	REQUIRE(age == 32);
	REQUIRE(salary == 2400.0);

	responder.send("ok", 2);

	client_thread.join();
}


TEST_CASE("receive more variables with single send", "[recv,send")
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
	zmqu::recv(responder, name, age, salary);

	REQUIRE(name == string{"Jane"});
	REQUIRE(age == 32);
	REQUIRE(salary == 2400.0);

	responder.send("ok", 2);

	client_thread.join();
}
