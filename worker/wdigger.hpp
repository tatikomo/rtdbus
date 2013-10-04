#ifndef __MDP_WORKER_DIGGER_HPP_INCLUDED__
#define __MDP_WORKER_DIGGER_HPP_INCLUDED__
#pragma once

#include <string>
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"

class Letter;

class Digger : public mdp::mdwrk
{
  public:
    Digger(std::string broker, std::string service, int verbose) 
        : mdwrk(broker, service, verbose) {};
    ~Digger() {};

    int handle_rtdbus_message(mdp::Letter*, std::string*);
    int handle_request(mdp::zmsg*, std::string *&);
    int handle_sell_request (std::string&, std::string&, std::string *);
    int handle_buy_request (std::string&, std::string&, std::string *);
};

#endif
