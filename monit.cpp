// client monitoring channel output sample
#include <string>
#include <thread>
#include <iostream>
#include "zmqu/clone_client.hpp"

using std::string;
using std::cout;

struct dummy_client_subscribe_impl : public zmqu::clone_client
{
	string last_news;

	void on_news(std::string const & s) override
	{
		last_news = s;
		cout << "message '" << s << "' received." << std::endl;
	}

	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override
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
};

int main(int argc, char * argv[])
{
	// dummy server listenning on 5556 port
	zmq::context_t ctx;
	zmq::socket_t serv{ctx, ZMQ_PUB};
	serv.bind("tcp://*:5556");

	dummy_client_subscribe_impl client;
	client.connect("localhost", 5556);

	std::thread client_thread{&dummy_client_subscribe_impl::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{100});  // wait for thread

	// send a news to client
	string expected{"hello jane!"};
	zmqu::send(serv, expected);
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for client

	cout << client.last_news;

	// wait and quit
	std::this_thread::sleep_for(std::chrono::seconds{3});
	zmqu::mailbox client_mail = client.create_mailbox();
	client.quit(client_mail);
	client_thread.join();
	cout << "done!" << std::endl;

	return 0;
}
