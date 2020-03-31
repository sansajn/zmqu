#include <functional>
#include <thread>
#include <iostream>
#include "poller.hpp"
#include "clone_client.hpp"
#include "recv.hpp"
#include "send.hpp"

namespace zmqu {

constexpr char const * SUBSCRIBER_MON_ADDR = "inproc://cloneclient.monitor.subscriber";
constexpr char const * REQUESTER_MON_ADDR = "inproc://cloneclient.monitor.requester";
constexpr char const * NOTIFIER_MON_ADDR = "inproc://cloneclient.monitor.notifier";

using std::string;
using std::to_string;
using std::shared_ptr;

clone_client::clone_client()
	: clone_client{shared_ptr<zmq::context_t>{}}
{}

clone_client::clone_client(shared_ptr<zmq::context_t> ctx)
	:  _ctx{ctx}
	, _subscriber{nullptr}
	, _requester{nullptr}
	, _notifier{nullptr}
	, _ready{false}
	, _quit{false}
	, _news_port{0}
	, _ask_port{0}
	, _notification_port{0}
	, _sub_mon{nullptr}
	, _req_mon{nullptr}
	, _notif_mon{nullptr}
{}

clone_client::~clone_client()
{
	_quit = true;
}

void clone_client::connect(string const & host, short news_port, short ask_port, short notification_port)
{
	assert(!host.empty() && "invalid host");
	_host = host;
	_news_port = news_port;
	_ask_port = ask_port;
	_notification_port = notification_port;
}

void clone_client::start()
{
	call_once(_running, std::bind(&clone_client::setup_and_run, this));
}

void clone_client::setup_and_run()
{
	if (!_ctx)
		_ctx = shared_ptr<zmq::context_t>{new zmq::context_t{}};

	// connect
	string const common_address = string{"tcp://"} + _host + ":";

	_subscriber = new zmq::socket_t{*_ctx, ZMQ_SUB};
	int linger_value = 0,
		reconnect_interval_ms{50};
	_subscriber->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	_subscriber->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_subscriber->connect((common_address + to_string(_news_port)).c_str());

	_requester = new zmq::socket_t{*_ctx, ZMQ_DEALER};
	_requester->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));  // do not block if socket is closed
	_requester->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_requester->connect((common_address + to_string(_ask_port)).c_str());

	_notifier = new zmq::socket_t{*_ctx, ZMQ_PUSH};
	_notifier->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_notifier->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_notifier->connect((common_address + to_string(_notification_port)).c_str());

	install_monitors();

	_ready = true;

	loop();

	_ready = false;

	free_zmq();
}

void clone_client::ask(string const & question) const
{
	_requester_queue.push(question);
}

void clone_client::notify(std::string const & news) const
{
	_notifier_queue.push(news);
}

void clone_client::idle()
{}

void clone_client::quit()
{
	_quit = true;
}

bool clone_client::ready() const
{
	return _ready;
}

int clone_client::clear_incoming_subscriber_message_queue()
{
	int removed_message_count = 0;
	zmq::message_t msg;
	while (_subscriber->recv(&msg, ZMQ_DONTWAIT))
		++removed_message_count;

	return removed_message_count;
}

void clone_client::on_news(string const &)
{}

void clone_client::on_answer(string const &)
{}

void clone_client::on_connected(socket_id, string const &)
{}

void clone_client::on_closed(socket_id, string const &)
{}

void clone_client::on_socket_event(socket_id, zmq_event_t const &, string const &)
{}

void clone_client::loop()
{
	while (!_quit)
	{
		handle_monitor_events();

		// ask
		string question;
		for (int i = 100; i && _requester_queue.try_pop(question); --i)
			zmqu::send(*_requester, question);

		// notify
		string news;
		for (int i = 100; i && _notifier_queue.try_pop(news); --i)
			zmqu::send(*_notifier, news);

		// news, answers
		string msg;
		while (zmqu::try_recv(*_subscriber, msg))
			on_news(msg);

		while (zmqu::try_recv(*_requester, msg))
			on_answer(msg);

		if (!_quit)
			idle();
	}
}

void clone_client::install_monitors()
{
	// establish monitor channels
	int result = zmq_socket_monitor((void *)*_subscriber, SUBSCRIBER_MON_ADDR, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " subscriber error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_requester, REQUESTER_MON_ADDR, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " requester error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_notifier, NOTIFIER_MON_ADDR, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " notifier error" << std::endl;
		assert(0);
	}

	// connect to monitor channels
	_sub_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	int linger_value = 0;
	int reconnect_interval_ms = 50;
	_sub_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_sub_mon->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_sub_mon->connect(SUBSCRIBER_MON_ADDR);

	_req_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_req_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_req_mon->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_req_mon->connect(REQUESTER_MON_ADDR);

	_notif_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_notif_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_notif_mon->setsockopt(ZMQ_RECONNECT_IVL, (void const *)&reconnect_interval_ms, sizeof(int));
	_notif_mon->connect(NOTIFIER_MON_ADDR);
}

void clone_client::handle_monitor_events()
{
	string addr;
	zmq_event_t event;

	while (zmqu::try_recv(*_sub_mon, event))
	{
		zmqu::recv(*_sub_mon, addr);
		socket_event(SUBSCRIBER, event, addr);
	}

	while (zmqu::try_recv(*_req_mon, event))
	{
		zmqu::recv(*_req_mon, addr);
		socket_event(REQUESTER, event, addr);
	}

	while (zmqu::try_recv(*_notif_mon, event))
	{
		zmqu::recv(*_notif_mon, addr);
		socket_event(NOTIFIER, event, addr);
	}
}

void clone_client::socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{
	switch (e.event)
	{
		case ZMQ_EVENT_CONNECTED:
			on_connected(sid, addr);
			break;
		case ZMQ_EVENT_CLOSED:
			on_closed(sid, addr);
			break;
	}

	on_socket_event(sid, e, addr);
}

void clone_client::free_zmq()
{
	// libzmq 4 suffers from hanging if zmq_socket_monitor() used, see https://github.com/zeromq/libzmq/issues/1279
	int result = zmq_socket_monitor((void *)*_subscriber, nullptr, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " subscriber error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_requester, nullptr, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " requester error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_notifier, nullptr, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " notifier error" << std::endl;
		assert(0);
	}

	delete _notif_mon;
	delete _req_mon;
	delete _sub_mon;
	delete _notifier;
	delete _requester;
	delete _subscriber;
}

}  // zmqu
