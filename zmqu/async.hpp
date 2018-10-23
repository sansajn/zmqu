#pragma once
#include <thread>

namespace zmqu {

//! \note we expect ClientImpl to be derived from zmqu::clone_client
template <typename ClientImpl>
class async_client
	: public ClientImpl
{
public:
	async_client();
	void run();
	virtual ~async_client();

private:
	std::thread _client_thread;
};


template <typename ClientImpl>
async_client<ClientImpl>::async_client() {}

template <typename ClientImpl>
void async_client<ClientImpl>::run()
{
	_client_thread = std::thread{&ClientImpl::start, this};
	std::this_thread::sleep_for(std::chrono::milliseconds{100});  // wait for thread
}

template <typename ClientImpl>
async_client<ClientImpl>::~async_client()
{
	ClientImpl::quit();
	if (_client_thread.joinable())
		_client_thread.join();
}

}  // zmqu
