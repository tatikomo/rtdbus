#pragma once
#ifndef MDP_WORKER_TESTER_HPP_INCLUDED
#define MDP_WORKER_TESTER_HPP_INCLUDED

#include <string>
#include "mdp_zmsg.hpp"
#include "msg_message.hpp"
#include "mdp_worker_api.hpp"

class Letter;

class Tester : public mdp::mdwrk
{
  public:
    Tester(const std::string&, const std::string&, int);
    ~Tester();

    int handle_rtdbus_message(msg::Letter*, std::string*);
    int handle_request(mdp::zmsg*, std::string *&);
    int handle_sell_request (std::string&, std::string&, std::string *);
    int handle_buy_request (std::string&, std::string&, std::string *);

  private:
    msg::MessageFactory* m_message_factory;
};

#endif
