#pragma once
#include <atomic>
#include <string>
#include <zmq.hpp>
#include "concurrent_queue.hpp"

namespace zmqu {

/*! Multithread safe (queue based) ZeroMQ clone pattern server implementation.
\code
	void foo()
	{
		clone_server serv;
		serv.bind("*", 5555, 5556, 5557);
		std::thread t{&clone_server::start, &serv};
		serv.publish("Patric Jane");
		// ...
		t.join();
	}
\endcode */
class clone_server
{
public:
	enum socket_id
	{
		PUBLISHER,  // can only transmit messages (news)
		RESPONDER, // can receive and transmit messages (question, answer)
		COLLECTOR  // can only receive a messages (notifications)
	};

	clone_server();
	clone_server(std::shared_ptr<zmq::context_t> ctx);
	virtual ~clone_server();

	virtual void bind(short news_port, short answer_port, short notification_port);
	virtual void bind(std::string const & host, short news_port, short answer_port, short notification_port);
	virtual void start();
	virtual void publish(std::string const & news) const;  // feed ?
	virtual void quit();

protected:
	virtual void idle();

	// client events
	virtual std::string on_question(std::string const & question);  //!< on client question
	virtual void on_notify(std::string const & s);  //!< on client notification

	// socket events
	virtual void on_accepted(socket_id sid, std::string const & addr);
	virtual void on_disconnected(socket_id sid, std::string const & addr);
	virtual void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);

private:
	void loop();
	void install_monitors();
	void handle_monitor_events();
	void socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);
	void free_zmq();

	std::shared_ptr<zmq::context_t> _ctx;
	// _thread_ctx_copy
	zmq::socket_t * _publisher, * _responder, * _collector;
	std::atomic_bool _running, _quit;
	mutable concurrent_queue<std::string> _publisher_queue;
	std::string _host;
	short _news_port, _answer_port, _notification_port;
	zmq::socket_t * _pub_mon, * _resp_mon, * _coll_mon;
};

}  // zmq
