#if !defined GEV_PULSAR_HPP
#define GEV_PULSAR_HPP
#pragma once

#include "zmsg.hpp"
#include "mdp_client_async_api.hpp"

class Pulsar: public mdcli
{
  public:
    Pulsar(std::string broker, int verbose);
};

#endif
