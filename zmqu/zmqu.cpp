#include "zmqu.hpp"
#include <string>
#include <boost/property_tree/json_parser.hpp>

namespace zmq {

using std::string;
using std::stringstream;
namespace pt = boost::property_tree;


probe::probe(char const * addr, std::shared_ptr<zmq::context_t> ctx)
	: _ctx{ctx}
{
	if (!ctx)
		_ctx = std::shared_ptr<zmq::context_t>{new zmq::context_t{}};

	_sock = new zmq::socket_t{*_ctx, ZMQ_PAIR};
	_sock->connect(addr);
}

probe::~probe()
{
	delete _sock;
}

probe::probe(probe && other)
	: _sock{other._sock}
{
	other._sock = nullptr;
}

probe & probe::operator=(probe && other)
{
	std::swap(_sock, other._sock);
	return *this;
}

void probe::recv()
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	int rc = zmq_msg_recv(&msg, (void *)*_sock, 0);
	assert(rc != -1);
	assert(zmq_msg_more(&msg));
	uint16_t event = *(uint16_t *)zmq_msg_data(&msg);

	rc = zmq_msg_recv(&msg, (void *)*_sock, 0);
	assert(rc != -1);
	assert(!zmq_msg_more(&msg));

	switch (event)
	{
		case ZMQ_EVENT_CONNECTED:
			on_connected((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_CONNECT_DELAYED:
			on_connect_delayed((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_CONNECT_RETRIED:
			on_connect_retired((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_LISTENING:
			on_listening((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_BIND_FAILED:
			on_bind_failed((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_ACCEPTED:
			on_accepted((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_ACCEPT_FAILED:
			on_accept_failed((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_CLOSED:
			on_closed((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_CLOSE_FAILED:
			on_close_failed((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_DISCONNECTED:
			on_disconnected((char const *)zmq_msg_data(&msg));
			break;

		case ZMQ_EVENT_MONITOR_STOPPED:
			on_monitor_stopped((char const *)zmq_msg_data(&msg));
			break;

		default:
			assert(0 && "unknown event");
			break;
	}

	zmq_close(&msg);
}

void probe::on_connected(char const * addr)
{}

void probe::on_connect_delayed(char const * addr)
{}

void probe::on_connect_retired(char const * addr)
{}

void probe::on_listening(char const * addr)
{}

void probe::on_bind_failed(char const * addr)
{}

void probe::on_accepted(char const * addr)
{}

void probe::on_accept_failed(char const * addr)
{}

void probe::on_closed(char const * addr)
{}

void probe::on_close_failed(char const * addr)
{}

void probe::on_disconnected(char const * addr)
{}

void probe::on_monitor_stopped(char const * addr)
{}


void poller::add(zmq::socket_t & sock, short revents)
{
	_items.push_back(zmq_pollitem_t{(void *)sock, 0, revents});
}

void poller::add(probe & p)
{
	_items.push_back(zmq_pollitem_t{p.socket(), 0, ZMQ_POLLIN});
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


std::string recv(zmq::socket_t & sock)
{
	zmq::message_t msg;
	sock.recv(&msg);
	return string{static_cast<char const *>(msg.data()), msg.size()};
}

void recv(zmq::socket_t & sock, std::string & s)
{
	zmq::message_t msg;
	sock.recv(&msg);
	s.assign(static_cast<char const *>(msg.data()), msg.size());
}

void recv_json(zmq::socket_t & sock, pt::ptree & json)
{
	zmq::message_t msg;
	sock.recv(&msg);
	string s(static_cast<char const *>(msg.data()), msg.size());
	stringstream ss{s};
	pt::read_json(ss, json);
}

void recv(socket_t & sock, std::vector<std::string> & messages)
{
	zmq::message_t msg;
	int more = 1;
	size_t more_size = sizeof(more);

	while (more)
	{
		sock.recv(&msg);
		messages.emplace_back(static_cast<char const *>(msg.data()), msg.size());
		sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
	}
}

void send(zmq::socket_t & sock, std::string const & buf, bool more)
{
    send(sock, buf.c_str(), buf.size(), more);
}

void send(zmq::socket_t & sock, std::ostringstream & buf, bool more)
{
    send(sock, buf.str(), more);
}

void send(zmq::socket_t & sock, char const * str, bool more)
{
    send(sock, str, strlen(str), more);
}

void send(zmq::socket_t & sock, char const * buf, unsigned len, bool more)
{
	zmq::message_t msg{len};
	memcpy(msg.data(), buf, len);
	sock.send(msg, more ? ZMQ_SNDMORE : 0);
}

void send_json(zmq::socket_t & sock, boost::property_tree::ptree & json)
{
	stringstream ss;
	pt::write_json(ss, json);
	string s = ss.str();
	zmq::message_t msg{s.size()};
	memcpy(msg.data(), s.data(), s.size());
	sock.send(msg);
}

void send(zmq::socket_t & sock, std::vector<std::string> const & messages)
{
	for (size_t i = 0; i < messages.size()-1; ++i)
		zmq::send(sock, messages[i], true);
	zmq::send(sock, messages.back());
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
