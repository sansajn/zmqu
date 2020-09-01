#pragma once
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>

namespace zmqu {

/*! helper to run client/server implementation in an asynchronous way */
template <typename T>
class async : public T
{
public:
	using T::T;  // reuse constructors
	~async();
	void run();
	bool run_sync(std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});
	bool run_sync(std::chrono::milliseconds timeout, bool & cancel);  //!< cancelable run with timeout
	void join();

private:
	std::thread _client_thread;
};

/*! wait for event with timeout
\code
async<clone_client> client;
client.connect("localhost", 5556, 5557, 5558);
client.run();
wait_event([&client]{return client.ready();}, seconds{1});
\endcode */
template <typename UnaryPredicate>
bool wait_event(UnaryPredicate pred, std::chrono::milliseconds const & timeout);

template <typename UnaryPredicate>
bool wait_event(UnaryPredicate pred, std::chrono::milliseconds const & timeout)
{
	using std::chrono::steady_clock;
	using std::chrono::microseconds;

	microseconds sleep_step = std::min(microseconds{10*1000}, microseconds{timeout}/100);
	auto until = steady_clock::now() + timeout;
	while (!pred() && (until > steady_clock::now()))
		std::this_thread::sleep_for(sleep_step);
	return pred();
}

//! cancelable `wait_event` implementation
template <typename UnaryPredicate>
bool wait_event(UnaryPredicate pred, std::chrono::milliseconds const & timeout, bool & cancel)
{
	using std::chrono::steady_clock;
	using std::chrono::microseconds;

	microseconds const sleep_step = std::min(microseconds{10*1000}, microseconds{timeout}/100);
	auto const until = steady_clock::now() + timeout;
	bool cond = false;

	while (!cancel && (until > steady_clock::now()) && !(cond = pred()))
		std::this_thread::sleep_for(sleep_step);

	return cond;
}

template <typename T>
async<T>::~async()
{
	T::quit();
	join();
}

template <typename T>
void async<T>::run()
{
	_client_thread = std::thread{&T::start, this};
}

template <typename T>
bool async<T>::run_sync(std::chrono::milliseconds timeout)
{
	run();
	return wait_event([this]{return T::ready();}, timeout);
}

template <typename T>
bool async<T>::run_sync(std::chrono::milliseconds timeout, bool & cancel)
{
	run();
	return wait_event([this]{return T::ready();}, timeout, cancel);
}

template <typename T>
void async<T>::join()
{
	if (_client_thread.joinable())
		_client_thread.join();
}

}  // zmqu
