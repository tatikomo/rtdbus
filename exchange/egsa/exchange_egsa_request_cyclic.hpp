#if !defined EXCHANGE_EGSA_REQUEST_CYCLIC_HPP
#define EXCHANGE_EGSA_REQUEST_CYCLIC_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exchange_egsa_request_base.hpp"

class CyclicRequest : public BaseRequest {
  public:
    CyclicRequest(ech_t_ReqId req_id)
      : BaseRequest(CYCLICREQ, req_id)
    {};

   ~CyclicRequest()
    {};
};

#endif

