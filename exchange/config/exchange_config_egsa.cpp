///////////////////////////////////////////////////////////////////////////////////////////
// Класс представления конифгурации модуля EGSA
// 2016/04/14
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"

using namespace rapidjson;
using namespace std;

EgsaConfig::EgsaConfig()
{
  LOG(INFO) << "Constructor EgsaConfig";
}

EgsaConfig::~EgsaConfig()
{
  LOG(INFO) << "Destructor EgsaConfig";
}


