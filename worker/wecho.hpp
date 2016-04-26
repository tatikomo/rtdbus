#pragma once
#ifndef __MDP_WORKER_ECHO_HPP_INCLUDED__
#define __MDP_WORKER_ECHO_HPP_INCLUDED__

#include <string>
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"

class Echo : public mdp::mdwrk
{
  public:
    Echo(std::string broker, std::string service, int verbose) 
        : mdwrk(broker, service, 1 /* num threads */) {};

    int handle_request(mdp::zmsg*, std::string *&);
};

#endif
