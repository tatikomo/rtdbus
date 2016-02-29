#pragma once
#ifndef GEV_HELPER_HPP
#define GEV_HELPER_HPP

#include <string>
#include <iterator>
#include <map>
#include <bitset>

#include "timer.hpp"

#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))

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
#if defined __SUN_CC
typedef std::pair<std::string, int> Pair;
typedef std::map<std::string, int> Options;
#else
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
#endif
typedef Options::iterator OptionIterator;

// Получить значение указанной опции из массива опций
bool  getOption(Options*, const std::string&, int&);
bool  setOption(Options*, const std::string&, int);

#ifdef __cplusplus
}
#endif

#endif
