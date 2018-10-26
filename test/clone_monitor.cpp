// clone pattern implementation sample with monitored events
#include <thread>
#include <iostream>
#include "zmqu/clone_client.hpp"
#include "zmqu/clone_server.hpp"

using std::cout;
using std::string;
using zmqu::clone_server;

struct cout_client : public zmqu::clone_client
{
	void on_news(string const & news) override
	{
		cout << news << std::endl;
	}

	void on_socket_event(socket_id sid, zmq_event_t const & e, string const & adr) override;
};

int main(int argc, char * argv[])
{
	clone_server serv;
	serv.bind("*", 5555, 5556, 5557);
	std::thread serv_loop{&clone_server::start, &serv};

	cout_client client;
	client.connect("localhost", 5555, 5556, 5557);
	std::thread client_loop{&cout_client::start, &client};

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for zmq to connect

	serv.publish("hello");

	while (true)
		;

	return 0;
}

void cout_client::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{
	cout << "on_socket_event(), ";
	switch (e.event)
	{
		case ZMQ_EVENT_CONNECTED:
			cout << "CONNECTED, sid:" << sid;
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
