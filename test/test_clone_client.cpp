#include <string>
#include <thread>
#include <memory>
#include <chrono>
#include <catch.hpp>
#include "zmqu/send.hpp"
#include "zmqu/recv.hpp"
#include "zmqu/clone_client.hpp"
#include "zmqu/async.hpp"

using std::string;
using std::shared_ptr;
using std::chrono::seconds;
using zmqu::async;
using zmqu::wait_event;

struct dummy_client_subscribe_test : public zmqu::clone_client
{
	string last_news;
	void on_news(std::string const & s) override {last_news = s;}
};

struct dummy_client_ask_test : public zmqu::clone_client
{
	string last_answer;
	void on_answer(std::string const & s) override {last_answer = s;}
};

struct dummy_client_monitoring_test : public zmqu::clone_client
{
	int subscriber_state = -1;
	int requester_state = -1;
	int notifier_state = -1;

	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr)
	{
		switch (sid)
		{
			case SUBSCRIBER:
				subscriber_state = e.event;
				break;

			case REQUESTER:
				requester_state = e.event;
				break;

			case NOTIFIER:
				notifier_state = e.event;
				break;

			default: break;
		}
	}
};

TEST_CASE("clone client news (subscriber) channel",
	"[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t socket{ctx, ZMQ_PUB};
	socket.bind("tcp://*:5556");

	async<dummy_client_subscribe_test> client;
	client.connect("localhost", 5556, 5557, 5558);
	client.run();
	REQUIRE(wait_event([&client]{return client.ready();}, seconds{1}));

	// send a news to client
	string expected{"hello jane!"};
	zmqu::send_sync(socket, expected);

	REQUIRE(wait_event(
		[&client]{return !client.last_news.empty();}, seconds{1}));

	REQUIRE(client.last_news == expected);
}

TEST_CASE("clone client ask channel test",
	"[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_ROUTER};
	responder.bind("tcp://*:5557");

	async<dummy_client_ask_test> client;
	client.connect("localhost", 5556, 5557, 5558);
	client.run();
	REQUIRE(wait_event([&client]{return client.ready();}, seconds{1}));

	string question{"who is there?"};
	string answer{"hello, teresa is there!"};

	client.ask(question);

	// receive a question (identity, message)
	zmq::message_t identity;
	responder.recv(&identity);

	string message = zmqu::recv(responder);
	REQUIRE(message == question);

	// answer
	responder.send(identity, ZMQ_SNDMORE);
	zmqu::send_sync(responder, answer);

	REQUIRE(wait_event(
		[&client]{return !client.last_answer.empty();}, seconds{1}));

	REQUIRE(client.last_answer == answer);
}

TEST_CASE("clone client notify channel test",
	"[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t collector{ctx, ZMQ_PULL};  // server
	collector.bind("tcp://*:5558");

	async<dummy_client_ask_test> client;
	client.connect("localhost", 5556, 5557, 5558);
	client.run();
	REQUIRE(wait_event([&client]{return client.ready();}, seconds{1}));

	string notify_msg = "Teresa ready";
	client.notify(notify_msg);

	string msg = zmqu::recv(collector);

	REQUIRE(msg == notify_msg);
}

TEST_CASE("clone client socket events monitoring test",
	"[clone_client]")
{
	// dummy server
	shared_ptr<zmq::context_t> ctx{new zmq::context_t};
	zmq::socket_t publisher_socket{*ctx, ZMQ_PUB};  // publisher
	publisher_socket.bind("tcp://*:5556");

	zmq::socket_t responder_socket{*ctx, ZMQ_ROUTER};  // responder
	responder_socket.bind("tcp://*:5557");

	zmq::socket_t collector_socket{*ctx, ZMQ_PULL};  // collector
	collector_socket.bind("tcp://*:5558");

	// client
	async<dummy_client_monitoring_test> client;
	client.connect("localhost", 5556, 5557, 5558);

	REQUIRE(client.subscriber_state == -1);
	REQUIRE(client.requester_state == -1);
	REQUIRE(client.notifier_state == -1);

	client.run();
	REQUIRE(wait_event([&client]{return client.ready();}, seconds{1}));

	REQUIRE(client.subscriber_state == ZMQ_EVENT_CONNECTED);
	REQUIRE(client.requester_state == ZMQ_EVENT_CONNECTED);
	REQUIRE(client.notifier_state == ZMQ_EVENT_CONNECTED);

	client.quit();
}
