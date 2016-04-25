#include "mailbox.hpp"
#include <utility>
#include "send.hpp"

namespace zmq {

using std::swap;


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

}  // zmq
