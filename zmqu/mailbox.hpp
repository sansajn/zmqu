#pragma once
#include <memory>
#include <string>
#include <boost/noncopyable.hpp>
#include <zmq.hpp>

namespace zmq {

class mailbox
	: private boost::noncopyable
{
public:
	mailbox() : _inproc{nullptr} {}
	mailbox(std::string const & uri, std::shared_ptr<zmq::context_t> ctx);
	~mailbox();
	void send(char const * command, std::string const & message);  // TODO: not a general (implement varargs template there)

	mailbox(mailbox && rhs);
	void operator=(mailbox && rhs);

private:
	zmq::socket_t * _inproc;
	std::string _addr;
	// TODO: should I store context to prevent its deletion ?
};

}  // zmq
