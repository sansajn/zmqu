#include <string>
#include <thread>
#include <iostream>
#include <zmqu/clone_client.hpp>

using std::cout;
using std::string;

struct news_client_impl : public zmqu::clone_client
{
	void on_news(string const & news) override
	{
		cout << news << std::endl;
	}
};

struct ask_client_impl : public zmqu::clone_client
{
	void on_answer(string const & answer) override
	{
		cout << answer << std::endl;
	}
};

struct monitor_client_impl : public zmqu::clone_client
{
	void on_connected(socket_id sid, string const & addr) override
	{
		string sock_name;
		switch (sid)
		{
			case SUBSCRIBER: sock_name = "subscriber"; break;
			case REQUESTER: sock_name = "requester"; break;
			case NOTIFIER: sock_name = "notifier"; break;
		}

		cout << sock_name << " socket connected to " << addr << std::endl;
	}
};

void test_news()
{
	cout << "listenning ..." << std::endl;

	news_client_impl client;
	client.connect("localhost", 8876, 8877, 8878);

	std::thread t{&news_client_impl::start, &client};
	t.join();
}

void test_ask()
{
	ask_client_impl client;
	client.connect("localhost", 8876, 8877, 8878);

	std::thread t{&ask_client_impl::start, &client};
	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	client.ask(R"({"Cmd":"RequestApiVersion"})");

	t.join();
}

void test_notify()
{
	zmqu::clone_client client;
	client.connect("localhost", 8876, 8877, 8878);

	std::thread t{&zmqu::clone_client::start, &client};
	std::this_thread::sleep_for(std::chrono::milliseconds{100});

	client.notify(R"({"Cmd":["clone_client", "there"]})");

	t.join();
}

void test_monitor()
{
	cout << "waiting ..." << std::endl;

	monitor_client_impl client;
	client.connect("localhost", 8876, 8877, 8878);

	std::thread t{&monitor_client_impl::start, &client};
	t.join();
}


int main(int argc, char * argv[])
{
	test_monitor();
	return 0;
}
