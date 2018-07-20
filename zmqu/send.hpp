#pragma once
#include <vector>
#include <string>
#include <zmq.hpp>
#include "json.hpp"

namespace zmqu {

void send_json(zmq::socket_t & sock, jtree & json);


// type aware send specializations
template <typename T>
inline size_t send_one(zmq::socket_t & sock, T const & t, int flags)  //!< generic POD send support
{
	return sock.send(static_cast<void const *>(&t), sizeof(T), flags);
}

template <>
inline size_t send_one<std::string>(zmq::socket_t & sock, std::string const & s, int flags)
{
	return sock.send(s.data(), s.size(), flags);
}

inline size_t send_one(zmq::socket_t & sock, char const * str, int flags)
{
	return sock.send(str, strlen(str), flags);
}

inline size_t send_one(zmq::socket_t & sock, char * str, int flags)
{
	return sock.send(str, strlen(str), flags);
}

template <typename T>
inline size_t send_one(zmq::socket_t & sock, std::vector<T> const & vec, int flags)
{
	size_t bytes = 0;
	for (size_t i = 0; i < vec.size()-1; ++i)
		bytes += send_one(sock, vec[i], flags|ZMQ_SNDMORE);
	bytes += send_one(sock, vec.back(), flags);
	return bytes;
}


namespace detail {

template <typename T, typename ... Args>
inline void byte_count_wrapper_send(zmq::socket_t & sock, size_t & bytes, T const & t, Args const & ... args)
{
	bytes += send_one(sock, t, ZMQ_SNDMORE|ZMQ_NOBLOCK);
	send(sock, bytes, t, args ...);
}

}  // detail

//! async send implementation
template <typename T>
inline size_t send(zmq::socket_t & sock, T const & t)
{
	return send_one(sock, t, 0|ZMQ_NOBLOCK);
}

template <typename T, typename ... Args>
inline size_t send(zmq::socket_t & sock, T const & t, Args const & ... args)
{
	size_t bytes = 0;
	detail::byte_count_wrapper_send(sock, bytes, t, args ...);
	return bytes;
}

/*! Send multipart ZMQ message.
\code
	send_multipart(sock, 1);
	send_multipart(sock, 2);
	send_sock(3);
\endcode */
template <typename T>
inline size_t send_multipart(zmq::socket_t & sock, T const & t)
{
	return send_one(sock, t, ZMQ_SNDMORE|ZMQ_NOBLOCK);
}


//! synced send implementation
template <typename T>
inline void send_sync(zmq::socket_t & sock, T const & t)
{
	send_one(sock, t, 0);
}

template <typename T, typename ... Args>
inline void send_sync(zmq::socket_t & sock, T const & t, Args const & ... args)
{
	send_one(sock, t, ZMQ_SNDMORE);
	send_sync(sock, args ...);
}

template <typename T>
inline void send_multipart_sync(zmq::socket_t & sock, T const & t)
{
	send_one(sock, t, ZMQ_SNDMORE);
}

}  // zmqu
