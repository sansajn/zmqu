#pragma once
#include "zmqu/zmqu.hpp"

namespace zmq {

//! Multithread safe ZeroMQ clone pattern server implementation.
class clone_server
{
public:
	enum socket_id {PUBLISHER, RESPONDER, COLLECTOR};

	clone_server();
	clone_server(std::shared_ptr<zmq::context_t> ctx);
	virtual ~clone_server();

	virtual void bind(short first_port, std::string const & host = "*");
	virtual void bind(short publisher_port, short responder_port, short collector_port, std::string const & host = "*");
	virtual void start();
	virtual void publish(mailbox & m, std::string const & news);  //!< publish to all subscribers (PUB socket)
	virtual void command(mailbox & m, std::string const & cmd);  //!< quit, install_monitors

	mailbox create_mailbox() const;  //!< \note needs to be called after start()

	// commands
	void quit(mailbox & m) const;

	virtual std::string on_question(std::string const & question);  //!< client question (ROUTER socket)
	virtual void on_notify(std::string const & s);  //!< notify message from client (PULL socket)
	virtual void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);

	// monitoring
	virtual void on_wait();
	virtual void on_receive();

protected:
	virtual void idle();
	void publish_internal(std::string const & s);

private:
	void loop();
	void handle_monitor_events();
	void install_monitors();

	std::shared_ptr<zmq::context_t> _ctx;
	zmq::socket_t * _publisher;
	zmq::socket_t * _responder;
	zmq::socket_t * _collector;
	zmq::socket_t * _inproc;
	zmq::socket_t * _publisher_mon;
	zmq::socket_t * _responder_mon;
	zmq::socket_t * _collector_mon;
	zmq::poller _socks;
	size_t _responder_idx, _collector_idx, _inproc_idx;
	size_t _publisher_mon_idx, _responder_mon_idx, _collector_mon_idx;
	bool _quit;
	bool _running;
	std::string _host;
	short _publisher_port, _responder_port, _collector_port;
};

}  // zmq
