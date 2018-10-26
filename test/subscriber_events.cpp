// explore raw socket events
#include <string>
#include <thread>
#include <iostream>
#include <cassert>
#include <zmq.h>
#include "zmqu/recv.hpp"
#include "zmqu/send.hpp"
#include "zmqu/cast.hpp"


using std::string;
using std::cout;

// non blocking listen
bool listen(zmq::socket_t & sub);
bool listen_monitor(zmq::socket_t & mon);

int main(int argc, char * argv[])
{
	zmq::context_t ctx;

	// publisher
	zmq::socket_t pub{ctx, ZMQ_PUB};
	pub.bind("tcp://*:5557");

	// subscriber
	zmq::socket_t sub{ctx, ZMQ_SUB};
	sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	sub.connect("tcp://localhost:5557");

	// subscriber monitor
	int rc = zmq_socket_monitor((void *)sub, "inproc://subscriber_monitor", ZMQ_EVENT_ALL);
	assert(rc >= 0);
	zmq::socket_t sub_mon{ctx, ZMQ_PAIR};
	sub_mon.connect("inproc://subscriber_monitor");

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for zmq to connect

	zmqu::send(pub, "hello");

	while (true)
	{
		listen(sub);
		listen_monitor(sub_mon);
	}

	return 0;
}

bool listen(zmq::socket_t & sub)
{
	string s;
	if (zmqu::try_recv(sub, s))
	{
		cout << s << std::endl;
		return true;
	}
	else
		return false;
}

bool listen_monitor(zmq::socket_t & mon)
{
	zmq_event_t event;
	if (zmqu::try_recv(mon, event))
	{
		string addr;
		if (zmqu::try_recv(mon, addr))
		{
			cout << "event=" << zmqu::event_to_string(event.event) << ", value=" << event.value << ", addr=" << addr << std::endl;
			return true;
		}
	}
	return false;
}
