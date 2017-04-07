#pragma once
#include <string>
#include <memory>
#include "zmqu/zmqu.hpp"

namespace zmqu {

/*! Multithread safe ZeroMQ clone pattern client implementation.
TODO: insert documentation reference */
class clone_client
{
public:
	enum socket_id {SUBSCRIBER, REQUESTER, NOTIFIER};

	clone_client();
	clone_client(std::shared_ptr<zmq::context_t> ctx);
	virtual ~clone_client();

	virtual void connect(std::string const & host, short first_port);
	virtual void connect(std::string const & host, short subscriber_port, short requester_port, short notifier_port);
	virtual void start();

	mailbox create_mailbox() const;

	void ask(mailbox & m, std::string const & question) const;  //!< ask a server (DEALER socket)
	void notify(mailbox & m, std::string const & news) const;  //!< notify a server (PUSH socket)
	void command(mailbox & m, std::string const & cmd) const;  //!< quit, install_monitors
	void quit(mailbox & m) const;

	virtual void on_news(std::string const & s);  //!< a news from publisher
	virtual void on_answer(std::string const & s);  //!< server answer on request \saa ask()

	// socket events
	virtual void on_connected(socket_id sid, std::string const & addr);
	virtual void on_closed(socket_id sid, std::string const & addr);
	virtual void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);

	// state monitoring api
	virtual void on_wait();
	virtual void on_receive();

protected:
	virtual void idle();  //!< called once each loop
	void ask_internal(std::string const & s);
	void notify_internal(std::string const & s);

private:
	void loop();
	void socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);
	void handle_monitor_events();
	void install_monitors();

	std::shared_ptr<zmq::context_t> _ctx;
	zmq::socket_t * _subscriber;
	zmq::socket_t * _requester;
	zmq::socket_t * _notifier;
	zmq::socket_t * _inproc;
	zmq::socket_t * _subscriber_mon;
	zmq::socket_t * _requester_mon;
	zmq::socket_t * _notifier_mon;
	zmqu::poller _socks;
	size_t _subscriber_idx, _requester_idx, _inproc_idx;
	size_t _subscriber_mon_idx, _requester_mon_idx, _notifier_mon_idx;
	bool _quit;
	bool _running;
	std::string _host;
	short _subscriber_port, _requester_port, _notifier_port;
};

}  // zmq
