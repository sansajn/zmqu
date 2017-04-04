// use of zmq::clone_server with a monitor channel
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>
#include <iostream>
#include <cassert>
#include <ctime>
#include "zmqu/zmqu.hpp"
#include "zmqu/clone_server.hpp"

using std::cout;
using std::string;
using std::to_string;
using std::vector;

using hres_clock = std::chrono::high_resolution_clock;

void dump_clients(std::map<unsigned, string> const & clients);


struct dummy_server : public zmq::clone_server
{
	dummy_server();

	void idle() override;

	string on_question(string const & question) override;
	void on_notify(string const & s) override;
	void on_socket_event(socket_id sid, zmq_event_t const & e, string const & addr) override;

	std::map<unsigned, string> clients;  // (id, last-heart-beat-timestamp)
	hres_clock::time_point set_pos_vect_at;

	// random generators
	std::default_random_engine drand;
	std::uniform_real_distribution<double> lat_rand_distr;
	std::uniform_real_distribution<double> lon_rand_distr;
};


dummy_server::dummy_server()
	: drand(time(0))
	, lat_rand_distr{-90.0, 90.0}
	, lon_rand_distr{-180.0, 180.0}
{
	set_pos_vect_at = hres_clock::now() + std::chrono::seconds{5};
}

string dummy_server::on_question(string const & question)
{
	jtree json;
	to_json(question, json);

	string cmd = json.get<string>("Cmd");
	if (cmd == "RequestApiVersion")
	{
		jtree answer;
		answer.put("Response", "Ok");
		answer.put("Source", "PlayerManager");
		answer.put("ApiVersion", "v1-0-0");
		return to_string(answer);
	}

	return string{};
}

void dummy_server::on_notify(string const & s)
{
	jtree json;
	to_json(s, json);

	string cmd = json.get<string>("Cmd");
	if (cmd == "RequestForRegistration")
	{
		unsigned id = json.get<unsigned>("WhoAmI");
		clients[id] = "";
		cout << "client '" << id << "' registered" << std::endl;
		dump_clients(clients);
	}
	else if (cmd == "HeartBeat")
	{
		unsigned client_id = json.get<unsigned>("WhoAmI");
		clients[client_id] = json.get<string>("UtcTimestamp");
		cout << "heart-beat from '" << client_id << "' received" << std::endl;
	}
}

void dummy_server::on_socket_event(socket_id sid, zmq_event_t const & e, string const & addr)
{
	string sock_name;
	switch (sid)
	{
		case PUBLISHER:
			sock_name = "publisher";
			break;

		case RESPONDER:
			sock_name = "responder";
			break;

		case COLLECTOR:
			sock_name = "collector";
			break;
	}

	cout << "event: socket:" << sock_name << ", event:" << zmq::event_to_string(e.event) << ", addr:" << addr << std::endl;
}


void dummy_server::idle()
{
	if (set_pos_vect_at < hres_clock::now())
	{
		double lat = lat_rand_distr(drand), lon = lon_rand_distr(drand);

		jtree json;
		json.put("Cmd", "SetPositionVector");
		json.put("lat", lat);
		json.put("lon", lon);
		publish_internal(to_string(json));

		set_pos_vect_at= hres_clock::now() + std::chrono::seconds{5};
	}
}


int main(int argc, char * argv[])
{
	dummy_server server;
	server.bind(5556);
	zmq::mailbox server_mailbox = server.create_mailbox();

	std::thread server_thread{&dummy_server::start, &server};
	std::this_thread::sleep_for(std::chrono::milliseconds{10});  // wait for thread

	server.install_monitors(server_mailbox);

	server_thread.join();

	return 0;
}

void dump_clients(std::map<unsigned, string> const & clients)
{
	cout << "clients:\n";
	unsigned count = 0;
	for (auto const & client : clients)
		cout << "  #" << count++ << ":" << client.first << "\n";
}
