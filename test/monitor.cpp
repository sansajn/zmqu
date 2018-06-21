// monitor and it's release in pure zmq
#include <thread>
#include <string>
#include <iostream>
#include <zmq.hpp>

using std::cout;
using std::string;

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t * pub = new zmq::socket_t{ctx, ZMQ_PUB};
	pub->bind("tcp://*:5555");

	zmq::socket_t * sub = new zmq::socket_t{ctx, ZMQ_SUB};
	sub->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	sub->connect("tcp://localhost:5555");

	int result = zmq_socket_monitor((void *)*sub, "inproc://monitor", ZMQ_EVENT_ALL);
	assert(result == 0);
	zmq::socket_t * sub_mon = new zmq::socket_t{ctx, ZMQ_PAIR};
	sub_mon->connect("inproc://monitor");

	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	string feed = "Patric Jane";
	pub->send(feed.c_str(), feed.size());
	cout << "send" << std::endl;

	zmq::message_t msg{30};
	sub->recv(&msg);
	string received_feed{msg.data<char>(), msg.size()};

	cout << "received-feed=" << received_feed << std::endl;

	delete sub_mon;
	delete sub;
	delete pub;

	return 0;
}
