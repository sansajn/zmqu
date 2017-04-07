#pragma once
#include <vector>
#include <string>
#include <zmq.hpp>
#include "json.hpp"

namespace zmqu {

std::string recv(zmq::socket_t & sock);
void recv_json(zmq::socket_t & sock, jtree & json);


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

}  // zmq
