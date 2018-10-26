#include "cast.hpp"
#include <zmq.hpp>

namespace zmqu {

using std::string;
using std::to_string;


string event_to_string(int event)
{
	switch (event)
	{
		case ZMQ_EVENT_CONNECTED: return "connected";
		case ZMQ_EVENT_CONNECT_DELAYED: return "connect delayed";
		case ZMQ_EVENT_CONNECT_RETRIED: return "connect retried";
		case ZMQ_EVENT_LISTENING: return "listenning";
		case ZMQ_EVENT_BIND_FAILED: return "bind failed";
		case ZMQ_EVENT_ACCEPTED: return "accepted";
		case ZMQ_EVENT_ACCEPT_FAILED: return "accept failed";
		case ZMQ_EVENT_CLOSED: return "closed";
		case ZMQ_EVENT_CLOSE_FAILED: return "close failed";
		case ZMQ_EVENT_DISCONNECTED: return "disconnected";
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 1, 0)
		case ZMQ_EVENT_MONITOR_STOPPED: return "monitor stopped";
#endif
		default: return to_string(event);
	}
}

}   // zmq
