#include <string>
#include <vector>
#include <thread>
//#include <memory>
//#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "wrk_hist_sampler.hpp"
#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"
// интерфейс работы с БДРВ
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
// общий интерфейс сообщений
#include "msg_common.h"
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"

#include "hiredis.h"

// ---------------------------------------------------------------------
enum ProcessingType_t { NONE = 0, ANALOG = 1, DISCRETE = 2 };

// Структура хранения информации о семпле со средними значениями 
typedef struct {
  // универсальное имя точки
  std::string tag;
  // класс точки
  int objclass;
  // тип точки
  ProcessingType_t type;
  // набор средних значений VAL за соответствующие периоды
  common_value_t average[STAGE_LAST];
  // Набор значений атрибута VALID
  int valid[STAGE_LAST];
} points_objclass_list_t;

// ---------------------------------------------------------------------
// Признаки необходимости завершения работы группы процессов генератора предыстории
extern int interrupt_worker;
bool SamplerWorker::m_interrupt = false;
bool ArchivistSamplerProxy::m_interrupt = false;
static const char *COMMAND_TERMINATION = "TERMINATE";

// Списки Точек аналогового и дискретного типов.
// Используются в SamplerWorker-ах,
// NB: заполняются один раз при старте ArchivistSamplerProxy
xdb::map_id_name_t analog_points;
xdb::map_id_name_t discrete_points;
// вектор структур {тег + тип + objclass}
std::vector <points_objclass_list_t> points_objclass_list;

// Массив точек связи между собирателями и управляющей нитью
const char* pairs[] = {
  "inproc://pair_1_min",    // PER_1_MINUTE  = 0
  "inproc://pair_5_min",    // PER_5_MINUTES = 1
  "inproc://pair_hour",     // PER_HOUR      = 2
  "inproc://pair_day",      // PER_DAY       = 3
  "inproc://pair_month",    // PER_MONTH     = 4
  "inproc://pair_main",     // STAGE_LAST    = 5
};

// Массив переходов от текущего типа собирателя к следующему
const state_pair_t m_stages[STAGE_LAST] = {
  // текущая стадия (current)
  // |              сокет текующей стадии (pair_name_current)
  // |              |
  // |              |                    следующая стадия (next)
  // |              |                    |
  // |              |                    |              сокет следующей стадии (pair_name_next)
  // |              |                    |              |
  // |              |                    |              |                     анализируемых отсчетов
  // |              |                    |              |                     предыдущей стадии (num_prev_samples)
  // |              |                    |              |                     |
  // |              |                    |              |                     |  суффикс файла с семплом
  // |              |                    |              |                     |  текущей стадии (suffix)
  // |              |                    |              |                     |  |
  // |              |                    |              |                     |  |     длительность стадии
  // |              |                    |              |                     |  |     в секундах (duration)
  // |              |                    |              |                     |  |     |
  { PER_1_MINUTE,  pairs[PER_1_MINUTE],  PER_5_MINUTES, pairs[PER_5_MINUTES], 1, ".0", 60},
  { PER_5_MINUTES, pairs[PER_5_MINUTES], PER_HOUR,      pairs[PER_HOUR],      5, ".1", 300},
  { PER_HOUR,      pairs[PER_HOUR],      PER_DAY,       pairs[PER_DAY],      12, ".2", 3600},
  { PER_DAY,       pairs[PER_DAY],       PER_MONTH,     pairs[PER_MONTH],    24, ".3", 86400},
  { PER_MONTH,     pairs[PER_MONTH],     STAGE_LAST,    pairs[STAGE_LAST],   30, ".4", 2592000} // для 30 дней
};

// ---------------------------------------------------------------------
static int do_mkdir(const char *path, mode_t mode)
{
  struct stat  st;
  int status = 0;

  if (stat(path, &st) != 0)
  {
    /* Directory does not exist. EEXIST for race condition */
    if (mkdir(path, mode) != 0 && errno != EEXIST)
      status = -1;
  }
  else if (!S_ISDIR(st.st_mode))
  {
    errno = ENOTDIR;
    status = -1;
  }

  return(status);
}

/* ---------------------------------------------------------------------
 * mkpath - ensure all directories in path exist
 * Algorithm takes the pessimistic view and works top-down to ensure
 * each directory in path exists, rather than optimistically creating
 * the last element and working backwards.
 * ---------------------------------------------------------------------
 */
int mkpath(const char *path, mode_t mode)
{
  char           *pp;
  char           *sp;
  int             status;
  char           *copypath = strdup(path);

  status = 0;
  pp = copypath;
  while (status == 0 && (sp = strchr(pp, '/')) != 0)
  {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
  }

  if (status == 0)
      status = do_mkdir(path, mode);

  free(copypath);
  return (status);
}

// ---------------------------------------------------------------------
SamplerWorker::SamplerWorker(zmq::context_t& ctx,
                             sampler_type_t type,
                             xdb::RtEnvironment* env,
                             const char* db_hist_ip_addr,
                             int db_hist_port) :
  m_env(env),
  m_db_connection(NULL),
  m_context(ctx),
  m_history_db_connected(false),
  m_sampler_type(type),
//  m_cwd(get_current_dir_name()),
  m_history_db_address(db_hist_ip_addr),
  m_history_db_port(db_hist_port)
{
  LOG(INFO) << "Constructor SamplerWorker, type: " << m_sampler_type
            << ", context: " << &m_context;
 //           << ", cwd: " << m_cwd;
}

// ---------------------------------------------------------------------
SamplerWorker::~SamplerWorker()
{
//  delete m_cwd;
  delete m_db_connection;
  LOG(INFO) << "Destructor SamplerWorker, type: " << m_sampler_type
            << ", context " << &m_context;
}

// ---------------------------------------------------------------------
void SamplerWorker::work()
{
  // Для сигнализации о завершении текущих работ в адрес следующего семплера
  zmq::socket_t ignitor (m_context, ZMQ_PAIR);
  zmq::socket_t finitor (m_context, ZMQ_PAIR);

  // Подключиться с БДРВ только для сборщика минутной истории
  if (PER_1_MINUTE == m_sampler_type)
  {
    // Соединение будет удалено в деструкторе
    m_db_connection = m_env->getConnection();
    assert(m_db_connection->state() == xdb::CONNECTION_VALID);
  }

  try
  {
    // Для получения "пинка"
    ignitor.bind(m_stages[m_sampler_type].pair_name_current);
    // Для отправки "пинка"
    finitor.connect(m_stages[m_sampler_type].pair_name_next);
 
    LOG(INFO) << "SamplerWorker(" << m_sampler_type << ") ready";
    while(!m_interrupt)
    {
      // Дождаться сигнала о начале работы
      std::string command = s_recv(ignitor);

      if (0 == command.compare(COMMAND_TERMINATION))
      {
        LOG(INFO) << "SamplerWorker(" << m_sampler_type << ") shutting down";
        s_send(finitor, COMMAND_TERMINATION);
        break;
      }

      LOG(INFO) << "SamplerWorker(" << m_sampler_type << ") iteration";

      // Собрать историю указанного типа
      collect_sample();

      // Разрешить выполнение следующего сборщика предыстории
      s_send(finitor, "");
     
    }
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << "SamplerWorker catch: " << error.what();
  }
  catch (std::exception &e)
  {
      LOG(ERROR) << "SamplerWorker catch the signal: " << e.what();
  }

//  delete m_db_connection; m_db_connection = NULL;
  LOG(INFO) << "SamplerWorker::work finish";
}

// ---------------------------------------------------------------------
// Собрать требуемую m_sampler_type предысторию
void SamplerWorker::collect_sample()
{
  struct tm edge;
  const time_t current = time(0);

  localtime_r(&current, &edge);

  switch(m_sampler_type)
  {
    case PER_1_MINUTE:  // 0
      // Выгрузить атрибуты VAL и VALID всех Точек аналогового типа
      //process_analog_samples(current, analog_points);
      make_samples(current);
      break;

    case PER_5_MINUTES: // 1
      // Проверка на границу пяти минут (минуты нацело делятся на 5)
      if (!(edge.tm_min % 5)) {
        // Посчитать новый семпл из ранее изготовленных минутных
        make_samples(current);
      }
      break;

    case PER_HOUR:      // 2
      // Проверка на границу часа (минуты д.б. равны 0)
      if (0 == edge.tm_min) {
        make_samples(current);
      }
      break;

    case PER_DAY:       // 3
      // Проверка на границу суток (полночь - минуты и часы равны 0)
      if ((0 == edge.tm_min) && (0 == edge.tm_hour)) {
        make_samples(current);
      }
      break;

    case PER_MONTH:     // 4
      // Проверка на границу месяца (граница суток + первый день месяца)
      if ((0 == edge.tm_min) && (0 == edge.tm_hour) && (1 == edge.tm_mday)) {
        make_samples(current);
      }
      break;

    case STAGE_LAST:    // NB: break пропущен специально
    default:
      LOG(ERROR) << "Unsupported history sample: " << m_sampler_type;
      break;
  }
}

// ---------------------------------------------------------------------
// Создать новую запись в соответствующей таблице семплов истории
// 1) прочитать несколько последних отсчетов VAL и VALID архивов предыдущего цикла
// 2) найти среднее арифметическое, поделив на число семплов предыдущего цикла
// 3) добавить новый семпл в таблицу соответствующих семплов
void SamplerWorker::make_samples(const time_t timer)
{
  xdb::AttributeInfo_t info_read_val;
  xdb::AttributeInfo_t info_read_valid;
  xdb::Error status;
  // Итератор доступа к тегам
  std::vector <points_objclass_list_t>::iterator it;
  bool is_success = true;
  int num_items = 0;
  // структуры для работы с Redis
  redisContext *context = NULL;
  redisReply *reply = NULL;
  void* v_reply = &reply;
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  char str[200];
  // индекс текущего семпла предыдущего цикла
//  int num_sample = 0;

  context = redisConnectWithTimeout(m_history_db_address, m_history_db_port, timeout);
  if (context == NULL || context->err) {
    if (context) {
      LOG(ERROR) << "Redis connection error: " << context->errstr;
      redisFree(context);
    } else {
      LOG(ERROR) << "Connection error: can't allocate redis context";
    }
    return;
  }

  /* Проверить доступность сервера хранения истории Redis */
  reply = static_cast<redisReply*>(redisCommand(context,"PING"));
  if (0 == strcmp(reply->str, "PONG")) {
    LOG(INFO) << "Succesfully connected to history database at "
              << m_history_db_address << ":" << m_history_db_port;
    m_history_db_connected = true;
  }
  else {
    LOG(ERROR) << "Unable to get responce from history database at "
               << m_history_db_address << ":" << m_history_db_port;
  }
  freeReplyObject(reply);

  if (!m_history_db_connected)
    return;
    
  // Пройтись по всем точкам, аналоговым и дискретным
  for(it = points_objclass_list.begin(); it != points_objclass_list.end(); it++)
  {
    // Для всех типов семплов, кроме минутного, расчет основывается на ранее
    // сохраненных в предытории значениях. Для минутного семпла расчет ведется
    // на основе данных из БДРВ.
    //
    // 1) Выгрузить из предыстории набор записей по ключам:
    // <текущее время - N - 0>:<тип предыдущего семпла>:<тег>
    // <текущее время - N - 1>:<тип предыдущего семпла>:<тег>
    // ...
    // <текущее время - N - N>:<тип предыдущего семпла>:<тег>
    //
    // 2) найти среднее арифметическое атрибута VAL
    // 3) найденное ср.арифм. есть значение атрибута VAL текущего семпла
    // 4) найти максимальное значение атрибута VALID
    // 5) найденный максимум есть значение атрибута VALID текущего семпла 
    // 6) занести новый семпл в БД истории

    switch(m_sampler_type)
    {
      case PER_1_MINUTE: // 0
        LOG(INFO) << "edge of minute";
        // Минутный семпл, в отличие от остальных, в качестве входных данных использует БДРВ
        memset((void*)&info_read_val.value, '\0', sizeof(info_read_val.value));
        memset((void*)&info_read_valid.value, '\0', sizeof(info_read_valid.value));

        // Прочесть значения атрибутов VAL, VALID
        info_read_val.name = (*it).tag + "." + RTDB_ATT_VAL;
        status = m_db_connection->read(&info_read_val);
        if (status.code() != xdb::rtE_NONE) {
          LOG(ERROR) << "Reading " << info_read_val.name;
          is_success = false;
        }

        info_read_valid.name = (*it).tag + "." + RTDB_ATT_VALID;
        status = m_db_connection->read(&info_read_valid);
        if (status.code() != xdb::rtE_NONE) {
          LOG(ERROR) << "Reading " << info_read_valid.name;
          is_success = false;
        }

        // Открыть файл с шаблонным именем, основанным на названии тега.
        // Занести в него новую строку, содержащую:
        // 1) Текущую дату/время
        // 2) Числовое значение значения
        // 3) Числовое значение достоверности
        switch((*it).type)
        {
          case ANALOG:
            sprintf(str, "HMSET %ld:%d:%s VAL \"%g\" VALID %d",
                    timer,
                    (int)m_sampler_type,
                    (*it).tag.c_str(),
                    info_read_val.value.fixed.val_double,
                    info_read_valid.value.fixed.val_int16);
            break;

          case DISCRETE:
            sprintf(str, "HMSET %ld:%d:%s VAL %lld VALID %d",
                    timer,
                    (int)m_sampler_type,
                    (*it).tag.c_str(),
                    info_read_val.value.fixed.val_uint64,
                    info_read_valid.value.fixed.val_int16);
            break;

          case NONE:
          default:
            LOG(ERROR) << "Unsupported type: " << (*it).type;
         }

        // Добавить в очередь занесение очередного семпла
        redisAppendCommand(context, str);
        num_items++;
        break;

      case PER_5_MINUTES: // 1
        LOG(INFO) << "edge of 5 min";
        break;

      case PER_HOUR:      // 2
        LOG(INFO) << "edge of hour";
        break;

      case PER_DAY:       // 3
        LOG(INFO) << "edge of day";
        break;

      case PER_MONTH:     // 4
        LOG(INFO) << "edge of month";
        break;

      case STAGE_LAST:    // NB: break пропущен специально
      default:
        LOG(ERROR) << "Unsupported history sample: " << m_sampler_type;
        break;
    }








    // Связаные запросы создаются с помошью redisAppendCommand(context, строка запроса)
    // в цикле, затем вызываются с помощью redisCommand(), с разбором ответов
    while ((--num_items > 0) && redisGetReply(context, &v_reply) == REDIS_OK) {
      freeReplyObject(v_reply);
    }
  
    if (num_items) {
      LOG(ERROR) << "Part of samplings (" << num_items << " are not storing in history database";
      is_success = false;
    }

    if (true == is_success)
    {
      // Если не было ошибок, ограничимся общим уведомлением об успехе чтения
      LOG(INFO) << "Successfully sampling analog parameters";
    }

  }

  // Disconnects and frees the Redis context
  redisFree(context);
  LOG(INFO) << "Disconnected sampler(" << m_sampler_type << ") from history database at "
            << m_history_db_address << ":" << m_history_db_port;

}

// ---------------------------------------------------------------------
void SamplerWorker::stop()
{
  LOG(INFO) << "Set SamplerWorker's STOP sign";
  m_interrupt = true;
}

// ---------------------------------------------------------------------
/*
void SamplerWorker::process_analog_samples(struct tm& timer, xdb::map_id_name_t& id_list)
{
  xdb::AttributeInfo_t info_read_val;
  xdb::AttributeInfo_t info_read_valid;
  xdb::Error status;
  xdb::map_id_name_t::iterator it;
  bool is_success = true;
  int num_items = 0;
  // структуры для работы с Redis
  redisContext *context = NULL;
  redisReply *reply = NULL;
  void* v_reply = &reply;
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  char str[200];

  context = redisConnectWithTimeout(m_history_db_address, m_history_db_port, timeout);
  if (context == NULL || context->err) {
    if (context) {
      LOG(ERROR) << "Redis connection error: " << context->errstr;
      redisFree(context);
    } else {
      LOG(ERROR) << "Connection error: can't allocate redis context";
    }
    return;
  }

  // Проверить доступность сервера хранения истории Redis
  reply = static_cast<redisReply*>(redisCommand(context,"PING"));
  if (0 == strcmp(reply->str, "PONG")) {
    LOG(INFO) << "Succesfully connected to history database at "
              << m_history_db_address << ":" << m_history_db_port;
    m_history_db_connected = true;
  }
  else {
    LOG(ERROR) << "Unable to get responce from history database at "
               << m_history_db_address << ":" << m_history_db_port;
  }
  freeReplyObject(reply);

  for(it = id_list.begin(), num_items = 0; it != id_list.end(); it++)
  {
    memset((void*)&info_read_val.value, '\0', sizeof(info_read_val.value));
    memset((void*)&info_read_valid.value, '\0', sizeof(info_read_valid.value));

    // Прочесть значения атрибутов VAL, VALID
    info_read_val.name = (*it).second + "." + RTDB_ATT_VAL;
    status = m_db_connection->read(&info_read_val);
    if (status.code() == xdb::rtE_NONE)
    {
#ifdef VERBOSE
      std::cout << "Get "<< info_read_val.name <<"="<< info_read_val.value.fixed.val_double << std::endl;
#endif
    }
    else {
      LOG(ERROR) << "Reading " << info_read_val.name;
      is_success = false;
    }

    info_read_valid.name = (*it).second + "." + RTDB_ATT_VALID;
    status = m_db_connection->read(&info_read_valid);
    if (status.code() == xdb::rtE_NONE)
    {
#ifdef VERBOSE
      std::cout << "Get "<< info_read_valid.name <<"="<< info_read_valid.value.fixed.val_int16 << std::endl;
#endif
    }
    else {
      LOG(ERROR) << "Reading " << info_read_valid.name;
      is_success = false;
    }

    // Открыть файл с шаблонным именем, основанным на названии тега.
    // Занести в него новую строку, содержащую:
    // 1) Текущую дату/время
    // 2) Числовое значение значения
    // 3) Числовое значение достоверности
#if 0
    is_success = store_samples(context, timer, (*it).second, info_read_val, info_read_valid);
#else
    sprintf(str, "HMSET %ld:%d:%s VAL \"%g\" VALID %d",
             mktime(&timer),
             (int)m_sampler_type,
             (*it).second.c_str(),
             info_read_val.value.fixed.val_double,
             info_read_valid.value.fixed.val_int16);
//1    LOG(INFO) << "str: " << str;

    // Добавить в очередь занесение очередного семпла
    redisAppendCommand(context, str);
    num_items++;
#endif
    
  }
#ifdef VERBOSE
  std::cout << std::endl;
#endif

  // Связаные запросы создаются с помошью redisAppendCommand(context, строка запроса)
  // в цикле, затем вызываются с помощью redisCommand(), с разбором ответов
  while ((--num_items > 0) && redisGetReply(context, &v_reply) == REDIS_OK) {
    freeReplyObject(v_reply);
  }
  
  if (num_items) {
    LOG(ERROR) << "Part of samplings (" << num_items << " are not storing in history database";
    is_success = false;
  }

  if (true == is_success)
  {
    // Если не было ошибок, ограничимся общим уведомлением об успехе чтения
    LOG(INFO) << "Successfully sampling analog parameters";
  }

  // Disconnects and frees the Redis context
  redisFree(context);
  LOG(INFO) << "Disconnected sampler(" << m_sampler_type << ") from history database at "
            << m_history_db_address << ":" << m_history_db_port;

} 

const int SAMPLE_REDIS_KEY_LENGTH = 12 + 1 + 1 + 1 + TAG_NAME_MAXLEN;
const int SAMPLE_REDIS_VALUE_LENGTH = 40;
// ---------------------------------------------------------------------
// Поместить прочитанные значения VAL VALID точки tag в хранилище Redis
bool SamplerWorker::store_samples(void *context,
                                  struct tm& timer,
                                  std::string& tag,
                                  xdb::AttributeInfo_t& info_val,
                                  xdb::AttributeInfo_t& info_valid)
{
  // Результат работы
  bool status = true;
  redisReply *reply = NULL;
  size_t key_printed;
  size_t value_printed;
  redisContext *c = static_cast<redisContext*>(context);
  // строка для формирования ключа одного семпла в формате:
  // <time_t>:<тип семпла>:<тег>
  char sample_db_key[SAMPLE_REDIS_KEY_LENGTH + 1];
  // строка для формирования значения одного семпла в формате:
  // VAL "<число с плав. точкой>" VALID <число>"
  char sample_db_value[SAMPLE_REDIS_VALUE_LENGTH + 1];

  if (m_history_db_connected) {
    // Пример команды установки значения "hmset 1000000000:0:KD4005/5/RY05 VAL "3.14" VALID 1"
    key_printed = snprintf(sample_db_key, SAMPLE_REDIS_KEY_LENGTH,
             "%ld:%d:%s",
             mktime(&timer),
             (int)m_sampler_type,
             tag.c_str());

    value_printed = snprintf(sample_db_value, SAMPLE_REDIS_VALUE_LENGTH,
             "VAL \"%g\" VALID %d",
             info_val.value.fixed.val_double,
             info_valid.value.fixed.val_int16);

    reply = static_cast<redisReply*>(redisCommand(c, "HMSET %b %b",
             sample_db_key, key_printed,
             sample_db_value, value_printed));

    printf("HMSET %s %s\n", sample_db_key, sample_db_value);
    printf("HMSET: %s\n", reply->str);
    freeReplyObject(reply);
    abort();
  }
  else {
    status = false;
  }

  return status;
}

// ---------------------------------------------------------------------
// Поместить прочитанные значения VAL VALID точки tag в файловое хранилище
bool SamplerWorker::store_samples_in_files(std::string& tag,
                                  xdb::AttributeInfo_t& info_val,
                                  xdb::AttributeInfo_t& info_valid)
{
  // Результат работы
  bool status = true;
  // первый символ тега всегда = "/", потому индекс начнется с 1
  std::string::size_type found = 1;
  std::string file;
  std::string dir;
  std::string full_path;
  uint8_t valid = info_valid.value.fixed.val_uint8;
  int fd = -1;
  int code = -1;
  ssize_t bytes_written = 0;
  ssize_t bytes_printed = 0;
  char buf[200];
  time_t curr_timer = time(0);

  file.assign(tag, tag.find_last_of("/") + 1, tag.length());
  dir.assign(tag, found, tag.find_last_of("/") - 1);
//  std::cout << "dir=" << dir << ", file=" << file << std::endl;
  code = mkpath(dir.c_str(), 0755);
  if (0 == code)
  {
      full_path = dir + "/" + file + m_stages[m_sampler_type].suffix;
      if (-1 != (fd = open(full_path.c_str(), O_CREAT|O_WRONLY|O_APPEND, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)))
      {
        switch(info_val.type)
        {
          case xdb::DB_TYPE_LOGICAL:
            bytes_printed = sprintf(buf, "%ld;%d;%d\n", curr_timer, (unsigned int)info_val.value.fixed.val_bool, valid);
            break;
          case xdb::DB_TYPE_UINT8:
            bytes_printed = sprintf(buf, "%ld;%d;%d\n", curr_timer, (unsigned int)info_val.value.fixed.val_uint8, valid);
            break;
          case xdb::DB_TYPE_UINT16:
            bytes_printed = sprintf(buf, "%ld;%d;%d\n", curr_timer, info_val.value.fixed.val_uint16, valid);
            break;

          case xdb::DB_TYPE_FLOAT:
            bytes_printed = sprintf(buf, "%ld;%8.4f;%d\n", curr_timer, info_val.value.fixed.val_float, valid);
            break;
          case xdb::DB_TYPE_DOUBLE:
            bytes_printed = sprintf(buf, "%ld;%8.4g;%d\n", curr_timer, info_val.value.fixed.val_double, valid);
            break;

          case xdb::DB_TYPE_UNDEF:
          default:
            LOG(WARNING) << "Store unsupported data '" << tag << "', type: " << info_val.type;
            break;
        }

        bytes_written = write(fd, buf, bytes_printed);
        if (bytes_printed != bytes_written)
        {
          LOG(ERROR) << "Point " << tag << " store, should by written: "
                     << bytes_printed << ", actually: " << bytes_written;
        }

        if (-1 == close(fd))
        {
          LOG(ERROR) << "Closing file '" << full_path << "', fd=" << fd
                     << ", errno: " << errno << ", " << strerror(errno);
        }
      }
      else
      {
        std::cout << "open("<<full_path<<") errno="<<errno<<": " << strerror(errno) << std::endl;
      }
  }
  else
  {
    std::cout << "mkdir("<<dir<<")="<<code<<", strerror=" << strerror(errno) << std::endl;
  }

  return status;
}
*/

// ---------------------------------------------------------------------
ArchivistSamplerProxy::ArchivistSamplerProxy(zmq::context_t& ctx, xdb::RtEnvironment* env) :
  m_context(ctx),
  m_worker_list(),
  m_worker_thread(),
  m_env(env),
  m_db_connection(NULL),
  m_time_before({0, 0}),
  m_time_after({0, 0}),
  m_history_db_port(HISTDB_PORT_NUM)
{
  strcpy(m_history_db_address, HISTDB_IP_ADDRESS);

  LOG(INFO) << "Constructor ArchivistSamplerProxy, env: " << m_env;
}

// ---------------------------------------------------------------------
ArchivistSamplerProxy::~ArchivistSamplerProxy()
{
  int idx;

  LOG(INFO) << "Destructor ArchivistSamplerProxy, env: " << m_env;

  idx = 0;
  for (std::vector<SamplerWorker*>::iterator it_wrk = m_worker_list.begin();
       it_wrk!=m_worker_list.end(); it_wrk++)
  {
    (*it_wrk)->stop();

    delete (*it_wrk);

    LOG(INFO) << "Destroy SamplerWorker thread " << idx << " resource";
    idx++;
  }

  // Остановить нити SamplerWorker
  idx = 0;
  for (std::vector<std::thread*>::iterator it_thread = m_worker_thread.begin();
       it_thread != m_worker_thread.end();
       it_thread++)
  {
    LOG(INFO) << "Prepare to join SamplerWorker thread " << idx;
    (*it_thread)->join();
    LOG(INFO) << "End joining SamplerWorker thread" << idx;
    delete (*it_thread);
    idx++;
  }

  delete m_db_connection;
}

// ---------------------------------------------------------------------
void ArchivistSamplerProxy::stop()
{
  LOG(INFO) << "Set ArchivistSamplerProxy's STOP sign";
  m_interrupt = true;
}

// ---------------------------------------------------------------------
// Заполнить два набора, аналоговых и дискретных точек, их идентификаторами.
// Используется при чтении VAL и VALID при семплировании.
void ArchivistSamplerProxy::fill_points_list()
{
  xdb::rtDbCq operation;
  xdb::Error result;
  int idx;
  // Наборы Точек с заданным objclass
  // Используются для накопления результата в общих списках
  // аналоговых и дискретных параметров.
  xdb::map_id_name_t analog_points_per_object_type;
  xdb::map_id_name_t discrete_points_per_object_type;
  xdb::map_id_name_t::iterator map_iter;
  ProcessingType_t processing_type;
  points_objclass_list_t item;

  // Получить список идентификаторов Точек с заданным в addrCnt значением objclass
  operation.action.query = xdb::rtQUERY_PTS_IN_CLASS;

  for (idx = GOF_D_BDR_OBJCLASS_TS;
       idx < GOF_D_BDR_OBJCLASS_FIXEDPOINT;
       idx++)
  {
    discrete_points_per_object_type.clear();
    analog_points_per_object_type.clear();
    processing_type = NONE;

    switch(xdb::ClassDescriptionTable[idx].val_type)
    {
      case xdb::DB_TYPE_LOGICAL: // 1
      case xdb::DB_TYPE_INT8:    // 2
      case xdb::DB_TYPE_UINT8:   // 3
      case xdb::DB_TYPE_INT16:   // 4
      case xdb::DB_TYPE_UINT16:  // 5
      case xdb::DB_TYPE_INT32:   // 6
      case xdb::DB_TYPE_UINT32:  // 7
      case xdb::DB_TYPE_INT64:   // 8
      case xdb::DB_TYPE_UINT64:  // 9
        // Дискретное состояние данного типа Точки
        processing_type = DISCRETE;
        operation.buffer = &discrete_points_per_object_type;
        operation.addrCnt = xdb::ClassDescriptionTable[idx].code;
        result = m_db_connection->QueryDatabase(operation);
        break;

      case xdb::DB_TYPE_FLOAT:   // 10
      case xdb::DB_TYPE_DOUBLE:  // 11
        // Аналоговое состояние данного типа Точки
        processing_type = ANALOG;
        operation.buffer = &analog_points_per_object_type;
        operation.addrCnt = xdb::ClassDescriptionTable[idx].code;
        result = m_db_connection->QueryDatabase(operation);
        break;

      default:
        processing_type = NONE; // Ничего не делать
    }

    // Чтение набора точек выполнено успешно?
    if (result.Ok())
    {
      switch(processing_type)
      {
        case ANALOG:
          analog_points.insert(analog_points_per_object_type.begin(),
                               analog_points_per_object_type.end());
          // Добавить в общий список точек новую запись, определяемую типом и названием тега
          for(map_iter = analog_points_per_object_type.begin();
              map_iter != analog_points_per_object_type.end();
              map_iter++) {

            item.tag = (*map_iter).second;
            item.objclass = xdb::ClassDescriptionTable[idx].code;
            item.type = processing_type;

            LOG(INFO) << "Add analog point: " << item.tag << ", " << item.objclass << ", " << item.type;
            points_objclass_list.push_back(item);
          }
          break;

        case DISCRETE:
          discrete_points.insert(discrete_points_per_object_type.begin(),
                                 discrete_points_per_object_type.end());
          // Добавить в общий список точек новую запись, определяемую типом и названием тега
          for(map_iter = discrete_points_per_object_type.begin();
              map_iter != discrete_points_per_object_type.end();
              map_iter++) {

            item.tag = (*map_iter).second;
            item.objclass = xdb::ClassDescriptionTable[idx].code;
            item.type = processing_type;

            LOG(INFO) << "Add discrete point: " << item.tag << ", " << item.objclass << ", " << item.type;
            points_objclass_list.push_back(item);
          }
          break;

        case NONE: ; // nothing to do
      }
    }
    else
    {
      LOG(ERROR) << "Reading points with objclass "
                 << xdb::ClassDescriptionTable[idx].code
                 << " : " << result.what();
    }
  }
}

// ---------------------------------------------------------------------
// Запуск набора собирателей предыстории в отдельных нитях
void ArchivistSamplerProxy::run()
{
  // Время обработки запроса
  timer_mark_t processing_duration;
  // Секунд до начала новой минуты
  long awaiting;
  // Для сигнализации о начале круга архивирования
  zmq::socket_t ignitor (m_context, ZMQ_PAIR);
  // Для сигнализации о завершении круга архивирования
  zmq::socket_t finitor (m_context, ZMQ_PAIR);

  // Начать цепочку с минутного сборщика истории
  ignitor.connect(m_stages[PER_1_MINUTE].pair_name_current);
  // Этот сокет получит сообщение от последнего семплера, тем самым цикл архивирования замкнется
  finitor.bind(m_stages[PER_MONTH].pair_name_next);

  // Это соединение удаляется в деструкторе
  m_db_connection = m_env->getConnection();

  for (int type = PER_1_MINUTE; type < STAGE_LAST; type++)
  {
    SamplerWorker* p_worker = new SamplerWorker(m_context,
                        static_cast<sampler_type_t>(type),
                        m_env,
                        m_history_db_address,
                        m_history_db_port);
    std::thread *p_swt = new std::thread(std::bind(&SamplerWorker::work, p_worker));
    m_worker_list.push_back(p_worker);
    m_worker_thread.push_back(p_swt);

    LOG(INFO) << "Make thread SamplerWorker(" << type << ")";
  }

  // Заполнить списки точек аналогового и дискретного типов
  fill_points_list();

  while (!m_interrupt)
  {
    // TODO: проверять работоспособность нитей Собирателей
    LOG(INFO) << "ArchivistSamplerProxy iteration";

    // Дождаться начала интервала новой минуты
    while(!m_interrupt && ((awaiting = get_seconds_to_minute_edge()) > 0))
    {
      std::cout << "wait " << awaiting << " sec" << std::endl;
      // Разбить возможно большое время ощидания начала минуты
      // на секундные интервалы
      sleep(1);
    }

    // За время ожидания могла прийти команда завершения работы
    if (m_interrupt) break;

    // Начало рабочего цикла
    // ---------------------------------------------------
 
    // Запомнить время начала исполнения цепочки
    m_time_before = getTime();

    // Начать цепочку исполнения семплеров
    LOG(INFO) << "before sending start to ignitor"; //1
    s_send(ignitor, "");
    LOG(INFO) << "after sending start to ignitor";  //1

    // Блокирующее чтение вестей от последнего семплера в цепочке
    LOG(INFO) << "before receiving from finitor";   //1
    s_recv (finitor);
    LOG(INFO) << "after receiving from finitor";    //1

    // Запомнить время завершения цепочки
    m_time_after = getTime();

    // Вычислить длительность исполнения цепочки, для формирования правильной задержки
    processing_duration = timeDiff(m_time_before, m_time_after);

    std::cout << "duration = " << processing_duration.tv_sec << std::endl;
    // Конец рабочего цикла
    // NB: Если длительность цикла составила меньше секунды, есть вероятность
    // повторения цикла сразу же после завершения предыдущего.
    // ---------------------------------------------------
    if (0 == processing_duration.tv_sec)
    {
      LOG(WARNING) << "Sampling cycle ends too fast, wait a second";
      sleep(1); // Подождем секунду, чтобы не повторить цикл два (или больше) раза подряд
    }
  }
  // Отправить команду Собирателям на завершение работы
  LOG(INFO) << "before sending " << COMMAND_TERMINATION << " to ignitor";   //1
  s_send(ignitor, COMMAND_TERMINATION);
  LOG(INFO) << "after sending " << COMMAND_TERMINATION << " to ignitor";   //1
  LOG(INFO) << "before responce receiving from finitor";   //1
  std::string result = s_recv(finitor);
  LOG(INFO) << "after responce receiving from finitor";   //1

  LOG(INFO) << "STOP ArchivistSamplerProxy::run(): result=" << result;
}

// ---------------------------------------------------------------------
ArchivistSampler::ArchivistSampler(const std::string& endpoint, const std::string& name) :
  mdp::mdwrk(endpoint, name, 1 /* num zmq io threads (default = 1) */),
  m_broker_endpoint(endpoint),
  m_server_name(name),
  m_proxy(NULL),
  m_proxy_thread(NULL),
  m_message_factory(new msg::MessageFactory(HIST_SAMPLER_NAME)),
  m_appli(NULL),
  m_environment(NULL),
  m_db_connection(NULL)
{
  LOG(INFO) << "Constructor ArchivistSampler " << m_server_name
            << ", connect to " << m_broker_endpoint;

  m_appli = new xdb::RtApplication(HIST_SAMPLER_NAME);
  // TODO: к этому моменту БДРВ должна быть открыта Диггером, завершиться если это не так
  m_appli->setOption("OF_READONLY",1);      // Открыть БД только для чтения
}

// ---------------------------------------------------------------------
ArchivistSampler::~ArchivistSampler()
{
  LOG(INFO) << "Destructor ArchivistSampler";

  delete m_message_factory;
  delete m_db_connection;
  // NB: m_environment удаляется в RtApplication
  delete m_appli;
}

// --------------------------------------------------------------------------------
// Функция обрабатывает полученные служебные сообщения, не требующих подключения к БДРВ.
// Сейчас (2015.07.06) эта функция принимает запросы на доступ к БД, но не обрабатывает их.
int ArchivistSampler::handle_request(mdp::zmsg* request, std::string* reply_to)
{
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      case ADG_D_MSG_ASKLIFE:
        handle_asklife(letter, reply_to);
        break;

      case ADG_D_MSG_STOP:
        handle_stop(letter, reply_to);
        break;

      default:
        LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;
  return 0;
}

// --------------------------------------------------------------------------------
int ArchivistSampler::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  LOG(WARNING) << "Received message not yet supported received: ADG_D_MSG_STOP";
  return 0;
}

// --------------------------------------------------------------------------------
int ArchivistSampler::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), EMPTY_FRAME);

  LOG(INFO) << "Processing asklife from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  return 0;
}

// ---------------------------------------------------------------------
bool ArchivistSampler::init()
{
  bool status = false;

  m_appli->initialize();

  if (NULL != (m_environment = m_appli->attach_to_env(RTDB_NAME)))
  {
    // Каждая нить процесса, желающая работать с БДРВ, должна получить свой экземпляр
    m_db_connection = m_environment->getConnection();

    if (m_db_connection && (m_db_connection->state() == xdb::CONNECTION_VALID))
      status = true;

    LOG(INFO) << "RTDB status: " << m_environment->getLastError().what()
              << ", connection status: " << status;
  }
  else
  {
    LOG(ERROR) << "Couldn't attach to '" << RTDB_NAME << "'";
  }

  // TODO: проверить подключение к Redis

  return status;
}

// ---------------------------------------------------------------------
void ArchivistSampler::run()
{
  std::string *reply_to = NULL;
  LOG(INFO) << "begin ArchivistSampler RUN";

  interrupt_worker = false;

  try
  {
    m_proxy = new ArchivistSamplerProxy(m_context, m_environment);
    m_proxy_thread = new std::thread(std::bind(&ArchivistSamplerProxy::run, m_proxy));

    while(!interrupt_worker)
    {
        reply_to = new std::string;

        LOG(INFO) << "ArchivistSampler::recv() ready";
        // NB: Функция recv возвращает только PERSISTENT-сообщения
        mdp::zmsg *request = recv (reply_to);

        if (request)
        {
          LOG(INFO) << "ArchivistSampler::recv() got a message";

          handle_request (request, reply_to);

          delete request;
        }
        else
        {
          LOG(INFO) << "ArchivistSampler::recv() got a NULL";
          interrupt_worker = true; // has been interrupted
        }

        delete reply_to;
    }
  }
  catch(zmq::error_t error)
  {
    interrupt_worker = true;
    LOG(ERROR) << error.what();
  }
  catch(std::exception &e)
  {
    interrupt_worker = true;
    LOG(ERROR) << e.what();
  }

  // Послать в ArchivistSamplerProxy сигнал завершения работы
  m_proxy->stop();

  m_proxy_thread->join();
  delete m_proxy_thread;

  delete m_proxy;
  LOG(INFO) << "end ArchivistSampler RUN";
}

// ---------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int status = EXIT_SUCCESS;
  ArchivistSampler* sampler = NULL;
  int  opt;
  char service_name[SERVICE_NAME_MAXLEN + 1];
  char broker_endpoint[ENDPOINT_MAXLEN + 1];

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  // Значения по-умолчанию
  strcpy(broker_endpoint, ENDPOINT_BROKER);
  strcpy(service_name, HIST_SAMPLER_NAME);

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
         strncpy(broker_endpoint, optarg, ENDPOINT_MAXLEN);
         broker_endpoint[ENDPOINT_MAXLEN] = '\0';
         break;

       case 's': // название собственной Службы
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
         break;

       case '?':
         if (optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;

       default:
         abort ();
     }
  }

  std::string bro_endpoint(broker_endpoint);
  std::string srv_name(service_name);
  sampler = new ArchivistSampler(bro_endpoint, srv_name);

  if (true == sampler->init())
  {
    LOG(INFO) << "Initialization success";
    sampler->run();
    status = EXIT_SUCCESS;
  }
  else
  {
    LOG(ERROR) << "Initialization failure";
    status = EXIT_FAILURE;
  }

  delete sampler;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return status;
}
