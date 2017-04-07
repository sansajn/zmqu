// clone pattern abstarction
// clonecli3.py script implementation based on zmqpp
#include <thread>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <sstream>
#include <random>
#include <memory>
#include <iostream>
#include <ctime>
#include <sys/time.h>
#include "zmqu/zmqu.hpp"
#include "zmqu/clone_client.hpp"

using std::cout;
using std::string;
using std::to_string;
using std::make_pair;

using hres_clock = std::chrono::high_resolution_clock;

struct dummy_client : public zmq::clone_client
{
	dummy_client();

	void idle() override;

	void on_news(string const & s) override;
	void on_answer(string const & s) override;
	void on_socket_event(socket_id sid, zmq_event_t const & e, string const & addr) override;

	std::vector<std::pair<double, double>> route;
	hres_clock::time_point heart_beat_at;
	string whoami;
	std::default_random_engine drand;
};

dummy_client::dummy_client() : drand(time(0))
{
	heart_beat_at = hres_clock::now() + std::chrono::seconds{5};
	whoami = to_string(drand());
}

void dummy_client::idle()
{
	// generate heart-beat
	if (heart_beat_at < hres_clock::now())
	{
		string utc_time_stamp;

		// TODO: linux only solution (not sure if usec part is correct), use boost::date_time instead
		{
			timeval tm;
			gettimeofday(&tm, nullptr);
			char str[1024];
			strftime(str, 1024, "%Y-%m-%d-%H-%M-%S", gmtime(&tm.tv_sec));
			utc_time_stamp = string{str} + string{"-"} + to_string(tm.tv_usec);
		}

		jtree heartbeat_cmd;
		heartbeat_cmd.put("Cmd", "HeartBeat");
		heartbeat_cmd.put("UtcTimestamp", utc_time_stamp);
		heartbeat_cmd.put("WhoAmI", whoami);
		notify_internal(to_string(heartbeat_cmd));

		heart_beat_at = hres_clock::now() + std::chrono::seconds{5};
	}
}

void dummy_client::on_news(string const & s)
{
	jtree json;
	to_json(s, json);

	string cmd = json.get<string>("Cmd");
	if (cmd == "SetPositionVector")
	{
		double lat = json.get<double>("lat");
		double lon = json.get<double>("lon");
		route.push_back(make_pair(lat, lon));
		cout << "position (lat:" << lat << ", lon:" << lon << ") received" << std::endl;
	}
}


void dummy_client::on_answer(string const & s)
{
	jtree json;
	to_json(s, json);
	string ver = json.get<string>("ApiVersion");
	cout << "server api version: " << ver << std::endl;
}

void dummy_client::on_socket_event(socket_id sid, zmq_event_t const & e, string const & addr)
{
	string sock_name;
	switch (sid)
	{
		case SUBSCRIBER:
			sock_name = "subscriber";
			break;

		case REQUESTER:
			sock_name = "requester";
			break;

		case NOTIFIER:
			sock_name = "notifier";
			break;
	}

	cout << "event: socket:" << sock_name << ", event:" << zmq::event_to_string(e.event) << ", addr:" << addr << std::endl;
}


int main(int argc, char * argv[])
{
	dummy_client client;
	client.connect("localhost", 5556);  // ports 5556, 5557 and 5558 will be used
	zmq::mailbox client_mailbox = client.create_mailbox();

	std::thread client_thread{&dummy_client::start, &client};
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	std::default_random_engine drand(time(0));
	string whoami = to_string(drand());

	// login
	jtree reg_cmd;
	reg_cmd.put("Cmd", "RequestForRegistration");
	reg_cmd.put("WhoAmI", whoami);
	client.notify(client_mailbox,  to_string(reg_cmd));

	// ask for api version
	client.ask(client_mailbox, R"({"Cmd":"RequestApiVersion"})");

	client_thread.join();

	return 0;
}
