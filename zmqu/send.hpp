#pragma once
#include <vector>
#include <string>
#include <zmq.hpp>
#include "json.hpp"

namespace zmq {

void send_json(zmq::socket_t & sock, jtree & json);


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

}  // zmq
