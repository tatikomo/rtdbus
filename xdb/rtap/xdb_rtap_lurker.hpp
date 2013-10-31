#if !defined XDB_RTA_LURKER_H_
#define XDB_RTA_LURKER_H_
#pragma once

#include <string>

#include "config.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"


class Letter;
class RtApplication;
class RtDbConnection;
class RtEnvironment;

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
    xdb::RtApplication*  m_appli;
    xdb::RtEnvironment*  m_environment;
    xdb::RtDbConnection* m_db_connection;
};

#endif
