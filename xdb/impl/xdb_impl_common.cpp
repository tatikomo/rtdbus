#include <string>
#include <map>

#include "xdb_impl_common.hpp"
#include "xdb_impl_common.h"

using namespace xdb;

bool getOption(const Options& _options, const std::string& key, int& val)
{
  bool status = false;
  const OptionIterator p = _options.find(key);

  if (p != _options.end()) {
    val = p->second;
    status = true;
    // std::cout << "Found '"<<key<<"' option=" << p->second << std::endl;
  }
  return status;
}

bool setOption(const Options& _options, const std::string& key, int val)
{
  bool status = false;
  const OptionIterator p = _options.find(key);

  if (p != _options.end()) {
    val = p->second;
    status = true;
//    std::cout << "Replace '" << key
//              << "' old value " << p->second
//              << " with " << val << std::endl;
  }
  else
  {
    _options.push_back(key, val);
    std::cout << "Insert into '" << key
              << " new value " << val << std::endl;
    status = true;
  }
  return status;
}

