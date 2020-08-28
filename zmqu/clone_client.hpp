#pragma once
#include <memory>
#include <atomic>
#include <mutex>
#include <string>
#include <zmq.hpp>
#include "concurrent_queue.hpp"
#include "poller.hpp"

namespace zmqu {

/*! Multithread safe ZeroMQ clone pattern client implementation.
\code
struct cout_client : public zmqu::clone_client
{
	void on_news(string const & news) override
	{
		std::cout << news << std::endl;
	}
};

void main(int argc, char * argv[])
{
	zmqu::async<cout_client> c;
	c.connect("localhost", 5555, 5556, 5557);
	c.run_sync();
	c.join();
}
\endcode */
class clone_client
{
public:
	enum socket_id
	{
		SUBSCRIBER,   // can only receive messages (news)
		REQUESTER,  // can transmit and receive messages (question, answer)
		NOTIFIER,  // can only transmit messages (notifications)
		SOCKET_ID_COUNT
	};

	clone_client();
	clone_client(std::shared_ptr<zmq::context_t> ctx);
	virtual ~clone_client();

	virtual void connect(std::string const & host, short news_port, short ask_port, short notification_port);
	virtual void start();
	virtual void ask(std::string const & question) const;  //!< ask server
	virtual void notify(std::string const & news) const;  //!< notify server
	virtual void quit();  //!< async quit request
	virtual bool ready() const;
	virtual bool connected() const;  //!< check all sockets connected
	virtual bool connected(socket_id sid) const;

	int clear_incoming_subscriber_message_queue();  //!< returns number of removed messages

protected:
	virtual void idle();  //!< \note executed from clone_client thread

	zmq::context_t & context();

	// server events
	virtual void on_news(std::string const & news);  //!< on server news (publisher socket)
	virtual void on_answer(std::string const & answer);  //!< on server's answer to question (ask method)

	// socket events
	virtual void on_connected(socket_id sid, std::string const & addr);
	virtual void on_closed(socket_id sid, std::string const & addr);
	virtual void on_socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);

private:
	void setup_and_run();
	void loop();
	void install_monitors();
	void handle_monitor_events();
	void socket_event(socket_id sid, zmq_event_t const & e, std::string const & addr);
	void free_zmq();
	void set_connected(socket_id sid, bool value);

	std::shared_ptr<zmq::context_t> _ctx;
	zmq::socket_t * _subscriber,
		* _requester,
		* _notifier;
	std::atomic_bool _ready,
		_quit;
	std::once_flag _running;
	mutable concurrent_queue<std::string> _requester_queue,
		_notifier_queue;
	std::string _host;
	short _news_port,
		_ask_port,
		_notification_port;
	zmq::socket_t * _sub_mon,
		* _req_mon,
		* _notif_mon;  //!< ZMQ monotor sockets
	std::atomic_bool _subscriber_connected,
		_requester_connected,
		_notifier_connected;
};

}   // zmqu
