#include "clone_server.hpp"
#include <iostream>

namespace zmq {

using std::string;
using std::to_string;
using std::vector;
using std::clog;

static size_t const INVALID_IDX = (size_t)-1;
static char const * INPROC_SOCKET_ADDRESS = "inproc://cloneserver.mailbox";

clone_server::clone_server()
	: clone_server(std::shared_ptr<zmq::context_t>{})
{}

clone_server::clone_server(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
	, _publisher{nullptr}
	, _responder{nullptr}
	, _collector{nullptr}
	, _inproc{nullptr}
	, _publisher_mon{nullptr}
	, _responder_mon{nullptr}
	, _collector_mon{nullptr}
	, _responder_idx{INVALID_IDX}
	, _collector_idx{INVALID_IDX}
	, _inproc_idx{INVALID_IDX}
	, _publisher_mon_idx{INVALID_IDX}
	, _responder_mon_idx{INVALID_IDX}
	, _collector_mon_idx{INVALID_IDX}
	, _quit{false}
	, _running{false}
	, _publisher_port{0}
	, _responder_port{0}
	, _collector_port{0}
{
	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};
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

void clone_server::bind(short first_port, std::string const & host)
{
	bind(first_port, first_port+1, first_port+2, host);
}

void clone_server::bind(short publisher_port, short responder_port, short collector_port, std::string const & host)
{
	// just save data for later
	_publisher_port = publisher_port;
	_responder_port = responder_port;
	_collector_port = collector_port;
	_host = host;
}

void clone_server::start()
{
	assert(!_running && "server already running");
	_running = true;

	_publisher = new zmq::socket_t{*_ctx, ZMQ_PUB};
	_responder = new zmq::socket_t{*_ctx, ZMQ_ROUTER};
	_collector = new zmq::socket_t{*_ctx, ZMQ_PULL};
	_inproc = new zmq::socket_t{*_ctx, ZMQ_PAIR};

	// bind
	string common_address = string{"tcp://"} + _host + string{":"};
	_publisher->bind((common_address + to_string(_publisher_port)).c_str());
	_responder->bind((common_address + to_string(_responder_port)).c_str());
	_collector->bind((common_address + to_string(_collector_port)).c_str());
	_inproc->bind(INPROC_SOCKET_ADDRESS);

	install_monitors();

	loop();
}

mailbox clone_server::create_mailbox() const
{
	return mailbox{INPROC_SOCKET_ADDRESS, _ctx};
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

void clone_server::idle()
{}

void clone_server::publish_internal(std::string const & s)
{
	zmq::send(*_publisher, s);
}

std::string clone_server::on_question(std::string const &)
{
	return string{};
}

void clone_server::on_notify(std::string const &)
{}

void clone_server::on_socket_event(socket_id, zmq_event_t const &, std::string const &)
{}

void clone_server::on_wait()
{}

void clone_server::on_receive()
{}

void clone_server::loop()
{
	_responder_idx = _socks.add(*_responder, ZMQ_POLLIN);
	_collector_idx = _socks.add(*_collector, ZMQ_POLLIN);
	_inproc_idx = _socks.add(*_inproc, ZMQ_POLLIN);

	while (!_quit)
	{
		on_wait();
		_socks.poll(std::chrono::milliseconds{20});

		if (_socks.has_input(_responder_idx))  // responder requests as (identity, message) tupple
		{
			on_receive();
			vector<string> msgs;
			zmq::recv(*_responder, msgs);
			assert(msgs.size() == 2 && "(identity, message) expected");

			msgs[1] = on_question(msgs[1]);

			zmq::send(*_responder, msgs);
		}

		if (_socks.has_input(_collector_idx))  // collector
		{
			on_receive();
			string m = zmq::recv(*_collector);
			on_notify(m);
		}

		if (_socks.has_input(_inproc_idx))  // inproc
		{
			on_receive();
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

	if (_publisher_mon && _socks.has_input(_publisher_mon_idx))
	{
		zmq::recv(*_publisher_mon, e, addr);
		on_socket_event(PUBLISHER, e, addr);
	}

	if (_responder_mon && _socks.has_input(_responder_mon_idx))
	{
		zmq::recv(*_responder_mon, e, addr);
		on_socket_event(RESPONDER, e, addr);
	}

	if (_collector_mon && _socks.has_input(_collector_mon_idx))
	{
		zmq::recv(*_collector_mon, e, addr);
		on_socket_event(COLLECTOR, e, addr);
	}
}

void clone_server::install_monitors()
{
	if (_publisher_mon && _responder_mon && _collector_mon)  // monitors already installed
		return;

	int rc = zmq_socket_monitor((void *)*_publisher, "inproc://cloneserver.monitor.publisher", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_publisher_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_publisher_mon->connect("inproc://cloneserver.monitor.publisher");
	_publisher_mon_idx = _socks.add(*_publisher_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_responder, "inproc://cloneserver.monitor.responder", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_responder_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_responder_mon->connect("inproc://cloneserver.monitor.responder");
	_responder_mon_idx = _socks.add(*_responder_mon, ZMQ_POLLIN);

	rc = zmq_socket_monitor((void *)*_collector, "inproc://cloneserver.monitor.collector", ZMQ_EVENT_ALL);
	assert(rc != -1);
	_collector_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_collector_mon->connect("inproc://cloneserver.monitor.collector");
	_collector_mon_idx = _socks.add(*_collector_mon, ZMQ_POLLIN);
}

}  // zmq
