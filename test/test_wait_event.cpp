#include <chrono>
#include <catch.hpp>
#include "zmqu/async.hpp"

using zmqu::wait_event;
using std::chrono::steady_clock;
using namespace std::chrono_literals;

TEST_CASE("we can quit waiting for an event", 
	"[wait]")
{
	bool quit = false;
	REQUIRE(wait_event([]{return true;}, 10ms, quit));

	quit = true;
	REQUIRE_FALSE(wait_event([]{return true;}, 10ms, quit));

	quit = false;
	auto t0 = steady_clock::now();
	REQUIRE(wait_event([t0]{
		return steady_clock::now() > t0 + 10ms ? true : false;}, 100ms, quit));

	t0 = steady_clock::now();
	REQUIRE_FALSE(wait_event([t0]{
		return steady_clock::now() > (t0 + 10ms) ? true : false;}, 5ms, quit));
}
