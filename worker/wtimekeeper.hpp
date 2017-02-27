#if !defined WRK_TIMEKEEPER_HPP_
#define WRK_TIMEKEEPER_HPP_

#include "config.h"

#include <string>
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"

// =======================================================================
// To allow multiple events to fire at the same instant, use a multimap,
// where keys are event times, and values are the events themselves.
// The multimap will then be sorted by time.
// Each time through the game loop, do something like this (pseudocode):
//
// now = getCurrentTime()
// while not events.isEmpty() and events.firstElement().key() < now:
//  e = events.firstElement().value
//  e.execute()
//  events.removeFirst()
// =======================================================================

class TimeKeeper : public mdp::mdwrk
{
  public:
    TimeKeeper(std::string broker, std::string service, int verbose) 
        : mdwrk(broker, service, 1) {};

    int handle_request(mdp::zmsg*, std::string *&);
};

#endif

