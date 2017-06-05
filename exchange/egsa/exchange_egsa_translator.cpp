#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <map>
#include <istream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_translator.hpp"

ExchangeTranslator::ExchangeTranslator(locstruct_item_t* _locstructs, elemtype_item_t* _elemtypes, elemstruct_item_t* _elemstructs)
{
  LOG(INFO) << "CTOR ExchangeTranslator";
  locstruct_item_t* locstruct = _locstructs;
  elemtype_item_t* elemtype = _elemtypes;
  elemstruct_item_t* elemstruct = _elemstructs;

  while (locstruct->name[0]) {
    LOG(INFO) << "locstruct: " << locstruct->name;
    locstruct++;
  }
};

ExchangeTranslator::~ExchangeTranslator()
{
  LOG(INFO) << "DTOR ExchangeTranslator";
};

int ExchangeTranslator::load(std::stringstream& buffer)
{
  static const char* fname = "ExchangeTranslator::load";
  int rc = OK;
  std::string line;

  while (buffer.good()) {

    buffer >> line;
    //LOG(INFO) << line;
  }

  return rc;
}

