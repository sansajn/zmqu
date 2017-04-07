#include <string>
#include <thread>
#include <memory>
#include <gtest/gtest.h>
#include "zmqu/clone_client.hpp"

using std::string;
using std::shared_ptr;

struct dummy_client_subscribe_test : public zmq::clone_client
{
	string last_news;
	void on_news(std::string const & s) override {last_news = s;}
};

struct dummy_client_ask_test : public zmq::clone_client
{
	string last_answer;
	void on_answer(std::string const & s) override {last_answer = s;}
};

struct dummy_client_monitoring_test : public zmq::clone_client
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

TEST(clone_client_test, subscribe)
{
	zmq::context_t ctx;
	zmq::socket_t socket{ctx, ZMQ_PUB};
	socket.bind("tcp://*:5556");

	dummy_client_subscribe_test client;
	client.connect("localhost", 5556);
	zmq::mailbox client_mail = client.create_mailbox();
	std::thread client_thread{&dummy_client_subscribe_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	// send a news to client
	string expected{"hello jane!"};
	zmq::send(socket, expected);

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for client

	EXPECT_EQ(expected, client.last_news);

	client.quit(client_mail);  // quit client

	client_thread.join();
}

TEST(clone_client_test, ask)
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_ROUTER};
	responder.bind("tcp://*:5557");

	dummy_client_ask_test client;
	client.connect("localhost", 5556, 5557, 5558);
	zmq::mailbox client_mail = client.create_mailbox();
	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	string question{"who is there?"};
	string answer{"hello, teresa is there!"};

	client.ask(client_mail, question);

	// receive a question (identity, message)
	zmq::message_t identity;
	responder.recv(&identity);

	string message = zmq::recv(responder);
	EXPECT_EQ(question, message);

	// answer
	responder.send(identity, ZMQ_SNDMORE);
	zmq::send(responder, answer);

	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for client

	EXPECT_EQ(answer, client.last_answer);

	client.command(client_mail, "quit");
	client_thread.join();
}

TEST(clone_client_test, notify)
{
	zmq::context_t ctx;
	zmq::socket_t collector{ctx, ZMQ_PULL};  // server
	collector.bind("tcp://*:5558");

	dummy_client_ask_test client;
	client.connect("localhost", 5556, 5557, 5558);
	zmq::mailbox client_mail = client.create_mailbox();
	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	string notify_msg = "Teresa ready";
	client.notify(client_mail, notify_msg);

	string msg = zmq::recv(collector);

	EXPECT_EQ(notify_msg, msg);

	client.quit(client_mail);
	client_thread.join();
}

TEST(clone_client_test, monitoring)
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

	EXPECT_EQ(-1, client.subscriber_state);
	EXPECT_EQ(-1, client.requester_state);
	EXPECT_EQ(-1, client.notifier_state);

	std::thread client_thread{&dummy_client_ask_test::start, &client};  // run client
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	EXPECT_EQ(ZMQ_EVENT_CONNECTED, client.subscriber_state);
	EXPECT_EQ(ZMQ_EVENT_CONNECTED, client.requester_state);
	EXPECT_EQ(ZMQ_EVENT_CONNECTED, client.notifier_state);

	// quit
	zmq::mailbox client_mail = client.create_mailbox();
	client.quit(client_mail);

	client_thread.join();
}
