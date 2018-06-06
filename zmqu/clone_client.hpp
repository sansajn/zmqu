#pragma once
#include <memory>
#include <atomic>
#include <string>
#include <zmq.hpp>
#include "concurrent_queue.hpp"
#include "poller.hpp"

namespace zmqu {

/*! Multithread safe (queue based) ZeroMQ clone pattern client implementation.
\code
	struct cout_client_impl : publis zmqu::clone_client
	{
		void on_news(string const & news) override
		{
			cout << news << std::endl;
		}
	};

	void foo()
	{
		cout_client_impl c;
		c.connect("localhost", 8876, 8877, 8878);
		std::thread t{&clone_client_q::start, &c};
		t.join();
	}
\endcode */
class clone_client
{
public:
	enum socket_id
	{
		SUBSCRIBER,   // can receive messages
		REQUESTER,  // can transmit and receive messages
		NOTIFIER  // can transmit messages
	};

	clone_client();
	clone_client(std::shared_ptr<zmq::context_t> ctx);
	virtual ~clone_client();

	virtual void connect(std::string const & host, short news_port, short ask_port, short notification_port);
	virtual void start();
	void ask(std::string const & question) const;  //!< ask server
	void notify(std::string const & news) const;  //!< notify server
	virtual void idle();
	virtual void quit();  //!< transparent async quit

	// server events
	virtual void on_news(std::string const & news);  //!< on server news (publisher socket)
	virtual void on_answer(std::string const & answer);  //!< on server's answer to question (ask method)

	// socket events
	virtual void on_connected(socket_id sid, std::string const & addr);
	virtual void on_closed(socket_id sid, std::string const & addr);
	virtual void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);

private:
	void loop();
	void install_monitors();
	void handle_monitor_events();
	void socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);
	void free_zmq();

	std::shared_ptr<zmq::context_t> _ctx;
	zmq::socket_t * _subscriber, * _requester, * _notifier;
	std::atomic_bool _running, _quit;
	mutable concurrent_queue<std::string> _requester_queue, _notifier_queue;
	std::string _host;
	short _news_port, _ask_port, _notification_port;
	zmq::socket_t * _sub_mon, * _req_mon, * _notif_mon;  //!< ZMQ monotor sockets
};

}   // zmqu
