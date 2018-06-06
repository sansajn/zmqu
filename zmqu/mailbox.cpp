#include "mailbox.hpp"
#include <utility>
#include "send.hpp"

namespace zmqu {

using std::swap;


mailbox::mailbox(std::string const & uri, std::shared_ptr<zmq::context_t> ctx)
	: _addr{uri}
{
	_inproc = new zmq::socket_t{*ctx, ZMQ_PAIR};
	_inproc->connect(_addr.c_str());
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

mailbox::operator bool() const
{
	return _inproc != nullptr;
}

mailbox::~mailbox()
{
	if (_inproc)
		_inproc->disconnect(_addr.c_str());

	delete _inproc;
}

void mailbox::send(char const * command, std::string const & message)
{
	zmqu::send(*_inproc, command, message);
}

}  // zmq
