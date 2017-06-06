#ifndef EXCHANGE_EGSA_TRANSLATOR_HPP
#define EXCHANGE_EGSA_TRANSLATOR_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <map>
#include <istream>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_impl.hpp"
// Конфигурация

//============================================================================
class ExchangeTranslator
{
  public:
    ExchangeTranslator(locstruct_item_t*, elemtype_item_t*, elemstruct_item_t*);
   ~ExchangeTranslator();

    int load(std::stringstream&);

  private:
    DISALLOW_COPY_AND_ASSIGN(ExchangeTranslator);
    std::map <const std::string, locstruct_item_t>  m_locstructs;
    std::map <const std::string, elemtype_item_t>   m_elemtypes;
    std::map <const std::string, elemstruct_item_t> m_elemstructs;

    // Прочитать из потока столько данных, сколько ожидается форматом Элементарного типа
    int read_lex_value(std::stringstream&, elemtype_item_t&, char*);
};

//============================================================================
#endif
