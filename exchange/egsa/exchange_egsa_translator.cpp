#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <algorithm>
#include <map>
#include <istream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_translator.hpp"

//============================================================================
void print_elemstruct(std::pair<const std::string, elemstruct_item_t> pair)
{
  std::cout << "ES:" << pair.first << " ";
}
void print_elemtype(std::pair<const std::string, elemtype_item_t> pair)
{
  std::cout << "ET:" << pair.first << " ";
}
void print_locstruct(std::pair<const std::string, locstruct_item_t> pair)
{
  std::cout << "LS:" << pair.first << " ";
}


//============================================================================
ExchangeTranslator::ExchangeTranslator(locstruct_item_t* _locstructs, elemtype_item_t* _elemtypes, elemstruct_item_t* _elemstructs)
{
  LOG(INFO) << "CTOR ExchangeTranslator";
  locstruct_item_t* locstruct = _locstructs;
  elemtype_item_t* elemtype = _elemtypes;
  elemstruct_item_t* elemstruct = _elemstructs;

  while (elemtype->name[0]) {
//    LOG(INFO) << "elemtype: " << elemtype->name;
    m_elemtypes[elemtype->name] = *elemtype;
    elemtype++;
  }

  while (elemstruct->name[0]) {
//    LOG(INFO) << "elemstruct: " << elemstruct->name;
    m_elemstructs[elemstruct->name] = *elemstruct;
    elemstruct++;
  }

  while (locstruct->name[0]) {
//    LOG(INFO) << "locstruct: " << locstruct->name;
    m_locstructs[locstruct->name] = *locstruct;
    locstruct++;
  }
};

//============================================================================
ExchangeTranslator::~ExchangeTranslator()
{
  LOG(INFO) << "DTOR ExchangeTranslator";
/*
  std::for_each(m_elemtypes.begin(), m_elemtypes.end(), print_elemtype);
  std::cout << std::endl;
  std::for_each(m_elemstructs.begin(), m_elemstructs.end(), print_elemstruct);
  std::cout << std::endl;
  std::for_each(m_locstructs.begin(), m_locstructs.end(), print_locstruct);
  std::cout << std::endl;
*/
};

//============================================================================
// Прочитать из потока столько данных, сколько ожидается форматом Элементарного типа.
// Результат помещается в value.
int ExchangeTranslator::read_lex_value(std::stringstream& buffer, elemtype_item_t& elemtype, char* value)
{
  int rc = OK;
  int bytes_to_read = 0;
  char* point;
  char* mantissa;

  value[0] = '\0';
  switch (elemtype.tm_type) {
    case TM_TYPE_INTEGER:   // I
      // <число>
      bytes_to_read = atoi(elemtype.size);
      break;
    case TM_TYPE_TIME:      // T
      // U<число>
      bytes_to_read = atoi(elemtype.size+1); // пропустить первый символ 'U'
      break;
    case TM_TYPE_STRING:    // S
      // <число>
      bytes_to_read = atoi(elemtype.size);
      break;
    case TM_TYPE_REAL:      // R
      // <цифра>.<цифра> или <число>e<цифра>
      if (NULL!= (point = strchr(elemtype.size, '.'))) {
        // Убедимся в том, что формат соотвествует <цифра>.<цифра>
        assert(ptrdiff_t(point - elemtype.size) == 1);
        // Формат с фиксированной точкой
        bytes_to_read += elemtype.size[0] - '0';    // <цифра>
        bytes_to_read += 1;                         // '.'
        bytes_to_read += elemtype.size[2] - '0';    // <цифра>
      }
      else {
        // Формат с плавающей точкой
        if (NULL != (mantissa = strchr(elemtype.size, 'e'))) {
          char t[10];
          strncpy(t, elemtype.size, ptrdiff_t(mantissa - elemtype.size) + 1);
          bytes_to_read += atoi(t);             // мантисса
          bytes_to_read += 1;                   // 'e'
          bytes_to_read += atoi(mantissa+1);    // порядок
        }
        else {
          LOG(ERROR) << elemtype.name << ": unsupported size \"" << elemtype.size << "\"";
        }
      }
      break;
    case TM_TYPE_LOGIC:     // L
      // <число> (всегда = 1)
      bytes_to_read = atoi(elemtype.size);
      assert(bytes_to_read == 1);
      break;
    default:
      LOG(ERROR) << elemtype.name << ": unsupported type " << elemtype.tm_type;
      rc = NOK;
  }

  if (bytes_to_read)
    buffer.get(value, bytes_to_read + 1);

  LOG(INFO) << "read '" << value << "' bytes:" << bytes_to_read+1 << " [name:" << elemtype.name << " type: " << elemtype.tm_type << " size:" << elemtype.size << "]";


  return rc;
}

//============================================================================
int ExchangeTranslator::load(std::stringstream& buffer)
{
  static const char* fname = "ExchangeTranslator::load";
  int rc = OK;
  std::string line;
  char service[10];
  char elemstruct[10];
  char elemtype[10];
  char lexem[10];   // лексема, могущая оказаться как UN?, так и C????
  char value[100];
  std::map <const std::string, elemstruct_item_t>::iterator it_es;
  std::map <const std::string, elemtype_item_t>::iterator it_et;
  std::map <const std::string, locstruct_item_t>::iterator it_ls;

  // Заголовок
  while ((OK == rc) && buffer.good()) {

    buffer.get(service, 3+1);
    // Может быть:
    // UNB    Interchange Header Segment
    // UNH    Message Header segment
    // UNT    Message End Segment
    // UNZ    Interchange End segment
    // APB    Applicative segment header 
    // APE
    if (0 == strcmp(service, "UNB")) {
      LOG(INFO) << service << ": Interchange Header Segment";
    } else if (0 == strcmp(service, "UNH")) {
      LOG(INFO) << service << ": Message Header Segment";
    } else if (0 == strcmp(service, "UNT")) {
      LOG(INFO) << service << ": Message End Segment";
    } else if (0 == strcmp(service, "UNZ")) {
      LOG(INFO) << service << ": Interchange End segment";
    } else if (0 == strcmp(service, "APB")) {
      LOG(INFO) << service << ": Applicative segment header";
    } else if (0 == strcmp(service, "APE")) {
      LOG(INFO) << service << ": ? segment";
    } else {
      LOG(ERROR) << service << ": unsupported segment";
      rc = NOK;
      break;
    }

    while ((OK == rc) && buffer.good()) {

      buffer.get(elemstruct, 5+1);
      // Может быть:
      // UNB    Interchange Header Segment
      // UNH    Message Header segment
      // UNT    Message End Segment
      // UNZ    Interchange End segment
      // APB    Applicative segment header 
      // APE
      // C????

      it_es = m_elemstructs.find(elemstruct);

      if (it_es != m_elemstructs.end()) {

        std::cout << elemstruct << std::endl;

        // Читать все элементарные типы, содержащиеся в (*it_es)
        for (size_t idx = 0; idx < (*it_es).second.num_fileds; idx++) {

          buffer.get(elemtype, 5+1);
          it_et = m_elemtypes.find(elemtype);

          if (it_et != m_elemtypes.end()) {
            if (0 == strcmp((*it_et).second.name, (*it_es).second.fields[idx])) {

              // Прочитать данные этого элементарного типа
              rc = read_lex_value(buffer, (*it_et).second, value);
            }
            else {
              // Прочитали лексему, не принадлежащую текущей elemstruct
              LOG(ERROR) << "expect:" << (*it_es).second.fields[idx] << ", read:" << elemtype;
              rc = NOK;
//              break;
            }
          }
          else {
            // Прочитали неизвестную лексему
            LOG(ERROR) << "expect:" << (*it_es).second.fields[idx] << ", read unknown:" << elemtype;
            rc = NOK;
//            break;
          } // конец блока if "не нашли элементарный тип"

        } // конец цикла чтения всех полей текущего композитного типа elemstruct

      } // конец блока if "этот композитный тип известен"
      else {
        LOG(ERROR) << elemstruct << ": unsupported element";
      }

    } // конец цикла "пока данные не закончились"

//    } // конец блока if "корректный заголовок"
//    else {
//      LOG(ERROR) << fname << ": missing UNB header";
//      rc = NOK;
//    }
  }

  return rc;
}


//============================================================================
