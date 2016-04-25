#pragma once
#include <string>
#include <zmq.hpp>
#include "json.hpp"
#include "recv.hpp"
#include "send.hpp"
#include "poller.hpp"
#include "mailbox.hpp"

namespace zmq {

std::string event_to_string(int event);

}   // zmq
