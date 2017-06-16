#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <time.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"

int ExchangeTranslator::esg_acq_dac_HistTiAcq(bool,
                              FILE*,
                              int,
                              const char*)
{
  static const char* fname = "esg_acq_dac_HistTiAcq"; 
  int rc = OK;

  LOG(ERROR) << fname << ": TODO";
  return rc;
}

