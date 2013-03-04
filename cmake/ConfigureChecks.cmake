include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckStructMember)
include(CheckTypeSize)

check_include_file(dirent.h     HAVE_DIRENT_H)  # 
check_include_file(io.h         HAVE_IO_H)      # internal
check_include_file(grp.h        HAVE_GRP_H)     # 
check_include_file(sys/poll.h   HAVE_POLL)      # 
check_include_file(sys/time.h   HAVE_SYS_TIME_H)# 
check_include_file(sys/wait.h   HAVE_SYS_WAIT_H)# 
check_include_file(time.h       HAVE_TIME_H)    #
check_include_file(ws2tcpip.h   HAVE_WS2TCPIP_H)#
check_include_file(wspiapi.h    HAVE_WSPIAPI_H) #
check_include_file(unistd.h     HAVE_UNISTD_H)  #
check_include_file(stdio.h      HAVE_STDIO_H)   #
check_include_file(sys/syslimits.h    HAVE_SYS_SYSLIMITS_H)   #
check_include_file(errno.h     HAVE_ERRNO_H)    #
check_include_file(signal.h     HAVE_SIGNAL_H)
check_include_file(locale.h     HAVE_LOCALE_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H) #
check_include_file(stdint.h     HAVE_STDINT_H)   #

check_symbol_exists(backtrace    "execinfo.h"       HAVE_BACKTRACE)          #
check_symbol_exists(getgrouplist "grp.h"            HAVE_GETGROUPLIST)       #
check_symbol_exists(getpeerucred "ucred.h"          HAVE_GETPEERUCRED)       #
check_symbol_exists(nanosleep    "time.h"           HAVE_NANOSLEEP)          #
check_symbol_exists(getpwnam_r   "errno.h pwd.h"    HAVE_POSIX_GETPWNAM_R)   #
check_symbol_exists(setenv       "stdlib.h"         HAVE_SETENV)             #
check_symbol_exists(unsetenv     "stdlib.h"         HAVE_UNSETENV)           #
check_symbol_exists(clearenv     "stdlib.h"         HAVE_CLEARENV)           #
check_symbol_exists(writev       "sys/uio.h"        HAVE_WRITEV)             #
check_symbol_exists(setrlimit    "sys/resource.h"   HAVE_SETRLIMIT)          #
check_symbol_exists(socketpair   "sys/socket.h"     HAVE_SOCKETPAIR)         #
check_symbol_exists(socklen_t    "sys/socket.h"     HAVE_SOCKLEN_T)          #
check_symbol_exists(setlocale    "locale.h"         HAVE_SETLOCALE)          #
check_symbol_exists(localeconv   "locale.h"         HAVE_LOCALECONV)         #
check_symbol_exists(strtoll      "stdlib.h"         HAVE_STRTOLL)            #
check_symbol_exists(strtoull     "stdlib.h"         HAVE_STRTOULL)           #

check_struct_member(cmsgcred cmcred_pid "sys/types.h sys/socket.h" HAVE_CMSGCRED)   #

# missing:
# HAVE_ABSTRACT_SOCKETS
# RTDBUS_HAVE_GCC33_GCOV

check_type_size("short"     SIZEOF_SHORT)
check_type_size("int"       SIZEOF_INT)
check_type_size("long"      SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("__int64"   SIZEOF___INT64)

# RTDBUS_INT64_TYPE
if(SIZEOF_INT EQUAL 8)
    set (RTDBUS_HAVE_INT64 1)
    set (RTDBUS_INT64_TYPE "int")
else(SIZEOF_INT EQUAL 8)
    if(SIZEOF_LONG EQUAL 8)
        set (RTDBUS_HAVE_INT64 1)
        set (RTDBUS_INT64_TYPE "long")
    else(SIZEOF_LONG EQUAL 8)
        if(SIZEOF_LONG_LONG EQUAL 8)
            set (RTDBUS_HAVE_INT64 1)
            set (RTDBUS_INT64_TYPE "long long")
        else(SIZEOF_LONG_LONG EQUAL 8)
            if(SIZEOF___INT64 EQUAL 8)
                set (RTDBUS_HAVE_INT64 1)
                set (RTDBUS_INT64_TYPE "__int64")
            endif(SIZEOF___INT64 EQUAL 8)
        endif(SIZEOF_LONG_LONG EQUAL 8)
    endif(SIZEOF_LONG EQUAL 8)
endif(SIZEOF_INT EQUAL 8)

# RTDBUS_INT32_TYPE
if(SIZEOF_INT EQUAL 4)
    set (RTDBUS_INT32_TYPE "int")
else(SIZEOF_INT EQUAL 4)
    if(SIZEOF_LONG EQUAL 4)
        set (RTDBUS_INT32_TYPE "long")
    else(SIZEOF_LONG EQUAL 4)
        if(SIZEOF_LONG_LONG EQUAL 4)
            set (RTDBUS_INT32_TYPE "long long")
        endif(SIZEOF_LONG_LONG EQUAL 4)
    endif(SIZEOF_LONG EQUAL 4)
endif(SIZEOF_INT EQUAL 4)

# RTDBUS_INT16_TYPE
if(SIZEOF_INT EQUAL 2)
    set (RTDBUS_INT16_TYPE "int")
else(SIZEOF_INT EQUAL 2)
    if(SIZEOF_SHORT EQUAL 2)
        set (RTDBUS_INT16_TYPE "short")
    endif(SIZEOF_SHORT EQUAL 2)
endif(SIZEOF_INT EQUAL 2)


find_library(LIBZMQ libzmq)


write_file("${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/cmake_try_compile.c" "#include <stdarg.h>
	#include <stdlib.h>
        static void f (int i, ...) {
	va_list args1, args2;
	va_start (args1, i);
	va_copy (args2, args1);
	if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
	  exit (1);
	va_end (args1); va_end (args2);
	}
	int main() {
	  f (0, 42);
	  return 0;
	}
")
try_compile(RTDBUS_HAVE_VA_COPY
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/cmake_try_compile.c)

if(RTDBUS_HAVE_VA_COPY)
  SET(RTDBUS_VA_COPY_FUNC va_copy CACHE STRING "va_copy function")
else(RTDBUS_HAVE_VA_COPY)
  write_file("${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/cmake_try_compile.c" "#include <stdarg.h>
          #include <stdlib.h>
	  static void f (int i, ...) {
	  va_list args1, args2;
	  va_start (args1, i);
	  __va_copy (args2, args1);
	  if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
	    exit (1);
	  va_end (args1); va_end (args2);
	  }
	  int main() {
	    f (0, 42);
	    return 0;
	  }
  ")
  try_compile(RTDBUS_HAVE___VA_COPY
              ${CMAKE_BINARY_DIR}
              ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/cmake_try_compile.c)
  if(RTDBUS_HAVE___VA_COPY)
    SET(RTDBUS_VA_COPY_FUNC __va_copy CACHE STRING "va_copy function")
  else(RTDBUS_HAVE___VA_COPY)
    SET(RTDBUS_VA_COPY_AS_ARRAY "1" CACHE STRING "'va_lists' cannot be copies as values")
  endif(RTDBUS_HAVE___VA_COPY)
endif(RTDBUS_HAVE_VA_COPY)
#### Abstract sockets

if (RTDBUS_ENABLE_ABSTRACT_SOCKETS)

  try_compile(HAVE_ABSTRACT_SOCKETS
              ${CMAKE_BINARY_DIR}
              ${CMAKE_SOURCE_DIR}/modules/CheckForAbstractSockets.c)

endif(RTDBUS_ENABLE_ABSTRACT_SOCKETS)

if(HAVE_ABSTRACT_SOCKETS)
  set(RTDBUS_PATH_OR_ABSTRACT_VALUE abstract)
else(HAVE_ABSTRACT_SOCKETS)
  set(RTDBUS_PATH_OR_ABSTRACT_VALUE path)
endif(HAVE_ABSTRACT_SOCKETS)

