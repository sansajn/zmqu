// tests for client and server interaction
#include <string>
#include <thread>
#include <iostream>
#include <gtest/gtest.h>
#include "zmqu/clone_server.hpp"
#include "zmqu/clone_client.hpp"

using std::string;
using std::cout;

struct dummy_server : public zmqu::clone_server
{
	string question;
	bool accepted = false;

	string on_question(string const & q) override
	{
		question = q;
		if (q == "who else?")
			return "Teresa Lisbon";
		else
			return string{"no idea"};
	}

	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override
	{
		if (sid == PUBLISHER && e.event == ZMQ_EVENT_ACCEPTED)
			accepted = true;
	}
};

struct dummy_client : public zmqu::clone_client
{
	string news, answer;
	bool connected = false;

	void on_news(std::string const & s) override {news = s;}
	void on_answer(string const & s) override {answer = s;}

	void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr) override
	{
		if (sid == SUBSCRIBER && e.event == ZMQ_EVENT_CONNECTED)
			connected = true;
	}
};

TEST(cclient_cserv_test, basic)
{
	dummy_server serv;
	serv.bind(5556);
	std::thread serv_thread{&dummy_server::start, &serv};
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	dummy_client client;
	client.connect("localhost", 5556);
	std::thread client_thread{&dummy_client::start, &client};
	std::this_thread::sleep_for(std::chrono::milliseconds{10});

	// publish news
	zmqu::mailbox serv_mail = serv.create_mailbox();
	serv.publish(serv_mail, "Patric Jane");

	// ask server
	zmqu::mailbox client_mail = client.create_mailbox();
	client.ask(client_mail, "who else?");

	std::this_thread::sleep_for(std::chrono::milliseconds{100});  // wait for zmq

	EXPECT_TRUE(serv.accepted);
	EXPECT_TRUE(client.connected);
	EXPECT_EQ("Patric Jane", client.news);
	EXPECT_EQ("who else?", serv.question);
	EXPECT_EQ("Teresa Lisbon", client.answer);

	// quit
	client.quit(client_mail);
	serv.quit(serv_mail);

	serv_thread.join();
	client_thread.join();
}
