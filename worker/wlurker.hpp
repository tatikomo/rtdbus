#pragma once
#if !defined W_LURKER_H_
#define W_LURKER_H_

#include <string>

#include "config.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_letter.hpp"

class Letter;
class RtApplication;
class RtConnection;
class RtEnvironment;
class Letter;

class Lurker : public mdp::mdwrk
{
  public:
    Lurker(std::string, std::string, int);
    ~Lurker();

    int handle_request(mdp::zmsg*, std::string *&);
    int handle_read(mdp::Letter*, std::string*);
    int handle_write(mdp::Letter*, std::string*);

  private:
    int m_flag;
    xdb::rtap::RtApplication*  m_appli;
    xdb::core::Environment*  m_environment;
    xdb::core::Connection*   m_db_connection;
};

#endif
