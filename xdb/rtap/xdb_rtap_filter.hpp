#pragma once
#if !defined XDB_RTAP_POINT_FILTER_HPP
#define XDB_RTAP_POINT_FILTER_HPP

#include "config.h"
#include "xdb_core_error.hpp"

namespace xdb {

class RtPointFilter
{
  public:
    typedef enum 
    {
      // whole database subtree must be searched
      MATCH_SUBTREE,
      // only a certain level of the subtree must be searched
      MATCH_NTH_LEVEL,
      // only upto a certain level of the subtree must be searched
      MATCH_TO_NTH_LEVEL,
      // the subtree must be searched past a certain level
      MATCH_PAST_NTH_LEVEL,
      // matches points whose attributes have value less than a given value 
      MATCH_LESS,
      // matches points whose attributes have value equal to a given value 
      MATCH_EQUAL,
      // matches points whose attributes have value greater than a given value 
      MATCH_GREATER,
      // matches points whose attributes have value greater than or equal to a given value 
      MATCH_GREATER_OR_EQUAL,
      // matches points whose attributes have value not equal to a given value
      MATCH_NOT_EQUAL,
      // matches points whose attributes have value less than or equal to a given value
      MATCH_LESS_OR_EQUAL,
      // matches points which contain a given attribute
      MATCH_READABLE,
      // the attribute value to be compared against is a constant and is provided in the filter
      MATCH_CONSTANT
    } ScopeType;

    RtPointFilter();
    ~RtPointFilter();

    //
    // Создание фильтров по критериям
    //

    // Creates a query that searches for aliases using a regular expression
    static RtPointFilter* createAliasFilter();
    // Creates a filter that can be used to search for points using a point class ID
    static RtPointFilter* createClassFilter(int);
    // Creates a query that searches for point names using a regular expression
    static RtPointFilter* createPointNameFilter();
    static RtPointFilter* createAttributeTestFilter();

  private:
    DISALLOW_COPY_AND_ASSIGN(RtPointFilter);
    static ScopeType   m_scope_type;
    static Error     m_last_error;
};

} // namespace xdb

#endif

