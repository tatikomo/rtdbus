#pragma once
#ifndef GEV_HELPER_HPP
#define GEV_HELPER_HPP

#include <string>
#include <map>
#include <bitset>

#include "timer.hpp"

void  hex_dump(const std::string&);
char* hex_dump(const char*, unsigned int);

#ifdef __cplusplus
extern "C" {
#endif


typedef std::bitset<8> BitSet8;

void LogError(int rc,
            const char *functionName,
            const char *format,
            ...);


/*void LogWarn(
            const char *functionName,
            const char *format,
            ...);

void LogInfo(
            const char *functionName,
            const char *format,
            ...);*/


// Описание хранилища опций в виде карты.
// Пара <символьный ключ> => <числовое значение>
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
typedef std::map<const std::string, int>::iterator OptionIterator;

// Получить значение указанной опции из массива опций
bool  getOption(Options&, const std::string&, int&);
bool  setOption(Options&, const std::string&, int);

#ifdef __cplusplus
}
#endif

#endif
