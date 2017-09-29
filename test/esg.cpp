#include "config.h"

// Общесистемные заголовочные файлы
#include <sys/types.h>
#include <sys/stat.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_smed.hpp"
#include "exchange_egsa_translator.hpp"

// ==========================================================================================================
// Загрузка обменных словарей для известных Сайтов
// Прочитать набор словарей для своего объекта. Тип режима закодирован в названии словаря.
// Формат имени файла: {SND|ACQ}INFOS.<Код удаленного объекта>.json
// (SND)DIFFUSION - что локальный объект должен отправить
// ACQUISITION - что локальный объект хочет получить
int load_all_dictionaries(SMED* smed)
{
  int rc = NOK;
  char snd_dict_filename[255];
  char acq_dict_filename[255];
  struct stat configfile_info;
  char *dictionaries[2] = { snd_dict_filename, acq_dict_filename };
  SmedObjectList<AcqSiteEntry*>* sites = smed->SiteList();

  for(size_t i=0; i < sites->count(); i++) {

    AcqSiteEntry* site = (*sites)[i];

    if (site) {

      sprintf(snd_dict_filename, "SNDINFOS.%s.json", site->name());
      sprintf(acq_dict_filename, "ACQINFOS.%s.json", site->name());

      for (size_t idx=0; idx < 2; idx++) { // Попытаться загрузить два типа словаря для каждого Сайта

        // Если файл существует, загрузить его
        if (0 == stat(dictionaries[idx], &configfile_info)) {
          LOG(INFO) << "LOAD " << dictionaries[idx];
          rc = smed->load_dict(dictionaries[idx]);
          if (OK != rc) {
            // Не удалось загрузить словарь обмена с Сайтом
            // TODO: Исключить его из опроса? Или ограничиться минимальным набором данных (только свое состояние)
            LOG(ERROR) << "TODO: Exclude site " << site->name() << " due its loading dictionary error";
          }
          LOG(INFO) << site->name() << ": Dictionary " << dictionaries[idx] << " loading status=" << rc;
        }
        else {
          // Не найден, пропустим
        }
      }
    }

  }

  return rc;
}

// ================================================================================================================
int main(int argc, char** argv)
{
  EgsaConfig *config = NULL;
  SMED* smed = NULL;
  ExchangeTranslator *translator = NULL;
  int rc = NOK;
  esg_esg_odm_t_ExchInfoElem element;
  int led = 344;
  const char* filename;
//  const char* local_object_code = "K42663";

  ::google::InstallFailureSignalHandler();

  if (argc >= 2) {
    filename = argv[1];
    if (argc == 3)
      led = atoi(argv[2]);
  }
  else {
    filename = "ESG_REPLY1.DAT";
  }

  do
  {
    config = new EgsaConfig("egsa.json");
    rc = config->load();
    if (rc != OK) {
      LOG(ERROR) << "Config loading";
      break;
    }

    smed = new SMED(config, "SMED.sqlite");
    if (OK == (rc = smed->connect())) {
      rc = load_all_dictionaries(smed);
    }

    // Тест - прочитать LED=344 из словаря K42001
    if (OK == rc) {
      rc = smed->get_diff_info("K42001", led, &element);
    }

    if (OK == rc) {
      LOG(INFO) << "load led #" << led << " tag=\"" << element.s_Name << "\" objclass=" << element.infotype << ", rc=" << rc;
    }

    if (OK == rc) {
      translator = new ExchangeTranslator(smed,
                                          config->elemtypes(),
                                          config->elemstructs());

      rc = translator->esg_acq_dac_Switch(filename);
    }

    if (rc != OK) {
      LOG(ERROR) << "Sample translating";
      break;
    }
  }
  while (false);

  delete smed;
  delete config;
  delete translator;

  return rc;
}
