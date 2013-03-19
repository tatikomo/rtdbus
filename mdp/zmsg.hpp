/*  =========================================================================
    zmsg.hpp

    Multipart message class for example applications.

    Follows the ZFL class conventions and is further developed as the ZFL
    zfl_msg class.  See http://zfl.zeromq.org for more details.

    -------------------------------------------------------------------------
    =========================================================================
*/

#ifndef __ZMSG_HPP_INCLUDED__
#define __ZMSG_HPP_INCLUDED__

#include "mdp_helpers.hpp"

#include <vector>
#include <string>
#include <stdarg.h>

#if USE_USTRING
typedef std::basic_string<unsigned char> ustring;
#else
typedef std::basic_string<char> ustring;
#endif

class zmsg 
{
 public:

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

   bool recv(zmq::socket_t & socket);

   void send(zmq::socket_t & socket);

   size_t parts();

   void body_set(const char *body);

   void
   body_fmt (const char *format, ...);

   char * body ();

   // zmsg_push
   void push_front(char *part);

   // zmsg_append
   void push_back(char *part);

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
   decode_uuid (ustring&);

   const ustring front();

   // zmsg_pop
   ustring pop_front();

   void append (const char *part);

   char *address();

   void wrap(const char *address, const char *delim = 0);

   char * unwrap();

   void dump();

 private:
   std::vector<ustring> m_part_data;
};

#endif /* ZMSG_HPP_ */
