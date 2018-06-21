// clone server socket events
#include <thread>
#include <iostream>
#include "zmqu/clone_server.hpp"

using std::cout;

struct publisher_impl : public zmqu::clone_server
{
	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override;
};

int main(int argc, char * argv[])
{
	cout << "broadcasting ..." << std::endl;

	publisher_impl pub;
	pub.bind("*", 7777, 7778, 7779);
	std::thread t{&publisher_impl::start, &pub};
	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	t.join();  // loop ...

	return 0;
}

void publisher_impl::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
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
