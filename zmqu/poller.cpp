#include "poller.hpp"

namespace zmq {

void poller::add(zmq::socket_t & sock, short revents)  // TODO: rename -> revent are events
{
	_items.push_back(zmq_pollitem_t{(void *)sock, 0, revents, 0});
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

}  // zmq