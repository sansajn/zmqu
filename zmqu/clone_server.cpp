#include "send.hpp"
#include "recv.hpp"
#include "poller.hpp"
#include "clone_server.hpp"

namespace zmqu {

constexpr char const * PUBLISHER_MON_ADDR = "inproc://cloneserver.monitor.publisher";
constexpr char const * RESPONDER_MON_ADDR = "inproc://cloneserver.monitor.responder";
constexpr char const * COLLECTOR_MON_ADDR = "inproc://cloneserver.monitor.collector";

using std::string;
using std::to_string;
using std::vector;

clone_server::clone_server()
	: clone_server(std::shared_ptr<zmq::context_t>{})
{}

clone_server::clone_server(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
	, _publisher{nullptr}
	, _responder{nullptr}
	, _collector{nullptr}
	, _running{false}
	, _quit{false}
	, _news_port{0}
	, _answer_port{0}
	, _notification_port{0}
	, _pub_mon{nullptr}
	, _resp_mon{nullptr}
	, _coll_mon{nullptr}
{}

clone_server::~clone_server()
{
	_quit = true;
}

void clone_server::bind(short news_port, short answer_port, short notification_port)
{
	bind("*", news_port, answer_port, notification_port);
}

void clone_server::bind(std::string const & host, short news_port, short answer_port, short notification_port)
{
	_host = host;
	_news_port = news_port;
	_answer_port = answer_port;
	_notification_port = notification_port;
}

void clone_server::start()
{
	assert(!_running && "server already running");
	_running = true;

	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};

	string common_address = string{"tcp://"} + _host + string{":"};

	_publisher = new zmq::socket_t{*_ctx, ZMQ_PUB};
	int linger_value = 0;
	_publisher->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_publisher->bind((common_address + to_string(_news_port)).c_str());

	_responder = new zmq::socket_t{*_ctx, ZMQ_ROUTER};
	_responder->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_responder->bind((common_address + to_string(_answer_port)).c_str());

	_collector = new zmq::socket_t{*_ctx, ZMQ_PULL};
	_collector->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_collector->bind((common_address + to_string(_notification_port)).c_str());

	install_monitors();

	loop();

	free_zmq();
	_running = false;
}

void clone_server::publish(std::string const & news) const
{
	_publisher_queue.push(news);
}

void clone_server::quit()
{
	_quit = true;
}

void clone_server::idle()
{}

std::string clone_server::on_question(std::string const &)
{
	return string{};
}

void clone_server::on_notify(std::string const &)
{}

void clone_server::on_accepted(socket_id, std::string const &)
{}

void clone_server::on_disconnected(socket_id, std::string const &)
{}

void clone_server::on_socket_event(socket_id, zmq_event_t const &, std::string const &)
{}

void clone_server::loop()
{
	zmqu::poller socks;
	size_t resp_idx = socks.add(*_responder, ZMQ_POLLIN);
	size_t coll_idx = socks.add(*_collector, ZMQ_POLLIN);

	while (!_quit)
	{
		// publish
		string news;
		for (int i = 100; i && _publisher_queue.try_pop(news); --i)
			zmqu::send(*_publisher, news);

		// answer, notifications
		socks.poll(std::chrono::milliseconds{20});

		if (socks.has_input(resp_idx))
		{
			vector<string> msgs;
			zmqu::recv(*_responder, msgs);
			assert(msgs.size() == 2 && "(identify, message) expected");
			msgs[1] = on_question(msgs[1]);
			zmqu::send(*_responder, msgs);
		}

		if (socks.has_input(coll_idx))
		{
			string m = zmqu::recv(*_collector);
			on_notify(m);
		}

		handle_monitor_events();

		if (!_quit)
			idle();
	}
}

void clone_server::install_monitors()
{
	// establish monitor channels
	int result = zmq_socket_monitor((void *)*_publisher, PUBLISHER_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_responder, RESPONDER_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_collector, COLLECTOR_MON_ADDR, ZMQ_EVENT_ALL);
	assert(result != -1);

	// connect to monitor channels
	_pub_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	int linger_value = 0;
	_pub_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_pub_mon->connect(PUBLISHER_MON_ADDR);

	_resp_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_resp_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_resp_mon->connect(RESPONDER_MON_ADDR);

	_coll_mon = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_coll_mon->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_coll_mon->connect(COLLECTOR_MON_ADDR);
}

void clone_server::handle_monitor_events()
{
	zmqu::poller socks;
	size_t pub_mon_idx = socks.add(*_pub_mon, ZMQ_POLLIN);
	size_t resp_mon_idx = socks.add(*_resp_mon, ZMQ_POLLIN);
	size_t coll_mon_idx = socks.add(*_coll_mon, ZMQ_POLLIN);

	socks.try_poll();

	string addr;
	zmq_event_t event;

	if (socks.has_input(pub_mon_idx))
	{
		zmqu::recv(*_pub_mon, event, addr);
		socket_event(PUBLISHER, event, addr);
	}

	if (socks.has_input(resp_mon_idx))
	{
		zmqu::recv(*_resp_mon, event, addr);
		socket_event(RESPONDER, event, addr);
	}

	if (socks.has_input(coll_mon_idx))
	{
		zmqu::recv(*_coll_mon, event, addr);
		socket_event(COLLECTOR, event, addr);
	}
}

void clone_server::socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
{
	switch (e.event)
	{
		case ZMQ_EVENT_ACCEPTED:
			on_accepted(sid, addr);
			break;

		case ZMQ_EVENT_DISCONNECTED:
			on_disconnected(sid, addr);
			break;
	}

	on_socket_event(sid, e, addr);
}

void clone_server::free_zmq()
{
	// libzmq 4 suffers from hanging if zmq_socket_monitor() used, see https://github.com/zeromq/libzmq/issues/1279
	int result = zmq_socket_monitor((void *)*_collector, nullptr, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_responder, nullptr, ZMQ_EVENT_ALL);
	assert(result != -1);

	result = zmq_socket_monitor((void *)*_publisher, nullptr, ZMQ_EVENT_ALL);
	assert(result != -1);

	delete _coll_mon;
	delete _resp_mon;
	delete _pub_mon;
	delete _collector;
	delete _responder;
	delete _publisher;
}

}  // zmqu
