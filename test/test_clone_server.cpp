// tests for client and server interaction
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <catch.hpp>
#include "zmqu/clone_server.hpp"
#include "zmqu/clone_client.hpp"

using std::mutex;
using std::atomic_bool;
using std::string;
using std::cout;

class dummy_server : public zmqu::clone_server
{
public:
	using lock_guard = std::lock_guard<std::mutex>;

	atomic_bool accepted;

	dummy_server() : accepted{false}
	{}

	string notification_value()
	{
		lock_guard lock{_mtx};
		return _notification;
	}

	string on_question(string const & q) override
	{
		lock_guard lock{_mtx};
		_question = q;
		if (q == "who else?")
			return "Teresa Lisbon";
		else
			return string{"no idea"};
	}

	void on_notify(string const & s) override
	{
		lock_guard lock{_mtx};
		_notification = s;
	}

	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override
	{
		if (sid == PUBLISHER && e.event == ZMQ_EVENT_ACCEPTED)
			accepted = true;
	}

private:
	string _question;
	string _notification;
	mutex _mtx;
};


TEST_CASE("clone server can publish news", "[clone_server]")
{
	dummy_server serv;
	serv.bind(5556, 5557, 5558);
	std::thread serv_thread{&dummy_server::start, &serv};
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	zmq::context_t ctx;
	zmq::socket_t sub{ctx, ZMQ_SUB};
	sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	sub.connect("tcp://localhost:5556");

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for connect, otherwise next one recv will block

	// publish news
	serv.publish("Patric Jane");

	zmq::message_t msg{30};
	sub.recv(&msg);
	string news{msg.data<char>(), msg.size()};

	REQUIRE(news == "Patric Jane");

	serv.quit();

	serv_thread.join();
}

TEST_CASE("clone server can answer questions", "[clone_server]")
{
	dummy_server serv;
	serv.bind(5556, 5557, 5558);
	std::thread serv_thread{&dummy_server::start, &serv};

	// client
	zmq::context_t ctx;
	zmq::socket_t req{ctx, ZMQ_DEALER};
	req.setsockopt<int>(ZMQ_LINGER, 0);
	req.connect("tcp://localhost:5557");
	assert(req.connected());

	string question = "who else?";
	req.send(question.c_str(), question.size());

	zmq::message_t msg{30};
	req.recv(&msg);
	string answer{msg.data<char>(), msg.size()};

	REQUIRE(answer == "Teresa Lisbon");

	serv.quit();
	serv_thread.join();
}

TEST_CASE("clone server can be also notified", "[clone_server]")
{
	dummy_server serv;
	serv.bind(5556, 5557, 5558);
	std::thread serv_thread{&dummy_server::start, &serv};

	// client
	zmq::context_t ctx;
	zmq::socket_t notif{ctx, ZMQ_PUSH};
	notif.connect("tcp://localhost:5558");
	assert(notif.connected());

	string notification{"Patric Jane is there!"};
	notif.send(notification.c_str(), notification.size());

	std::this_thread::sleep_for(std::chrono::milliseconds{200});  // wait ZMQ to deliver a message

	REQUIRE(serv.notification_value() == "Patric Jane is there!");

	serv.quit();
	serv_thread.join();
}
