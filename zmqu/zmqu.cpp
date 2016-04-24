#include "zmqu.hpp"
#include <string>
#include <utility>
#include <boost/property_tree/json_parser.hpp>

namespace zmq {

using std::swap;
using std::string;
using std::stringstream;
namespace pt = boost::property_tree;


void poller::add(zmq::socket_t & sock, short revents)
{
	_items.push_back(zmq_pollitem_t{(void *)sock, 0, revents});
}

void poller::poll(std::chrono::milliseconds timeout)
{
	int rc = zmq_poll(const_cast<zmq_pollitem_t *>(_items.data()), _items.size(), timeout.count());
	if (rc == -1)
		throw std::runtime_error{"zmq_poll() failed"};
}

bool poller::has_input(size_t idx) const  // has_input
{
	return events(idx) & ZMQ_POLLIN;
}

bool poller::has_output(size_t idx) const
{
	return events(idx) & ZMQ_POLLOUT;
}

bool poller::has_event(size_t idx) const
{
	return events(idx) & ZMQ_POLLERR;
}

short poller::events(size_t idx) const
{
	return _items[idx].revents;
}


mailbox::mailbox(std::string const & uri, std::shared_ptr<zmq::context_t> ctx)
{
	_inproc = new zmq::socket_t{*ctx, ZMQ_PAIR};
	_inproc->connect(uri.c_str());
}

mailbox::mailbox(mailbox && rhs)
	: _inproc{rhs._inproc}
{
	rhs._inproc = nullptr;
}

void mailbox::operator=(mailbox && rhs)
{
	swap(_inproc, rhs._inproc);
}

mailbox::~mailbox()
{
	delete _inproc;
}

void mailbox::send(char const * command, std::string const & message)
{
	zmq::send(*_inproc, command, message);
}


std::string recv(zmq::socket_t & sock)
{
	zmq::message_t msg;
	sock.recv(&msg);
	return string{static_cast<char const *>(msg.data()), msg.size()};
}

void recv_json(zmq::socket_t & sock, jtree & json)
{
	zmq::message_t msg;
	sock.recv(&msg);
	string s(static_cast<char const *>(msg.data()), msg.size());
	stringstream ss{s};
	pt::read_json(ss, json);
}

void recv_one(zmq::socket_t & sock, std::vector<std::string> & vec)
{
	zmq::message_t msg;

	do
	{
		sock.recv(&msg);
		vec.emplace_back(static_cast<char const *>(msg.data()), msg.size());
	}
	while (msg.more());
}


void send_json(zmq::socket_t & sock, jtree & json)
{
	stringstream ss;
	pt::write_json(ss, json);
	string s = ss.str();
	zmq::message_t msg{s.size()};
	memcpy(msg.data(), s.data(), s.size());
	sock.send(msg);
}


string event_to_string(int event)
{
	switch (event)
	{
		case ZMQ_EVENT_CONNECTED: return "connected";
		case ZMQ_EVENT_CONNECT_DELAYED: return "connect delayed";
		case ZMQ_EVENT_CONNECT_RETRIED: return "connect retried";
		case ZMQ_EVENT_LISTENING: return "listenning";
		case ZMQ_EVENT_BIND_FAILED: return "bind failed";
		case ZMQ_EVENT_ACCEPTED: return "accepted";
		case ZMQ_EVENT_ACCEPT_FAILED: return "accept failed";
		case ZMQ_EVENT_CLOSED: return "closed";
		case ZMQ_EVENT_CLOSE_FAILED: return "close failed";
		case ZMQ_EVENT_DISCONNECTED: return "disconnected";
		case ZMQ_EVENT_MONITOR_STOPPED: return "monitor stopped";
		default: return "unknown";
	}
}

}   // zmq
