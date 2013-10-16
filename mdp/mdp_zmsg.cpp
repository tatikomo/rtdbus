/*  =========================================================================
 zmsg.hpp

 Multipart message class for example applications.

 Follows the ZFL class conventions and is further developed as the ZFL
 zfl_msg class.  See http://zfl.zeromq.org for more details.

 -------------------------------------------------------------------------
 =========================================================================
*/

#include <vector>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <glog/logging.h>

#include "config.h"
#include "helper.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_common.h"

using namespace mdp;

zmsg::zmsg()
{
}

//  --------------------------------------------------------------------------
//  Constructor, sets initial body
zmsg::zmsg(char const *body)
{
    body_set(body);
}

//  -------------------------------------------------------------------------
//  Constructor, sets initial body and sends message to socket
zmsg::zmsg(char const *body, zmq::socket_t &socket)
{
    body_set(body);
    send(socket);
}

//  --------------------------------------------------------------------------
//  Constructor, calls first receive automatically
zmsg::zmsg(zmq::socket_t &socket)
{
    recv(socket);
}

//  --------------------------------------------------------------------------
//  Copy Constructor, equivalent to zmsg_dup
zmsg::zmsg(zmsg &msg)
{
    m_part_data.resize(msg.m_part_data.size());
    std::copy(msg.m_part_data.begin(), msg.m_part_data.end(), m_part_data.begin());
}

zmsg::~zmsg() {
   clear();
}

//  --------------------------------------------------------------------------
//  Erases all messages
void zmsg::clear() {
//    std::cerr <<  "on " << this << " zmsg::clear() " << parts() << " parts" << std::endl;
    m_part_data.clear();
}

void zmsg::set_part(size_t part_nbr, unsigned char *data) {
    if (part_nbr < m_part_data.size() /*&& part_nbr >= 0*/) {
        m_part_data[part_nbr] = (char*)data;
    }
}

const std::string* zmsg::get_part(size_t part_nbr) {
    if (part_nbr < m_part_data.size() /*&& part_nbr >= 0*/) {
        return &m_part_data[part_nbr];
    }
    // индекс невалиден
    return NULL;
}


bool zmsg::recv(zmq::socket_t & socket) {
   int more;
   int iteration = 0;
   size_t more_size = sizeof(more);
   char *uuidstr = NULL;

   clear();
   while(1)
   {
      iteration++;
      zmq::message_t message(0);
      try {
         if (!socket.recv(&message, 0)) {
            LOG(WARNING) << "zmsg::recv() false";
            return false;
         }
      } catch (zmq::error_t error) {
         LOG(ERROR) << "Catch '" << error.what() << "', code=" << error.num();
         return false;
      }
      std::string data;
      data.assign(static_cast<const char*>(message.data()), message.size());
      data[message.size()] = 0;

      /*
       * В первом пакете приходит идентификатор отправителя в бинарном формате.
       * Длина фрейма 5 байтов, начинается с 0.
       * Конвертируем его в UUID, 10 символов.
       */
      if ((iteration == 1)
/* [GEV:генерация GUID] */    && (message.size() == 5)
      && data[0] == 0) {
         uuidstr = encode_uuid((unsigned char*) message.data());
         push_back(uuidstr);
         delete[] uuidstr;
      }
      else
      {
         push_back(data);
//         LOG(ERROR)<<"R "<<iteration<<"/"<<m_part_data.size()
//                   <<" dsize="<<data.size()
//                   <<" msize="<<message.size()
//                   <<" "<<hex_dump(static_cast<const char*>(data.data()), data.size());
      }
      socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
      if (!more) {
         break;
      }
   }
   return true;
}

void zmsg::send(zmq::socket_t & socket)
{
  unsigned char * uuidbin;

    for (size_t part_nbr = 0; part_nbr < m_part_data.size(); part_nbr++)
    {
       zmq::message_t message;
       std::string data = m_part_data[part_nbr];

//       LOG(ERROR)<<"s "<<part_nbr+1<<"/"<<m_part_data.size()
//                 <<" dsize="<<data.size()
//                 <<" "<<hex_dump(static_cast<const char*>(message.data()), message.size());

       /*
        * Первый фрейм - символьное представление адреса получателя.
        * Необходимо обратное преобразование в двоичное представление.
        * 11 байт - это первый символ '@' + 10 байт представления Адресата (identity)
        */
       if (part_nbr == 0 && data.size() == IDENTITY_MAXLEN && data [0] == '@')
       {
          uuidbin = decode_uuid(data);
          message.rebuild(5);
          memcpy(message.data(), uuidbin, 5);
          delete[] uuidbin;
       }
       else 
       {
          message.rebuild(data.size());
          memcpy(message.data(), data.data(), data.size());
//          LOG(ERROR)<<"S "<<part_nbr+1<<"/"<<m_part_data.size()
//                 <<" dsize="<<data.size()
//                 <<" "<<hex_dump(static_cast<const char*>(message.data()), message.size());
       }

       try
       {
          socket.send(message, part_nbr < m_part_data.size() - 1 ? ZMQ_SNDMORE : 0);
       }
       catch (zmq::error_t error)
       {
          assert(error.num()!=0);
       }
    }
    clear();
}

size_t zmsg::parts() {
   return m_part_data.size();
}

void zmsg::body_set(const char *body) {
   if (m_part_data.size() > 0) {
      m_part_data.erase(m_part_data.end()-1);
   }
   push_back((char*)body);
}

void zmsg::body_fmt (const char *format, ...)
{
    char value [255 + 1];
    va_list args;

    va_start (args, format);
    vsnprintf (value, 255, format, args);
    va_end (args);

    body_set (value);
}

char * zmsg::body ()
{
    if (m_part_data.size())
        return ((char *) m_part_data [m_part_data.size() - 1].c_str());
    else
        return 0;
}

// zmsg_push
void zmsg::push_front(char *part) {
   m_part_data.insert(m_part_data.begin(), part);
}

void zmsg::push_front(std::string& part) {
   m_part_data.insert(m_part_data.begin(), part);
}

// zmsg_append
void zmsg::push_back(char *part) {
   m_part_data.push_back(part);
}

void zmsg::push_back(std::string& part) {
   m_part_data.push_back(part);
}



//  --------------------------------------------------------------------------
//  Formats 5-byte UUID as 11-char string starting with '@'
//  Lets us print UUIDs as C strings and use them as addresses
//
char *
zmsg::encode_uuid (unsigned char* data)
{
    static char
        hex_char [] = "0123456789ABCDEF";

    assert (data);
    assert (data[0] == '\0');

    if ((NULL == data) || ('\0' != data[0]))
    {
      LOG(ERROR) << "Try to encode empty string";
      return NULL;
    }

    char *uuidstr = new char[IDENTITY_MAXLEN + 1];
    uuidstr [0] = '@';
    int byte_nbr;
    for (byte_nbr = 0; byte_nbr < 5; byte_nbr++) {
        uuidstr [byte_nbr * 2 + 1] = hex_char [data [byte_nbr + 0] >> 4];
        uuidstr [byte_nbr * 2 + 2] = hex_char [data [byte_nbr + 0] & 15];
    }
    uuidstr [IDENTITY_MAXLEN] = 0;
    return (uuidstr);
}


// --------------------------------------------------------------------------
// Formats 5-byte UUID as 11-char string starting with '@'
// Lets us print UUIDs as C strings and use them as addresses
//
unsigned char *
zmsg::decode_uuid (std::string& uuidstr)
{
    static char
        hex_to_bin [128] = {
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1, /* 0..9 */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* A..F */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* a..f */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }; /* */

    assert (uuidstr.size() == IDENTITY_MAXLEN);
    assert (uuidstr [0] == '@');
    unsigned char *data = new unsigned char[5+1];
    int byte_nbr;
    data [0] = 0;
    for (byte_nbr = 0; byte_nbr < 5; byte_nbr++)
        data [byte_nbr + 0]
            = (hex_to_bin [uuidstr [byte_nbr * 2 + 1] & 0x7F] << 4)
            + (hex_to_bin [uuidstr [byte_nbr * 2 + 2] & 0x7F]);

    return (data);
}

const std::string zmsg::front() {
  return m_part_data.front();
}

// zmsg_pop
std::string zmsg::pop_front() {
   if (m_part_data.empty()) {
      return 0;
   }
   std::string part = m_part_data.front();
   m_part_data.erase(m_part_data.begin());
   return part;
}

void zmsg::append (const char *part)
{
    assert (part);
    push_back((char*)part);
}


/* 
 * Двоичное представление Адресата содержит символы '\0', однако при приеме
 * оно конвертируется в символьную форму (5 двоичных байт в '@'+10 символьных байт).
 */
char *zmsg::address() {
   if (m_part_data.size() > 0) {
      return (char*)m_part_data[0].c_str();
   } else {
      return 0;
   }
}


void zmsg::wrap(const char *address, const char *delim) {
   if (delim) {
      push_front(const_cast<char*>(delim));
   }
   push_front(const_cast<char*>(address));
}


char * zmsg::unwrap() 
{
   if (m_part_data.empty()) {
      return NULL;
   }

   std::string addr_str = pop_front();
   char *addr = new char[addr_str.size()+1];
   memcpy(addr, addr_str.data(), addr_str.size());
   addr[addr_str.size()] = '\0';

   if (address() && *address() == 0) {
      pop_front();
   }
   return addr;
}


// Вывод содержимого zmq
// TODO: содержит опасные трюки с указателями, необходимо переделать
void zmsg::dump()
{
   char buf[4000];
   int offset;
   int is_text;
   unsigned int char_nbr;
   unsigned int part_nbr;

   LOG(INFO) << "--------------------------------------";

   for (part_nbr = 0; part_nbr < m_part_data.size(); part_nbr++) 
   {
       std::string data = m_part_data [part_nbr];
       // Dump the message as text or binary
       is_text = 1;
       for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
           if (static_cast<unsigned char>(data[char_nbr]) < 32 
            || static_cast<unsigned char>(data[char_nbr]) > 127)
               is_text = 0;

       offset = sprintf(buf, "[%03d] ", (int) data.size());
       for (char_nbr = 0; char_nbr < data.size(); char_nbr++)
       {
           if (is_text) 
           {
               strcpy((char*)(&buf[0] + offset++), (const char*) &data [char_nbr]);
           }
           else 
           {
               snprintf(&buf[offset], 3, "%02X", (unsigned char)data[char_nbr]);
               offset += 2;
           }
       }
       buf[offset] = '\0';
       LOG(INFO) << buf;
   }
}

