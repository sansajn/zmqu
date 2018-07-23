#include <atomic>
#include <thread>
#include <string>
#include <catch.hpp>
#include "zmqu/send.hpp"
#include "zmqu/recv.hpp"

using std::string;



struct message_consumer
{
public:
	message_consumer(zmq::context_t & ctx)
		: _pull{zmq::socket_t{ctx, ZMQ_PULL}}
	{
		_pull.setsockopt(ZMQ_LINGER, 0);
		_pull.bind("tcp://*:13333");
	}

	void consume(size_t n)
	{
		for (; n > 0; --n)
			string data = zmqu::recv(_pull);
	}

private:
	zmq::socket_t _pull;
};


TEST_CASE("send will not block", "[send]")
{
	string const msg = "hello Patrick";

	zmq::context_t ctx;
	zmq::socket_t sock{ctx, ZMQ_PUSH};
	sock.setsockopt(ZMQ_LINGER, 0);
	sock.connect("tcp://localhost:13333");

	size_t message_count = 0;

	// fill sending queue
	while (true)
	{
		size_t bytes = zmqu::send(sock, msg);
		if (bytes == 0)
		{
			assert(zmq_errno() == EAGAIN);
			break;
		}

		message_count += 1;
	}

	size_t const full_queue_count = message_count;

	// consume messages
	message_consumer consumer{ctx};
	consumer.consume(200);

	std::this_thread::sleep_for(std::chrono::seconds{1});  // wait for sending queue clean-up

	// we should be able send more messages now
	for (int i = 0; i < 100; ++i)
	{
		size_t bytes = zmqu::send(sock, msg);
		if (bytes == 0)
		{
			assert(zmq_errno() == EAGAIN);
			break;
		}

		message_count += 1;
	}

	REQUIRE(message_count == (full_queue_count + 100));
}
