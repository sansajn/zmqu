#pragma once
#include <vector>
#include <string>
#include <zmq.hpp>

namespace zmqu {

std::string recv(zmq::socket_t & sock);

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
	memcpy(&e, msg.data(), sizeof(e));
}

void recv_one(zmq::socket_t & sock, std::vector<std::string> & vec);  // TODO: expand to general T not only string


//! receive one single data (blocking mode)
template <typename T>
inline void recv(zmq::socket_t & sock, T & t)
{
	recv_one(sock, t);
}

//! receive multiple heterogeneous data (blocking mode)
template <typename T, typename ... Args>
inline void recv(zmq::socket_t & sock, T & t, Args & ... args)
{
	recv_one(sock, t);
	recv(sock, args ...);
}


// non blocking receive support

template <typename T>
inline bool recv_one(zmq::socket_t & sock, T & t, int flags)  //!< generic POD receive support
{
	zmq::message_t msg;
	bool ret = sock.recv(&msg, flags);
	if (ret)
	{
		t = *(T *)msg.data();
		assert(msg.size() == sizeof(T) && "message size not match");
	}
	return ret;
}

template <>
inline bool recv_one<std::string>(zmq::socket_t & sock, std::string & s, int flags)
{
	zmq::message_t msg;
	bool ret = sock.recv(&msg, flags);
	if (ret)
		s.assign(static_cast<char const *>(msg.data()), msg.size());
	return ret;
}

template <>
inline bool recv_one<zmq_event_t>(zmq::socket_t & sock, zmq_event_t & e, int flags)
{
	zmq::message_t msg;
	bool ret = sock.recv(&msg, flags);
	if (ret)
		memcpy(&e, msg.data(), sizeof(e));
	return ret;
}

//! non blocking receive
template <typename T>
inline bool try_recv(zmq::socket_t & sock, T & t)
{
	return recv_one(sock, t, 0|ZMQ_DONTWAIT);
}

}  // zmq
