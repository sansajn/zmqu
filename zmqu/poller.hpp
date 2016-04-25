#pragma once
#include <vector>
#include <chrono>
#include <zmq.hpp>

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

}  // zmq
