#pragma once
#ifndef WORKER_DIGGER_HPP
#define WORKER_DIGGER_HPP

#include <string>

#include "config.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"
#include "mdp_letter.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

namespace mdp {

class Digger : public mdp::mdwrk
{
  public:
    Digger(std::string, std::string, int);
    ~Digger();

    int handle_request(mdp::zmsg*, std::string *&);
    int handle_read(mdp::Letter*, std::string*);
    int handle_write(mdp::Letter*, std::string*);

  private:
    int m_flag;
    xdb::RtApplication*  m_appli;
    xdb::RtEnvironment*  m_environment;
    xdb::RtConnection*   m_db_connection;
};

}; // namespace mdp

#endif
