#include <assert.h>
#include <string>

#include "config.h"

#include "xdb_core_error.hpp"
#include "xdb_rtap_filter.hpp"

using namespace xdb;

Error RtPointFilter::m_last_error;
RtPointFilter::ScopeType RtPointFilter::m_scope_type;

RtPointFilter::RtPointFilter()
{
}

RtPointFilter::~RtPointFilter()
{
}

// Creates a query that searches for aliases using a regular expression
RtPointFilter* RtPointFilter::createAliasFilter()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Creates a filter that can be used to search for points using a point class ID
RtPointFilter* RtPointFilter::createClassFilter(int)
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

// Creates a query that searches for point names using a regular expression
RtPointFilter* RtPointFilter::createPointNameFilter()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

RtPointFilter* RtPointFilter::createAttributeTestFilter()
{
  m_last_error.set(rtE_NOT_IMPLEMENTED);
  return NULL;
}

