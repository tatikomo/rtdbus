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

int ExchangeTranslator::esg_acq_dac_MultiThresAcq(FILE*, int, const char* s_AcqSite)
{
  static const char* fname = "esg_acq_dac_MultiThresAcq";
  int rc = OK;

  LOG(ERROR) << fname << ": TODO " << s_AcqSite;
  return rc;
}

