#pragma once
#include <string>
#include <boost/property_tree/json_parser.hpp>

using jtree = boost::property_tree::ptree;

namespace std {

//! converts tree to json string
string to_string(jtree const & json);

}

//! converts json string to tree
void to_json(std::string const & s, jtree & result);
