#include <string>
#include <thread>
#include <atomic>
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
	std::atomic_size_t news_count = 0;

	void on_news(string const & news) override
	{
		++news_count;
	}
};

namespace {

class dummy_server
{
private:
	zmq::context_t _ctx;  // needs to be at the beginning to force initialization order

public:
	zmq::socket_t publisher,
		responder,
		collector;

	dummy_server()
		: _ctx{}
		, publisher{_ctx, ZMQ_PUB}
		, responder{_ctx, ZMQ_ROUTER}
		, collector{_ctx, ZMQ_PULL}
	{}

	void bind()
	{
		publisher.bind("tcp://*:5556");
		responder.bind("tcp://*:5557");
		collector.bind("tcp://*:5558");
	}
};

}  // make private for this cpp file

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
	// SETUP

	dummy_server server;
	server.bind();

	// client
	async<dummy_client_monitoring_test> client;
	client.connect("localhost", 5556, 5557, 5558);

	REQUIRE_FALSE(client.connected());

	// ACT

	REQUIRE(client.run_sync());

	// CHECK

	REQUIRE(client.ready());
	REQUIRE(client.connected(dummy_client_monitoring_test::SUBSCRIBER));
	REQUIRE(client.connected(dummy_client_monitoring_test::REQUESTER));
	REQUIRE(client.connected(dummy_client_monitoring_test::NOTIFIER));
	REQUIRE(client.connected());

	// CLEANUP

	client.quit();
}

TEST_CASE("client can receive news",
	"[clone_client][receive]")
{
	// SETUP
	constexpr size_t NEWS_AMOUNT = 100;

	dummy_server server;
	server.bind();

	async<dummy_client_monitoring_test> client;
	client.connect("localhost", 5556, 5557, 5558);

	REQUIRE_FALSE(client.connected());

	// ACT

	REQUIRE(client.run_sync());

	for (size_t i = 0; i < NEWS_AMOUNT; ++i)
		zmqu::send(server.publisher, i);

	// CHECK

	REQUIRE(client.ready());
	REQUIRE(client.connected());
	REQUIRE(wait_event([&client]{return client.news_count == NEWS_AMOUNT;}, seconds{1}));

	// CLEANUP

	client.quit();
}
