#if TYPE_OS == LINUX
#include <unistd.h>
#include <sys/syscall.h> // gettid()
#endif

#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>

#include <string.h> // strncat

#include <string>
#include <map>
#include <iterator>

#include "helper.hpp"
typedef Options::iterator OptionIterator;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char buf[4000];

void LogError(int rc,
            const char *functionName,
            const char *format,
            ...)
{
    const char *empty = "";
    const char *pre_format = "E %s [%d]: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    pthread_mutex_lock(&mutex);
#if defined(sun)
    printf ("[%lu] ", pthread_self());
#elif TYPE_OS==LINUX
    printf ("[%ld] ", static_cast<long int>(syscall(SYS_gettid)));
#else
#warning "Unsupported OS"
#endif

    sprintf(buffer, pre_format, 
            functionName? functionName : empty,
            rc);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
    va_end (args);
    pthread_mutex_unlock(&mutex);
}

void LogWarn(
            const char *functionName,
            const char *format,
            ...)
{
    const char *empty = "";
    const char *pre_format = "W %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    pthread_mutex_lock(&mutex);
    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
#if defined(sun)
    fprintf(stdout, "[%lu] %s\n", pthread_self(), buffer);
#elif (TYPE_OS==LINUX)
    fprintf(stdout, "[%ld] %s\n", static_cast<long int>(syscall(SYS_gettid)), buffer);
#else
#warning "Unsupported OS"
#endif
    fflush(stdout);
    va_end (args);
    pthread_mutex_unlock(&mutex);
}

void LogInfo(
            const char *functionName,
            const char *format,
            ...)
{
#if defined DEBUG
    pthread_mutex_lock(&mutex);
    const char *empty = "";
    const char *pre_format = "I %s: ";
    char buffer[255];
    char user_msg[255];
    va_list args;

    sprintf(buffer, pre_format, functionName? functionName : empty);

    va_start (args, format);
    vsprintf (user_msg, format, args);
    strncat(buffer, user_msg, sizeof(buffer)-1);
#if defined(sun)
    fprintf(stdout, "[%lu] %s\n", pthread_self(), buffer);
#elif (TYPE_OS==LINUX)
    fprintf(stdout, "[%ld] %s\n", static_cast<long int>(syscall(SYS_gettid)), buffer);
#else
#warning "Unsupported OS"
#endif
    fflush(stdout);
    va_end (args);
    pthread_mutex_unlock(&mutex);
#else
  ;
#endif
}

void hex_dump(const std::string& data)
{
   int offset;
   int is_text;
   unsigned int char_nbr;

   // Dump the message as text or binary
   is_text = 1;
   for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
       if (static_cast<unsigned char>(data[char_nbr]) < 32 || static_cast<unsigned char>(data[char_nbr]) > 127)
           is_text = 0;

   offset = sprintf(buf, "[%03d] ", static_cast<int>(data.size()));
   for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
   {
       if (is_text) 
       {
           strcpy(static_cast<char*>(&buf[0] + offset++), static_cast<const char*>(&data[char_nbr]));
       }
       else 
       {
           snprintf(&buf[offset], 3, "%02X", static_cast<unsigned char>(data[char_nbr]));
           offset += 2;
       }
   }
   buf[offset] = '\0';
   fprintf(stdout, "%s\n", buf); fflush(stdout);
}

char* hex_dump(const char* data, unsigned int size)
{
   int offset;
   int is_text;
   unsigned int char_nbr;

   // Dump the message as text or binary
   is_text = 1;
   for (char_nbr = 0; char_nbr < size; char_nbr++)
   {
       if (static_cast<unsigned char>(data[char_nbr]) < 32 
        || static_cast<unsigned char>(data[char_nbr]) > 127)
        {
           // Встречаются непечатные символы, выводим в двоичном виде
           is_text = 0;
           break;
        }
   }

   offset = sprintf(buf, "[%03d] ", size);
   for (char_nbr = 0; char_nbr < size; char_nbr++)
   {
       if (is_text) 
       {
           strcpy(static_cast<char*>(&buf[0] + offset++),
                  static_cast<const char*>(&data[char_nbr]));
       }
       else 
       {
           snprintf(&buf[offset], 3, "%02X", static_cast<unsigned char>(data[char_nbr]));
           offset += 2;
       }
   }
   buf[offset] = '\0';
   return buf;
}

bool getOption(Options* _options, const std::string& key, int& val)
{
  bool status = false;

  assert(_options);

  OptionIterator p = _options->find(key);

  if (p != _options->end())
  {
    val = p->second;
    status = true;
    //std::cout << "Found '"<<key<<"' option=" << p->second << std::endl;
  }
  return status;
}

bool setOption(Options* _options, const std::string& key, int val)
{
  bool status = false;

  assert(_options);

  OptionIterator p = _options->find(key);

  if (p != _options->end())
  {
    val = p->second;
    status = true;
//    std::cout << "Replace '" << key
//              << "' old value " << p->second
//              << " with " << val << std::endl;
  }
  else
  {
    _options->insert(Pair(key, val));
//    std::cout << "Insert into '" << key
//              << " new value " << val << std::endl;
    status = true;
  }
  return status;
}

