#include "clone_client.hpp"
#include <chrono>
#include <vector>
#include <iostream>

namespace zmq {

using std::string;
using std::to_string;
using std::vector;
using std::clog;

void read_monitor_event(zmq::socket_t & sock, zmq_event_t & e, std::string & addr);

clone_client::clone_client()
	: clone_client(std::shared_ptr<zmq::context_t>{})
{}

clone_client::clone_client(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
{
	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};

	// TODO: initialize in connect
	_subscriber = new zmq::socket_t{*_ctx, ZMQ_SUB};
	_requester = new zmq::socket_t{*_ctx, ZMQ_DEALER};
	_notifier = new zmq::socket_t{*_ctx, ZMQ_PUSH};
	_inproc = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_subscriber_mon = nullptr;
	_requester_mon = nullptr;
	_notifier_mon = nullptr;
	_quit = false;
	_running = false;
}

clone_client::~clone_client()
{
	delete _subscriber;
	delete _requester;
	delete _notifier;
	delete _inproc;
	delete _subscriber_mon;
	delete _requester_mon;
	delete _notifier_mon;
}

void clone_client::connect(string const & host, short first_port)
{
	connect(host, first_port, first_port+1, first_port+2);
}

void clone_client::connect(string const & host, short subscriber_port, short requester_port, short notifier_port)
{
	// TODO: arg: adresy, porty [a topic pre subscriber]
	_subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);

	string address = string{"tcp://"} + host + ":";
	string full_address = address + to_string(subscriber_port);
	_subscriber->connect(full_address.c_str());

	full_address = address + to_string(requester_port);
	_requester->connect(full_address.c_str());

	full_address = address + to_string(notifier_port);
	_notifier->connect(full_address.c_str());

	_inproc->bind("inproc://cloneclient.mailbox");
}

void clone_client::start()
{
	assert(!_running && "client already running");
	_running = true;
	loop();
}

mailbox clone_client::create_mailbox() const
{
	return mailbox{"inproc://cloneclient.mailbox", _ctx};
}

void clone_client::loop()
{
	_socks.add(*_requester, ZMQ_POLLIN);
	_socks.add(*_subscriber, ZMQ_POLLIN);
	_socks.add(*_inproc, ZMQ_POLLIN);

	while (!_quit)
	{
		_socks.poll(std::chrono::milliseconds{20});

		if (_socks.has_input(0))  // requester
		{
			string s;
			zmq::recv(*_requester, s);
			on_answer(s);
		}

		if (_socks.has_input(1))  // subscriber
		{
			string s;
			zmq::recv(*_subscriber, s);
			on_news(s);
		}

		if (_socks.has_input(2))  // inproc
		{
			vector<string> msgs;
			zmq::recv(*_inproc, msgs);
			assert(msgs.size() == 2 && "two frame message expected (command:int, content:string)");
			string & command = msgs[0];
			string & content = msgs[1];

			if (command == "ask")  // TODO: enum comparsion
				ask_internal(content);
			else if (command == "notify")
				notify_internal(content);
			else if (command == "command")
			{
				if (content == "quit")
					_quit = true;
				else if (content == "install_monitors")
					install_monitors_internal();
				else
					clog << "unknown command '" << content << "'" << std::endl;
			}
		}

		handle_monitor_events();

		idle();
	}  // while (!_quit

	_running = false;
}

void clone_client::handle_monitor_events()
{
	string addr;
	zmq_event_t e;

	if (_subscriber_mon && _socks.has_input(3))
	{
		read_monitor_event(*_subscriber_mon, e, addr);
		on_socket_event(SUBSCRIBER, e, addr);
	}

	if (_requester_mon && _socks.has_input(4))
	{
		read_monitor_event(*_requester_mon, e, addr);
		on_socket_event(REQUESTER, e, addr);
	}

	if (_notifier_mon && _socks.has_input(5))
	{
		read_monitor_event(*_notifier_mon, e, addr);
		on_socket_event(NOTIFIER, e, addr);
	}
}


void clone_client::install_monitors_internal()
{
	if (_subscriber_mon && _requester_mon && _notifier_mon)  // monitors already installed
		return;

	int rc = zmq_socket_monitor((void *)*_subscriber, "inproc://cloneclient.monitor.subscriber", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_subscriber_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_subscriber_mon->connect("inproc://cloneclient.monitor.subscriber");
	_socks.add(*_subscriber_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_requester, "inproc://cloneclient.monitor.requester", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_requester_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_requester_mon->connect("inproc://cloneclient.monitor.requester");
	_socks.add(*_requester_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_notifier, "inproc://cloneclient.monitor.notifier", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_notifier_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_notifier_mon->connect("inproc://cloneclient.monitor.notifier");
	_socks.add(*_notifier_mon, ZMQ_POLLIN);
}


void clone_client::ask(mailbox & m, std::string const & question) const
{
	m.send("ask", question);
}

void clone_client::notify(mailbox & m, std::string const & news) const
{
	m.send("notify", news);
}

void clone_client::command(mailbox & m, std::string const & cmd) const
{
	m.send("command", cmd);
}

void clone_client::install_monitors(mailbox & m) const
{
	m.send("command", "install_monitors");
}

void clone_client::quit(mailbox & m) const
{
	m.send("command", "quit");
}

void clone_client::on_news(string const & s)
{}

void clone_client::on_answer(string const & s)
{}

void clone_client::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{}

void clone_client::idle()
{}

void clone_client::ask_internal(std::string const & s)
{
	zmq::send(*_requester, s);
}

void clone_client::notify_internal(std::string const & s)
{
	zmq::send(*_notifier, s);
}

void read_monitor_event(zmq::socket_t & sock, zmq_event_t & e, std::string & addr)
{
	zmq::recv(sock, e, addr);
}

}  // zmq