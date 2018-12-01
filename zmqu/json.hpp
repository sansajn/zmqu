#pragma once
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

using jtree = boost::property_tree::ptree;

namespace std {

//! converts tree to json string
string to_string(jtree const & json);

}

//! converts json string to tree
void to_json(std::string const & s, jtree & result);

//! put vector to jtree
void vector_put(std::vector<std::string> const & v, std::string const & key, jtree & result);
void vector_put(std::vector<size_t> const & v, std::string const & key, jtree & result);
