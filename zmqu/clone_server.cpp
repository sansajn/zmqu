#include <functional>
#include <iostream>
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
using std::call_once;

clone_server::clone_server()
	: clone_server(std::shared_ptr<zmq::context_t>{})
{}

clone_server::clone_server(std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
	, _publisher{nullptr}
	, _responder{nullptr}
	, _collector{nullptr}
	, _ready{false}
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


void clone_server::setup_and_run()
{
	if (!_ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};

	string common_address = string{"tcp://"} + _host + string{":"};

	_publisher = new zmq::socket_t{*_ctx, ZMQ_PUB};
	int linger_value = 0, sndhwm = 0;
	_publisher->setsockopt(ZMQ_LINGER, &linger_value, sizeof(int));
	_publisher->setsockopt(ZMQ_SNDHWM, &sndhwm, sizeof(int));
	_publisher->bind((common_address + to_string(_news_port)).c_str());

	{  // publisher info
		int rcvhwm = -1;
		size_t length = sizeof(int);
		_publisher->getsockopt(ZMQ_RCVHWM, &rcvhwm, &length);

		int sndhwm = -1;
		length = sizeof(int);
		_publisher->getsockopt(ZMQ_SNDHWM, &sndhwm, &length);
	}

	_responder = new zmq::socket_t{*_ctx, ZMQ_ROUTER};
	_responder->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_responder->bind((common_address + to_string(_answer_port)).c_str());

	_collector = new zmq::socket_t{*_ctx, ZMQ_PULL};
	_collector->setsockopt(ZMQ_LINGER, (void const *)&linger_value, sizeof(int));
	_collector->bind((common_address + to_string(_notification_port)).c_str());

	install_monitors();

	_ready = true;

	loop();

	_ready = false;

	free_zmq();
}

void clone_server::start()
{
	call_once(_running, std::bind(&clone_server::setup_and_run, this));
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

bool clone_server::ready() const
{
	return _ready;
}

void clone_server::loop()
{
	while (!_quit)
	{
		int err = zmq_errno();
		if (err > 0 && err != EAGAIN)
			std::cerr << "zmq: error (" << err << ") detected what: " << zmq_strerror(err) << std::endl;

		handle_monitor_events();

		// publish
		string news;
		for (int i = 100; i && _publisher_queue.try_pop(news); --i)
		{
			size_t bytes = zmqu::send(*_publisher, news);
			if (bytes == 0)
				std::cerr << "zmq: zero bytes send via zmq send() function" << std::endl;
		}

		vector<string> id_with_msg{2};
		while (zmqu::try_recv(*_responder, id_with_msg[0]))
		{
			zmqu::recv(*_responder, id_with_msg[1]);
			id_with_msg[1] = on_question(id_with_msg[1]);
			zmqu::send(*_responder, id_with_msg);
		}

		string feed;
		while (zmqu::try_recv(*_collector, feed))
			on_notify(feed);

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
	string addr;
	zmq_event_t event;

	if (zmqu::try_recv(*_pub_mon, event))
	{
		zmqu::recv(*_pub_mon, addr);
		socket_event(PUBLISHER, event, addr);
	}

	if (zmqu::try_recv(*_resp_mon, event))
	{
		zmqu::recv(*_resp_mon, addr);
		socket_event(RESPONDER, event, addr);
	}

	if (zmqu::try_recv(*_coll_mon, event))
	{
		zmqu::recv(*_coll_mon, addr);
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
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " collector error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_responder, nullptr, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " responder error" << std::endl;
		assert(0);
	}

	result = zmq_socket_monitor((void *)*_publisher, nullptr, ZMQ_EVENT_ALL);
	if (result == -1)
	{
		std::cerr << __PRETTY_FUNCTION__ << " publisher error" << std::endl;
		assert(0);
	}

	delete _coll_mon;
	delete _resp_mon;
	delete _pub_mon;
	delete _collector;
	delete _responder;
	delete _publisher;
}

}  // zmqu
