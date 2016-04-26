#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_egsa_impl.hpp"

int main()
{
  int rc = NOK;
  EGSA* instance = NULL;

  instance = new EGSA();
  LOG(INFO) << "Start EGSA instance";

  if (OK == instance->init()) {
    rc = instance->run();
  }

  LOG(INFO) << "Finish EGSA instance, rc=" << rc;

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}

