/*  =========================================================================
    zmsg.hpp

    Multipart message class

    =========================================================================
*/

#pragma once
#ifndef __ZMSG_HPP_INCLUDED__
#define __ZMSG_HPP_INCLUDED__

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "mdp_helpers.hpp"

#include <vector>
#include <string>
#include <stdarg.h>

namespace mdp {

class zmsg 
{
 public:

   typedef enum {
     BROKER     = 1,
     SUBSCRIBER = 2,
     DIRECT     = 3,
     PEER       = 4
   } msg_source_t;

   zmsg();

   //  --------------------------------------------------------------------------
   //  Constructor, sets initial body
   zmsg(char const *body);

   //  -------------------------------------------------------------------------
   //  Constructor, sets initial body and sends message to socket
   zmsg(char const *body, zmq::socket_t &socket);

   //  --------------------------------------------------------------------------
   //  Constructor, calls first receive automatically
   zmsg(zmq::socket_t &socket);

   //  --------------------------------------------------------------------------
   //  Copy Constructor, equivalent to zmsg_dup
   zmsg(zmsg &msg);

   virtual ~zmsg();

   //  --------------------------------------------------------------------------
   //  Erases all messages
   void clear();

   void set_part(size_t part_nbr, unsigned char *data);
   // вернуть нужный фрейм по его номеру
   const std::string* get_part(size_t);

   bool recv(zmq::socket_t & socket);

   void send(zmq::socket_t & socket);

   size_t parts();

   void body_set(const char *body);

   void
   body_fmt (const char *format, ...);

   char * body ();

   // zmsg_push
   void push_front(char *part);
   void push_front(std::string& part);

   // zmsg_append
   void push_back(char *part);
   void push_back(std::string& part);

   //  --------------------------------------------------------------------------
   //  Formats 17-byte UUID as 33-char string starting with '@'
   //  Lets us print UUIDs as C strings and use them as addresses
   //
   char *
   encode_uuid (unsigned char*);

   // --------------------------------------------------------------------------
   // Formats 5-byte UUID as 10-char string starting with '00'
   // Lets us print UUIDs as C strings and use them as addresses
   unsigned char *
   decode_uuid (std::string&);

   const std::string front();

   // zmsg_pop
   std::string pop_front();

   void append (const char *part);

   char *address();

   void wrap(const char *address, const char *delim = 0);

   char * unwrap();

   void dump();

   // Источник получения сообщения определяется тем сокетом, откуда оно прочиталось
   // Это могут быть: Брокер, Подписка, Службы, Клиенты 
   void set_source(msg_source_t _src) { m_source = _src; };
   msg_source_t get_source() { return m_source; };

 private:
   DISALLOW_COPY_AND_ASSIGN(zmsg);
   std::vector<std::string> m_part_data;
   msg_source_t             m_source;
};

} //namespace mdp

#endif /* __ZMSG_HPP_INCLUDED__ */
