#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <boost/noncopyable.hpp>
#include <zmq.hpp>
#include "json.hpp"

namespace zmq {

class probe
	: private boost::noncopyable
{
public:
	probe(char const * addr, std::shared_ptr<zmq::context_t> ctx);
	virtual ~probe();
	void recv();
	void * socket() {return (void *)_sock;}

	virtual void on_connected(char const * addr);
	virtual void on_connect_delayed(char const * addr);
	virtual void on_connect_retired(char const * addr);
	virtual void on_listening(char const * addr);
	virtual void on_bind_failed(char const * addr);
	virtual void on_accepted(char const * addr);
	virtual void on_accept_failed(char const * addr);
	virtual void on_closed(char const * addr);
	virtual void on_close_failed(char const * addr);
	virtual void on_disconnected(char const * addr);
	virtual void on_monitor_stopped(char const * addr);

	probe(probe && other);
	probe & operator=(probe && other);

private:
	std::shared_ptr<zmq::context_t> _ctx;
	zmq::socket_t * _sock;
};

class poller
{
public:
	void add(zmq::socket_t & sock, short revents = ZMQ_POLLIN);  //!< \param[in] revents use ZMQ_POLLIN, ZMQ_POLLOUT or ZMQ_POLLERR \sa zmq_poll()
	void add(probe & p);
	void poll(std::chrono::milliseconds timeout);
	bool has_input(size_t idx) const;
	bool has_output(size_t idx) const;
	bool has_event(size_t idx) const;
	short events(size_t idx) const;

private:
	std::vector<zmq_pollitem_t> _items;
};



std::string recv(zmq::socket_t & sock);
void recv(socket_t & sock, std::vector<std::string> & messages);
void recv_json(zmq::socket_t & sock, boost::property_tree::ptree & json);

void send(zmq::socket_t & sock, std::string const & buf, bool more = false);
void send(zmq::socket_t & sock, std::ostringstream & buf, bool more = false);
void send(zmq::socket_t & sock, char const * str, bool more = false);
void send(zmq::socket_t & sock, char const * buf, unsigned len, bool more = false);
void send(zmq::socket_t & sock, std::vector<std::string> const & messages);  //!< sends multipart message
void send_json(zmq::socket_t & sock, boost::property_tree::ptree & json);




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
