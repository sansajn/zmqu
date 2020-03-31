// boost::property_tree based json support
#pragma once
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <zmq.hpp>

using jtree = boost::property_tree::ptree;

namespace std {

//! converts tree to json string
string to_string(jtree const & json, bool pretty = true);

}

//! converts json string to tree
void to_json(std::string const & s, jtree & result);

//! put vector to jtree
void vector_put(std::vector<std::string> const & v, std::string const & key, jtree & result);
void vector_put(std::vector<size_t> const & v, std::string const & key, jtree & result);

namespace zmqu {

void send_json(zmq::socket_t & sock, jtree & json);
void recv_json(zmq::socket_t & sock, jtree & json);

}  // zmqu
