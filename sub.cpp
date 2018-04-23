// client events
#include <string>
#include <thread>
#include <iostream>
#include "zmqu/clone_client.hpp"

using std::cout;

struct subscriber_impl : public zmqu::clone_client
{
	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override;
};


int main(int argc, char * argv[])
{
	cout << "listenning: ..." << std::endl;

	subscriber_impl sub;
	sub.connect("localhost", 7777);
	std::thread t{&subscriber_impl::start, &sub};
	std::this_thread::sleep_for(std::chrono::milliseconds{100});  // wayt for thread

	t.join();  // loop ...

	return 0;
}


void subscriber_impl::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{
	cout << "on_socket_event(), ";
	switch (e.event)
	{
		case ZMQ_EVENT_CONNECTED:
			cout << " CONNECTED, sid:" << sid;
			break;

		case ZMQ_EVENT_DISCONNECTED:
			cout << "DISCONNECTED, sid:" << sid;
			break;

		case ZMQ_EVENT_CONNECT_DELAYED:
			cout << "CONNECT_DELAYED, sid:" << sid;
			break;

		case ZMQ_EVENT_CONNECT_RETRIED:
			cout << "CONNECT_RETRIED, sid:" << sid;
			break;

		case ZMQ_EVENT_LISTENING:
			cout << "LISTENING, sid:" << sid;
			break;

		case ZMQ_EVENT_BIND_FAILED:
			cout << "BIND_FAILED, sid:" << sid;
			break;

		case ZMQ_EVENT_ACCEPTED:
			cout << "ACCEPTED, sid:" << sid;
			break;

		case ZMQ_EVENT_ACCEPT_FAILED:
			cout << "ACCEPT_FAILED, sid:" << sid;
			break;

		case ZMQ_EVENT_CLOSED:
			cout << "CLOSED, sid:" << sid;
			break;

		case ZMQ_EVENT_CLOSE_FAILED:
			cout << "CLOSE_FAILED, sid:" << sid;
			break;

		case ZMQ_EVENT_MONITOR_STOPPED:
			cout << "MONITOR_STOPPED, sid:" << sid;
			break;

		default:
			cout << "unknown";
	}
	cout << std::endl;
}
