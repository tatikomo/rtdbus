/*
 * g++ -g list_databases.cpp -I../../eXtremeDB_5_fusion_eval/include -L../../eXtremeDB_5_fusion_eval/target/bin.so -Wl,-rpath,../../eXtremeDB_5_fusion_eval/target/bin.so -lmcolib -lmcotmvcc -lmcounrt -lmcovtmem -lmcomipc -lmcoslnx -o list
 * 
 */
#include <cstddef> // ptrdiff_t
#include <string>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "mco.h"

int main(int argc, char* argv[])
{
  int status = EXIT_SUCCESS;
  char lpBuffer[4000];
  char* ptr = &lpBuffer[0];
  mco_size32_t buffer_size = 4000;
  mco_counter32_t skip_first = 0;
  MCO_RET rc;

  do
  {
    rc = mco_runtime_start();
    if (rc)
    {
      std::cout << "E: mco_runtime_start, rc=" << rc << std::endl;
      break;
    }

    rc = mco_db_databases(lpBuffer, buffer_size, skip_first);
    if (rc)
    {
      std::cout << "E: mco_runtime_start, rc=" << rc << std::endl;
      break;
    }

    int idx = 0;
    while(*ptr)
    {
      idx++;
      std::cout << (unsigned int) idx << " name: " << ptr << std::endl;
      ptr = ptr + strlen(ptr) + 1;
    }

    status = EXIT_SUCCESS;

  } while(false);

  rc = mco_runtime_stop();

  std::cout << "mco_db_databases test: " << status << ", rc=" << rc << std::endl;
  return status;
}
