#ifndef __MDP_WORKER_ECHO_HPP_INCLUDED__
#define __MDP_WORKER_ECHO_HPP_INCLUDED__
#pragma once

#include <string>
#include "zmsg.hpp"
#include "mdp_worker_api.hpp"

class Echo : public mdp::mdwrk
{
  public:
    Echo(std::string broker, std::string service, int verbose) 
        : mdwrk(broker, service, verbose) {};

    int handle_request(mdp::zmsg*, std::string *&);
};

#endif
