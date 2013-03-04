#ifndef __MDP_WORKER_DIGGER_HPP_INCLUDED__
#define __MDP_WORKER_DIGGER_HPP_INCLUDED__
#pragma once

#include <string>
#include "zmsg.hpp"
#include "mdp_worker_api.hpp"

class Digger : public mdwrk
{
  public:
    Digger(std::string broker, std::string service, int verbose) 
        : mdwrk(broker, service, verbose) {};

    int handle_request(zmsg* request, std::string* reply_to);
};

#endif
