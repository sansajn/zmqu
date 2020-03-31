#pragma once
#include <thread>

namespace zmqu {

/*! helper to run client/server implementation in an asynchronous way */
template <typename T>
class async : public T
{
public:
	using T::T;  // reuse constructors
	~async();
	void run();
	void join();

private:
	std::thread _client_thread;
};

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
void async<T>::join()
{
	if (_client_thread.joinable())
		_client_thread.join();
}

}  // zmqu
