#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include <catch.hpp>
#include <zmqu/clone_client.hpp>
#include <zmqu/clone_server.hpp>

using std::string;
using std::vector;

struct collector_client_impl : public zmqu::clone_client
{
	vector<string> feeds;

	void on_news(string const & feed) override
	{
		feeds.push_back(feed);
	}
};

TEST_CASE("publisher/subscriber scenario", "[clone_client, clone_server]")
{
	zmqu::clone_server serv;
	serv.bind("*", 5555, 5556, 5557);
	std::thread serv_thread{&zmqu::clone_server::start, &serv};

	collector_client_impl client;
	client.connect("localhost", 5555, 5556, 5557);
	std::thread client_thread{&collector_client_impl::start, &client};

	std::this_thread::sleep_for(std::chrono::milliseconds{50});  // wait for ZMQ

	serv.publish("Patric Jane");
	serv.publish("Teresa Lisbon");
	serv.publish("Red John");

	std::this_thread::sleep_for(std::chrono::milliseconds{500});

	REQUIRE(client.feeds.size() == 3);
	REQUIRE(client.feeds[0] == "Patric Jane");
	REQUIRE(client.feeds[1] == "Teresa Lisbon");
	REQUIRE(client.feeds[2] == "Red John");

	client.quit();
	serv.quit();

	client_thread.join();
	serv_thread.join();
}
