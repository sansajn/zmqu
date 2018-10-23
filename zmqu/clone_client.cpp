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
	: _ctx{ctx}
	, _subscriber{nullptr}
	, _requester{nullptr}
	, _notifier{nullptr}
	, _running{false}
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
	_host = host;
	_news_port = news_port;
	_ask_port = ask_port;
	_notification_port = notification_port;
}

void clone_client::start()
{
	assert(!_host.empty() && "invalid host");
	assert(!_running && "client already running");
	_running = true;

	if (!_ctx)
		_ctx = shared_ptr<zmq::context_t>{new zmq::context_t{}};

	// connect
	string const common_address = string{"tcp://"} + _host + ":";

	_subscriber = new zmq::socket_t{*_ctx, ZMQ_SUB};
	int linger_value = 0;
	_subscriber->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	_subscriber->connect((common_address + to_string(_news_port)).c_str());

	_requester = new zmq::socket_t{*_ctx, ZMQ_DEALER};
	_requester->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));  // do not block if socket is closed
	_requester->connect((common_address + to_string(_ask_port)).c_str());

	_notifier = new zmq::socket_t{*_ctx, ZMQ_PUSH};
	_notifier->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_notifier->connect((common_address + to_string(_notification_port)).c_str());

	install_monitors();

	loop();

	free_zmq();
	_running = false;
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

void clone_client::clear_incoming_message_queue(socket_id sid)
{
	if (sid != SUBSCRIBER)
		return;  // nothing to do

	zmq::message_t msg;
	while (_subscriber->recv(&msg, ZMQ_DONTWAIT))
		;
}

void clone_client::on_news(std::string const & news)
{}

void clone_client::on_answer(std::string const & answer)
{}

void clone_client::on_connected(socket_id sid, std::string const & addr)
{}

void clone_client::on_closed(socket_id sid, std::string const & addr)
{}

void clone_client::on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{}


void clone_client::loop()
{
	zmqu::poller socks;
	size_t sub_idx = socks.add(*_subscriber, ZMQ_POLLIN);
	size_t req_idx = socks.add(*_requester, ZMQ_POLLIN);

	while (!_quit)
	{
		// ask
		string question;
		for (int i = 100; i && _requester_queue.try_pop(question); --i)
			zmqu::send(*_requester, question);

		// notify
		string news;
		for (int i = 100; i && _notifier_queue.try_pop(news); --i)
			zmqu::send(*_notifier, news);

		// news, answers
		socks.poll(std::chrono::milliseconds{20});

		if (socks.has_input(sub_idx))
		{
			string s;
			zmqu::recv(*_subscriber, s);
			on_news(s);
		}

		if (socks.has_input(req_idx))
		{
			string s;
			zmqu::recv(*_requester, s);
			on_answer(s);
		}

		handle_monitor_events();

		if (!_quit)
			idle();
	}
}

void clone_client::install_monitors()
{
	// establish monitor channels
	int result = zmq_socket_monitor((void *)*_subscriber, SUBSCRIBER_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_requester, REQUESTER_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_notifier, NOTIFIER_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	// connect to monitor channels
	_sub_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	int linger_value = 0;
	_sub_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_sub_mon->connect(SUBSCRIBER_MON_ADDR);

	_req_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_req_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_req_mon->connect(REQUESTER_MON_ADDR);

	_notif_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_notif_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_notif_mon->connect(NOTIFIER_MON_ADDR);
}

void clone_client::handle_monitor_events()
{
	zmqu::poller socks;
	size_t sub_mon_idx = socks.add(*_sub_mon, ZMQ_POLLIN);
	size_t req_mon_idx = socks.add(*_req_mon, ZMQ_POLLIN);
	size_t notif_mon_idx = socks.add(*_notif_mon, ZMQ_POLLIN);

	socks.try_poll();

	// monitor events
	string addr;
	zmq_event_t event;

	if (socks.has_input(sub_mon_idx))
	{
		zmqu::recv(*_sub_mon, event, addr);
		socket_event(SUBSCRIBER, event, addr);
	}

	if (socks.has_input(req_mon_idx))
	{
		zmqu::recv(*_req_mon, event, addr);
		socket_event(REQUESTER, event, addr);
	}

	if (socks.has_input(notif_mon_idx))
	{
		zmqu::recv(*_notif_mon, event, addr);
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
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_requester, nullptr, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_notifier, nullptr, ZMQ_EVENT_ALL);
	assert(result != -1);

	delete _notif_mon;
	delete _req_mon;
	delete _sub_mon;
	delete _notifier;
	delete _requester;
	delete _subscriber;
}

}  // zmqu
