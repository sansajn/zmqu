#include <string>
#include <thread>
#include <gtest/gtest.h>
#include "zmqu/clone_client.hpp"

using std::string;

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
