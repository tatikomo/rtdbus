/* config.h. Generated by cmake from config.h.cmake */

#ifndef _RTDBUS_CONFIG_H
#define _RTDBUS_CONFIG_H
/****************************/
/* indicate that we are building with cmake */
#define RTDBUS_CMAKE 1

#cmakedefine HAVE_GNUC_VARARGS 1

#cmakedefine RTDBUS_CONSOLE_AUTH_DIR "@RTDBUS_CONSOLE_AUTH_DIR@"
#cmakedefine RTDBUS_DATADIR  "@RTDBUS_DATADIR@"
#cmakedefine RTDBUS_BINDIR   "@RTDBUS_BINDIR@"
#cmakedefine RTDBUS_SYSTEM_CONFIG_FILE  "@RTDBUS_SYSTEM_CONFIG_FILE@"
#cmakedefine RTDBUS_SESSION_CONFIG_FILE "@RTDBUS_SESSION_CONFIG_FILE@"
#cmakedefine RTDBUS_DAEMON_NAME "@RTDBUS_DAEMON_NAME@"
#cmakedefine RTDBUS_SYSTEM_BUS_DEFAULT_ADDRESS  "@RTDBUS_SYSTEM_BUS_DEFAULT_ADDRESS@"
#cmakedefine RTDBUS_MACHINE_UUID_FILE "@RTDBUS_MACHINE_UUID_FILE@"
//#cmakedefine RTDBUS_SESSION_BUS_DEFAULT_ADDRESS "@RTDBUS_SESSION_BUS_DEFAULT_ADDRESS@"
#cmakedefine RTDBUS_DAEMONDIR "@RTDBUS_DAEMONDIR@"
#cmakedefine PACKAGE "@PACKAGE@"
/* Version number of package */
#cmakedefine RTDBUS_MAJOR_VERSION @RTDBUS_MAJOR_VERSION@
#cmakedefine RTDBUS_MINOR_VERSION @RTDBUS_MINOR_VERSION@
#cmakedefine RTDBUS_MICRO_VERSION @RTDBUS_MICRO_VERSION@
#cmakedefine RTDBUS_VERSION ((@RTDBUS_MAJOR_VERSION@ << 16) | (@RTDBUS_MINOR_VERSION@ << 8) | (@RTDBUS_MICRO_VERSION@))
#cmakedefine RTDBUS_VERSION_STRING "@RTDBUS_VERSION_STRING@"

#define VERSION RTDBUS_VERSION_STRING

#define TEST_LISTEN       "@TEST_LISTEN@"
#define TEST_CONNECTION   "@TEST_CONNECTION@"

#cmakedefine MAX_SERVICES_ENTRY     @MAX_SERVICES_ENTRY@
#cmakedefine MAX_WORKERS_ENTRY      @MAX_WORKERS_ENTRY@
#cmakedefine SERVICE_NAME_MAXLEN    @SERVICE_NAME_MAXLEN@
#cmakedefine IDENTITY_MAXLEN @IDENTITY_MAXLEN@
#cmakedefine DBNAME_MAXLEN          @DBNAME_MAXLEN@
#cmakedefine HEARTBEAT_PERIOD_MSEC  @HEARTBEAT_PERIOD_MSEC@
#cmakedefine EXTREMEDB_VERSION      @EXTREMEDB_VERSION@
#cmakedefine MESSAGE_EXPIRATION_PERIOD_MSEC      @MESSAGE_EXPIRATION_PERIOD_MSEC@
#cmakedefine USE_EXTREMEDB_HTTP_SERVER  @USE_EXTREMEDB_HTTP_SERVER@


// test binaries
/* Full path to test file test/test-exit in builddir */
#define TEST_BUS_BINARY          "@TEST_BUS_BINARY@"
/* Full path to test file test/test-exit in builddir */
#define TEST_EXIT_BINARY          "@TEST_EXIT_BINARY@"
/* Full path to test file test/test-segfault in builddir */
#define TEST_SEGFAULT_BINARY      "@TEST_SEGFAULT_BINARY@"
/* Full path to test file test/test-service in builddir */
#define TEST_SERVICE_BINARY       "@TEST_SERVICE_BINARY@"
/* Full path to test file test/test-shell-service in builddir */
#define TEST_SHELL_SERVICE_BINARY "@TEST_SHELL_SERVICE_BINARY@"
/* Full path to test file test/test-sleep-forever in builddir */
#define TEST_SLEEP_FOREVER_BINARY "@TEST_SLEEP_FOREVER_BINARY@"

/* Some dbus features */
#cmakedefine RTDBUS_BUILD_TESTS 1
#cmakedefine RTDBUS_ENABLE_ANSI 1
#cmakedefine RTDBUS_ENABLE_VERBOSE_MODE 1
#cmakedefine RTDBUS_DISABLE_ASSERTS 1
#cmakedefine RTDBUS_DISABLE_CHECKS 1
#cmakedefine RTDBUS_USE_EXTREMEDB_HTTP_SERVER 1
#cmakedefine RTDBUS_USE_MSEC_IN_EXPIRATION 1
/* xmldocs */
/* doxygen */
#cmakedefine RTDBUS_GCOV_ENABLED 1

/* abstract-sockets */

#cmakedefine HAVE_ABSTRACT_SOCKETS 1

#cmakedefine RTDBUS_PATH_OR_ABSTRACT_VALUE 1

#if (defined RTDBUS_PATH_OR_ABSTRACT_VALUE)
#define RTDBUS_PATH_OR_ABSTRACT @RTDBUS_PATH_OR_ABSTRACT_VALUE@
#endif

#ifdef RTDBUS_PATH_OR_ABSTRACT_VALUE
#undef RTDBUS_PATH_OR_ABSTRACT_VALUE
#endif

/* selinux */
#cmakedefine RTDBUS_BUS_ENABLE_DNOTIFY_ON_LINUX 1
/* kqueue */
#cmakedefine HAVE_CONSOLE_OWNER_FILE 1
#define RTDBUS_CONSOLE_OWNER_FILE "@RTDBUS_CONSOLE_OWNER_FILE@"

#cmakedefine RTDBUS_HAVE_ATOMIC_INT 1
#cmakedefine RTDBUS_USE_ATOMIC_INT_486 1
#if (defined(__i386__) || defined(__x86_64__))
# define RTDBUS_HAVE_ATOMIC_INT 1
# define RTDBUS_USE_ATOMIC_INT_486 1
#endif

#cmakedefine RTDBUS_BUILD_X11 1

#define _RTDBUS_VA_COPY_ASSIGN(a1,a2) { a1 = a2; }

#cmakedefine RTDBUS_VA_COPY_FUNC
#if (defined RTDBUS_VA_COPY_FUNC)
# define RTDBUS_VA_COPY @RTDBUS_VA_COPY_FUNC@
#endif

#ifdef RTDBUS_VA_COPY_FUNC
#undef RTDBUS_VA_COPY_FUNC
#endif

#cmakedefine RTDBUS_VA_COPY_AS_ARRAY @RTDBUS_VA_COPY_AS_ARRAY@

// headers
/* Define to 1 if you have dirent.h */
#cmakedefine   HAVE_DIRENT_H 1

/* Define to 1 if you have io.h */
#cmakedefine   HAVE_IO_H 1

/* Define to 1 if you have grp.h */
#cmakedefine   HAVE_GRP_H 1

/* Define to 1 if you have sys/poll.h */
#cmakedefine    HAVE_POLL 1

/* Define to 1 if you have sys/time.h */
#cmakedefine    HAVE_SYS_TIME 1

/* Define to 1 if you have sys/wait.h */
#cmakedefine    HAVE_SYS_WAIT 1

/* Define to 1 if you have time.h */
#cmakedefine   HAVE_TIME_H 1

/* Define to 1 if you have ws2tcpip.h */
#cmakedefine   HAVE_WS2TCPIP_H

/* Define to 1 if you have wspiapi.h */
#cmakedefine   HAVE_WSPIAPI_H 1

/* Define to 1 if you have unistd.h */
#cmakedefine   HAVE_UNISTD_H 1

/* Define to 1 if you have stdio.h */
#cmakedefine   HAVE_STDIO_H 1

/* Define to 1 if you have sys/syslimits.h */
#cmakedefine   HAVE_SYS_SYSLIMITS_H 1

/* Define to 1 if you have errno.h */
#cmakedefine   HAVE_ERRNO_H 1

/* Define to 1 if you have signal.h */
#cmakedefine   HAVE_SIGNAL_H 1

/* Define to 1 if you have locale.h */
#cmakedefine   HAVE_LOCALE_H 1

/* Define to 1 if you have inttypes.h */
#cmakedefine   HAVE_INTTYPES_H 1

/* Define to 1 if you have stdint.h */
#cmakedefine   HAVE_STDINT_H 1

// symbols
/* Define to 1 if you have backtrace */
#cmakedefine   HAVE_BACKTRACE 1

/* Define to 1 if you have getgrouplist */
#cmakedefine   HAVE_GETGROUPLIST 1

/* Define to 1 if you have getpeerucred */
#cmakedefine   HAVE_GETPEERUCRED 1

/* Define to 1 if you have nanosleep */
#cmakedefine   HAVE_NANOSLEEP 1

/* Define to 1 if you have getpwnam_r */
#cmakedefine   HAVE_POSIX_GETPWNAM_R 1

/* Define to 1 if you have socketpair */
#cmakedefine   HAVE_SOCKETPAIR 1

/* Define to 1 if you have setenv */
#cmakedefine   HAVE_SETENV 1

/* Define to 1 if you have unsetenv */
#cmakedefine   HAVE_UNSETENV 1

/* Define to 1 if you have clearenv */
#cmakedefine   HAVE_CLEARENV 1

/* Define to 1 if you have writev */
#cmakedefine   HAVE_WRITEV 1

/* Define to 1 if you have socklen_t */
#cmakedefine   HAVE_SOCKLEN_T 1

/* Define to 1 if you have setlocale */
#cmakedefine   HAVE_SETLOCALE 1

/* Define to 1 if you have localeconv */
#cmakedefine   HAVE_LOCALECONV 1

/* Define to 1 if you have strtoll */
#cmakedefine   HAVE_STRTOLL 1

/* Define to 1 if you have strtoull */
#cmakedefine   HAVE_STRTOULL 1

// structs
/* Define to 1 if you have struct cmsgred */
#cmakedefine    HAVE_CMSGCRED 1

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
   void operator=(const TypeName&)

// system type defines
#if defined(_WIN32) || defined(_WIN64) || defined (_WIN32_WCE)
# define RTDBUS_WIN
# define RTDBUS_WIN_FIXME 1
// Монотонно увеличивающийся таймер
# define CLOCK_TYPE CLOCK_MONOTONIC 
# ifdef _WIN32_WCE
#  define RTDBUS_WINCE
# else
#  define RTDBUS_WIN32
# endif
#else
# define RTDBUS_UNIX
// Тип таймера, не подверженного NTP - специфично для Linux
# define CLOCK_TYPE CLOCK_MONOTONIC_RAW
#endif

#if defined(_WIN32) || defined(_WIN64)
// mingw mode_t
# ifdef HAVE_STDIO_H
#  include <stdio.h>
# endif
# ifndef _MSC_VER
#  define uid_t int
#  define gid_t int
# else
#  define snprintf _snprintf
   typedef int mode_t;
#  if !defined(_WIN32_WCE)
#    define strtoll _strtoi64
#    define strtoull _strtoui64
#    define HAVE_STRTOLL 1
#    define HAVE_STRTOULL 1
#  endif
# endif
#endif	// defined(_WIN32) || defined(_WIN64)

#ifdef interface
#undef interface
#endif

#ifndef SIGHUP
#define SIGHUP	1
#endif

#cmakedefine RTDBUS_VERBOSE_C_S 1
#ifdef RTDBUS_VERBOSE_C_S
#define _dbus_verbose_C_S printf
#else
#define _dbus_verbose_C_S _dbus_verbose
#endif 

#endif  // _RTDBUS_CONFIG_H
