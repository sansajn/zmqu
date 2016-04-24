#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <boost/noncopyable.hpp>
#include <zmq.hpp>
#include "json.hpp"

namespace zmq {

class poller
{
public:
	void add(zmq::socket_t & sock, short revents = ZMQ_POLLIN);  //!< \param[in] revents use ZMQ_POLLIN, ZMQ_POLLOUT or ZMQ_POLLERR \sa zmq_poll()
	void poll(std::chrono::milliseconds timeout);
	bool has_input(size_t idx) const;
	bool has_output(size_t idx) const;
	bool has_event(size_t idx) const;
	short events(size_t idx) const;
	size_t size() const {return _items.size();}

private:
	std::vector<zmq_pollitem_t> _items;
};

class mailbox
	: private boost::noncopyable
{
public:
	mailbox(std::string const & uri, std::shared_ptr<zmq::context_t> ctx);
	~mailbox();
	void send(char const * command, std::string const & message);  // TODO: not a general (implement varargs template there)

	mailbox(mailbox && rhs);
	void operator=(mailbox && rhs);

private:
	zmq::socket_t * _inproc;
	// TODO: should I store context to prevent its deletion ?
};


std::string recv(zmq::socket_t & sock);
void recv_json(zmq::socket_t & sock, jtree & json);
void send_json(zmq::socket_t & sock, jtree & json);

std::string event_to_string(int event);


// variable argument receive implementation

template <typename T>
inline void recv_one(zmq::socket_t & sock, T & t)  //!< generic POD receive support
{
	zmq::message_t msg;
	sock.recv(&msg);
	assert(msg.size() == sizeof(T) && "message size not match");
	t = *(T *)msg.data();
}

template <>
inline void recv_one<std::string>(zmq::socket_t & sock, std::string & s)
{
	zmq::message_t msg;
	sock.recv(&msg);
	s.assign(static_cast<char const *>(msg.data()), msg.size());
}

template <>
inline void recv_one<zmq_event_t>(zmq::socket_t & sock, zmq_event_t & e)
{
	zmq::message_t msg;
	sock.recv(&msg);
	assert(msg.more());
	void * p = msg.data();
	e.event = *(uint16_t *)p;
	e.value = *(uint32_t *)((uint8_t *)p + 2);
}

void recv_one(zmq::socket_t & sock, std::vector<std::string> & vec);  // TODO: expand to general T not only string


template <typename T>
inline void recv(zmq::socket_t & sock, T & t)
{
	recv_one(sock, t);
}

//! multipart message receive for heterogeneous data
template <typename T, typename ... Args>
inline void recv(zmq::socket_t & sock, T & t, Args & ... args)
{
	recv_one(sock, t);
	recv(sock, args ...);
}


// variable argument send implementation

template <typename T>
inline void send_one(zmq::socket_t & sock, T const & t, bool more)  //!< generic POD send support
{
	sock.send(static_cast<void const *>(&t), sizeof(T), more ? ZMQ_SNDMORE : 0);
}

template <>
inline void send_one<std::string>(zmq::socket_t & sock, std::string const & s, bool more)
{
	sock.send(s.data(), s.size(), more ? ZMQ_SNDMORE : 0);
}

inline void send_one(zmq::socket_t & sock, char const * str, bool more)
{
	sock.send(str, strlen(str), more ? ZMQ_SNDMORE : 0);
}

inline void send_one(zmq::socket_t & sock, char * str, bool more)
{
	sock.send(str, strlen(str), more ? ZMQ_SNDMORE : 0);
}

template <typename T>
inline void send_one(zmq::socket_t & sock, std::vector<T> const & vec, bool more)
{
	for (size_t i = 0; i < vec.size()-1; ++i)
		send_one(sock, vec[i], true);
	send_one(sock, vec.back(), more ? ZMQ_SNDMORE : 0);
}


template <typename T>
inline void send(zmq::socket_t & sock, T const & t)
{
	send_one(sock, t, false);
}

//! multipart message send for heterogeneous data
template <typename T, typename ... Args>
inline void send(zmq::socket_t & sock, T const & t, Args const & ... args)
{
	send_one(sock, t, true);
	send(sock, args ...);
}


template <typename T>
inline void send_multipart(zmq::socket_t & sock, T const & t)
{
	send_one(sock, t, true);
}

}   // zmq
