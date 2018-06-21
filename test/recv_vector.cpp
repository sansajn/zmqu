// not yet working
#include <vector>
#include <string>
#include <thread>
#include <zmqu/send.hpp>
#include <zmqu/recv.hpp>

using std::string;
using std::vector;

void simple_varargs_client_with_vector_send(zmq::context_t & ctx)
{
	zmq::socket_t requester{ctx, ZMQ_REQ};
	requester.connect("tcp://localhost:5555");

	// send multipart message (name, age, salary)
//	vector<int> data{1, 2, 3, 4, 5};
	zmqu::send(requester, 1);
	zmqu::send(requester, 2);
	zmqu::send(requester, 3);
	zmqu::send(requester, 4);
	zmqu::send(requester, 5);

	string s = zmqu::recv(requester);

	// done
}

int main(int argc, char * argv[])
{
	zmq::context_t ctx;
	zmq::socket_t responder{ctx, ZMQ_REP};
	responder.bind("tcp://*:5555");

	// install client
	std::thread client_thread{simple_varargs_client_with_vector_send, std::ref(ctx)};
	std::this_thread::sleep_for(std::chrono::milliseconds{20});  // wait for client

	vector<int> data{0, 0, 0, 0, 0};
	zmqu::recv(responder, data[0]);

//	REQUIRE(data.size() == 5);
	assert(data[0] == 1);
//	REQUIRE(data[1] == 2);
//	REQUIRE(data[2] == 3);
//	REQUIRE(data[3] == 4);
//	REQUIRE(data[4] == 5);

	responder.send("ok", 2);

	client_thread.join();
}
