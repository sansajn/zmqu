#include "clone_client.hpp"
#include <chrono>
#include <vector>
#include <iostream>

namespace zmqu {

using std::string;
using std::to_string;
using std::vector;
using std::clog;

static size_t const INVALID_IDX = (size_t)-1;
static char const * INPROC_SOCKET_ADDRESS = "inproc://cloneclient.mailbox";

void read_monitor_event(zmq::socket_t & sock, zmq_event_t & e, std::string & addr);

clone_client::clone_client()
	: clone_client(std::shared_ptr<zmq::context_t>{})
{}

clone_client::clone_client(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
	, _requester{nullptr}
	, _notifier{nullptr}
	, _subscriber_mon{nullptr}
	, _requester_mon{nullptr}
	, _notifier_mon{nullptr}
	, _subscriber_idx{INVALID_IDX}
	, _requester_idx{INVALID_IDX}
	, _inproc_idx{INVALID_IDX}
	, _subscriber_mon_idx{INVALID_IDX}
	, _requester_mon_idx{INVALID_IDX}
	, _notifier_mon_idx{INVALID_IDX}
	, _quit{false}
	, _running{false}
	, _subscriber_port{0}
	, _requester_port{0}
	, _notifier_port{0}
{
	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};
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
	// just save data for later
	_host = host;
	_subscriber_port = subscriber_port;
	_requester_port = requester_port;
	_notifier_port = notifier_port;
}

void clone_client::start()
{
	assert(!_running && "client already running");
	_running = true;

	// connect
	_subscriber = new zmq::socket_t{*_ctx, ZMQ_SUB};
	_requester = new zmq::socket_t{*_ctx, ZMQ_DEALER};
	_notifier = new zmq::socket_t{*_ctx, ZMQ_PUSH};
	_inproc = new zmq::socket_t{*_ctx, ZMQ_PAIR};

	_inproc->bind(INPROC_SOCKET_ADDRESS);
	string common_address = string{"tcp://"} + _host + ":";
	_subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	_subscriber->connect((common_address + to_string(_subscriber_port)).c_str());
	_requester->connect((common_address + to_string(_requester_port)).c_str());
	_notifier->connect((common_address + to_string(_notifier_port)).c_str());

	install_monitors();

	loop();  // blocking
}

mailbox clone_client::create_mailbox() const
{
	return mailbox{INPROC_SOCKET_ADDRESS, _ctx};
}

void clone_client::loop()
{
	_inproc_idx = _socks.add(*_inproc, ZMQ_POLLIN);
	_subscriber_idx = _socks.add(*_subscriber, ZMQ_POLLIN);
	_requester_idx = _socks.add(*_requester, ZMQ_POLLIN);

	while (!_quit)
	{
		on_wait();  // notify, we are waiting on message
		_socks.poll(std::chrono::milliseconds{20});

		if (_socks.has_input(_subscriber_idx))  // subscriber
		{
			on_receive();  // notify, we are received a message
			string s;
			zmqu::recv(*_subscriber, s);
			on_news(s);
		}

		if (_socks.has_input(_inproc_idx))  // inproc
		{
			on_receive();
			vector<string> msgs;
			zmqu::recv(*_inproc, msgs);
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
				else
					clog << "unknown command '" << content << "'" << std::endl;
			}
		}

		if (_socks.has_input(_requester_idx))  // requester
		{
			on_receive();  // notify, we are received a message
			string s;
			zmqu::recv(*_requester, s);
			on_answer(s);
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

	if (_subscriber_mon && _socks.has_input(_subscriber_mon_idx))
	{
		read_monitor_event(*_subscriber_mon, e, addr);
		on_socket_event(SUBSCRIBER, e, addr);
	}

	if (_requester_mon && _socks.has_input(_requester_mon_idx))
	{
		read_monitor_event(*_requester_mon, e, addr);
		on_socket_event(REQUESTER, e, addr);
	}

	if (_notifier_mon && _socks.has_input(_notifier_mon_idx))
	{
		read_monitor_event(*_notifier_mon, e, addr);
		on_socket_event(NOTIFIER, e, addr);
	}
}


void clone_client::install_monitors()
{
	int ret = zmq_socket_monitor((void *)*_subscriber, "inproc://cloneclient.monitor.subscriber", ZMQ_EVENT_ALL);
	assert(ret != -1);

	ret = zmq_socket_monitor((void *)*_requester, "inproc://cloneclient.monitor.requester", ZMQ_EVENT_ALL);
	assert(ret != -1);

	ret = zmq_socket_monitor((void *)*_notifier, "inproc://cloneclient.monitor.notifier", ZMQ_EVENT_ALL);
	assert(ret != -1);

	_subscriber_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_subscriber_mon->connect("inproc://cloneclient.monitor.subscriber");
	_subscriber_mon_idx = _socks.add(*_subscriber_mon, ZMQ_POLLIN);

	_requester_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_requester_mon->connect("inproc://cloneclient.monitor.requester");
	_requester_mon_idx = _socks.add(*_requester_mon, ZMQ_POLLIN);

	_notifier_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_notifier_mon->connect("inproc://cloneclient.monitor.notifier");
	_notifier_mon_idx = _socks.add(*_notifier_mon, ZMQ_POLLIN);
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

void clone_client::on_wait()
{}

void clone_client::on_receive()
{}

void clone_client::idle()
{}

void clone_client::ask_internal(std::string const & s)
{
	zmqu::send(*_requester, s);
}

void clone_client::notify_internal(std::string const & s)
{
	zmqu::send(*_notifier, s);
}

void read_monitor_event(zmq::socket_t & sock, zmq_event_t & e, std::string & addr)
{
	zmqu::recv(sock, e, addr);
}

}  // zmq
