#pragma once
#ifndef XDB_CORE_COMMON_HPP
#define XDB_CORE_COMMON_HPP

#include <string>
#include "helper.hpp" // Options

bool getOption(const Options&, const std::string&, int&);
bool setOption(const Options&, const std::string&, int);

#endif
