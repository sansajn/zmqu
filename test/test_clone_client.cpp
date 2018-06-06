#include <string>
#include <thread>
#include <memory>
#include <catch.hpp>
#include <zmqu/send.hpp>
#include <zmqu/recv.hpp>
#include <zmqu/clone_client.hpp>

using std::string;
using std::shared_ptr;

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
		}
	}
};

TEST_CASE("clone client news (subscriber) channel", "[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t socket{ctx, ZMQ_PUB};
	socket.bind("tcp://*:5556");

	dummy_client_subscribe_test client;
	client.connect("localhost", 5556, 5557, 5558);
	std::thread client_thread{&dummy_client_subscribe_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	// send a news to client
	string expected{"hello jane!"};
	zmqu::send(socket, expected);

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for client

	REQUIRE(client.last_news == expected);

	client.quit();  // quit client

	client_thread.join();
}

TEST_CASE("clone client ask channel test", "[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_ROUTER};
	responder.bind("tcp://*:5557");

	dummy_client_ask_test client;
	client.connect("localhost", 5556, 5557, 5558);
	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

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
	zmqu::send(responder, answer);

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for client

	REQUIRE(client.last_answer == answer);

	client.quit();
	client_thread.join();
}

TEST_CASE("clone client notify channel test", "[clone_client]")
{
	zmq::context_t ctx;
	zmq::socket_t collector{ctx, ZMQ_PULL};  // server
	collector.bind("tcp://*:5558");

	dummy_client_ask_test client;
	client.connect("localhost", 5556, 5557, 5558);
	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	string notify_msg = "Teresa ready";
	client.notify(notify_msg);

	string msg = zmqu::recv(collector);

	REQUIRE(msg == notify_msg);

	client.quit();
	client_thread.join();
}

TEST_CASE("clone client socket events test", "[clone_client]")
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
	dummy_client_monitoring_test client;
	client.connect("localhost", 5556, 5557, 5558);

	REQUIRE(client.subscriber_state == -1);
	REQUIRE(client.requester_state == -1);
	REQUIRE(client.notifier_state == -1);

	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{100});  // wait for thread

	REQUIRE(client.subscriber_state == ZMQ_EVENT_CONNECTED);
	REQUIRE(client.requester_state == ZMQ_EVENT_CONNECTED);
	REQUIRE(client.notifier_state == ZMQ_EVENT_CONNECTED);

	client.quit();
	client_thread.join();
}
