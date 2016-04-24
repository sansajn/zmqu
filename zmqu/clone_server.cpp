#include "clone_server.hpp"
#include <iostream>

namespace zmq {

using std::string;
using std::to_string;
using std::vector;
using std::clog;

clone_server::clone_server()
	: clone_server(std::shared_ptr<zmq::context_t>{})
{}

clone_server::clone_server(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
{
	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};

	_publisher = new zmq::socket_t{*_ctx, ZMQ_PUB};
	_responder = new zmq::socket_t{*_ctx, ZMQ_ROUTER};
	_collector = new zmq::socket_t{*_ctx, ZMQ_PULL};
	_inproc = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_publisher_mon = nullptr;
	_responder_mon = nullptr;
	_collector_mon = nullptr;
	_quit = false;
	_running = false;
}

clone_server::~clone_server()
{
	delete _publisher;
	delete _responder;
	delete _collector;
	delete _inproc;
	delete _publisher_mon;
	delete _responder_mon;
	delete _collector_mon;
}

void clone_server::bind(short first_port)
{
	bind(first_port, first_port+1, first_port+2);
}

void clone_server::bind(short publisher_port, short responder_port, short collector_port)
{
	string address{"tcp://*:"};

	string full_address = address + to_string(publisher_port);
	_publisher->bind(full_address.c_str());

	full_address = address + to_string(responder_port);
	_responder->bind(full_address.c_str());

	full_address = address + to_string(collector_port);
	_collector->bind(full_address.c_str());

	_inproc->bind("inproc://cloneserver.mailbox");
}

void clone_server::start()
{
	assert(!_running && "server already running");
	_running = true;
	loop();
}

mailbox clone_server::create_mailbox() const
{
	return mailbox{"inproc://cloneserver.mailbox", _ctx};
}

void clone_server::publish(mailbox & m, std::string const & feed)
{
	m.send("publish", feed);
}

void clone_server::command(mailbox & m, std::string const & cmd)
{
	m.send("command", cmd);
}

void clone_server::quit(mailbox & m) const
{
	m.send("command", "quit");
}

void clone_server::install_monitors(mailbox & m) const
{
	m.send("command", "install_monitors");
}

void clone_server::idle()
{}

void clone_server::publish_internal(std::string const & s)
{
	zmq::send(*_publisher, s);
}

std::string clone_server::on_question(std::string const & question)
{
	return string{};
}

void clone_server::on_notify(std::string const & s)
{}

void clone_server::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{}

void clone_server::loop()
{
	_socks.add(*_responder, ZMQ_POLLIN);
	_socks.add(*_collector, ZMQ_POLLIN);
	_socks.add(*_inproc, ZMQ_POLLIN);

	while (!_quit)
	{
		_socks.poll(std::chrono::milliseconds{20});

		if (_socks.has_input(0))  // responder requests as (identity, message) tupple
		{
			vector<string> msgs;
			zmq::recv(*_responder, msgs);
			assert(msgs.size() == 2 && "(identity, message) expected");

			msgs[1] = on_question(msgs[1]);

			zmq::send(*_responder, msgs);
		}

		if (_socks.has_input(1))  // collector
		{
			string m = zmq::recv(*_collector);
			on_notify(m);
		}

		if (_socks.has_input(2))  // inproc
		{
			vector<string> msgs;
			zmq::recv(*_inproc, msgs);
			assert(msgs.size() == 2 && "(title, message) expected");
			string & command = msgs[0];
			string & content = msgs[1];

			if (command == "publish")
				publish_internal(content);
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

void clone_server::handle_monitor_events()
{
	string addr;
	zmq_event_t e;

	if (_publisher_mon && _socks.has_input(3))
	{
		zmq::recv(*_publisher_mon, e, addr);
		on_socket_event(PUBLISHER, e, addr);
	}

	if (_responder_mon && _socks.has_input(4))
	{
		zmq::recv(*_responder_mon, e, addr);
		on_socket_event(RESPONDER, e, addr);
	}

	if (_collector_mon && _socks.has_input(5))
	{
		zmq::recv(*_collector_mon, e, addr);
		on_socket_event(COLLECTOR, e, addr);
	}
}

void clone_server::install_monitors_internal()
{
	if (_publisher_mon && _responder_mon && _collector_mon)  // monitors already installed
		return;

	int rc = zmq_socket_monitor((void *)*_publisher, "inproc://cloneserver.monitor.publisher", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_publisher_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_publisher_mon->connect("inproc://cloneserver.monitor.publisher");
	_socks.add(*_publisher_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_responder, "inproc://cloneserver.monitor.responder", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_responder_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_responder_mon->connect("inproc://cloneserver.monitor.responder");
	_socks.add(*_responder_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_collector, "inproc://cloneserver.monitor.collector", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_collector_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_collector_mon->connect("inproc://cloneserver.monitor.collector");
	_socks.add(*_collector_mon, ZMQ_POLLIN);
}

}  // zmq
