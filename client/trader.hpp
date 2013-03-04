#if !defined GEV_TRADER_HPP
#define GEV_TRADER_HPP
#pragma once

#include "zmsg.hpp"
#include "mdp_common.h"
#include "trader.hpp"
#include "mdp_client_async_api.hpp"

class Trader: public mdcli
{
  public:
    Trader(std::string broker, int verbose);
};

#endif
