#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_config_egsa.hpp"
#include "xdb_common.hpp"

// ==========================================================================================================
ega_ega_odm_t_RequestEntry EGSA::m_requests_table[] = {
// Идентификатор запроса
// |                    Название запроса
// |                    |                        Приоритет
// |                    |                        |   Тип объекта - информация, СС, оборудование
// |                    |                        |   |       Режим сбора - дифференциальный или нет
// |                    |                        |   |       |              Признак отношения запроса:
// |                    |                        |   |       |              к технологическим данным (1)
// |                    |                        |   |       |              к состоянию системы сбора (0)
// |                    |                        |   |       |              | 
  {ECH_D_GENCONTROL,    EGA_EGA_D_STRGENCONTROL, 80, ACQSYS, NONDIFF,       true  },
  {ECH_D_INFOSACQ,      EGA_EGA_D_STRINFOSACQ,   80, ACQSYS, DIFF,          true  },
  {ECH_D_URGINFOS,      EGA_EGA_D_STRURGINFOS,   80, ACQSYS, DIFF,          true  },
  {ECH_D_GAZPROCOP,     EGA_EGA_D_STRGAZPROCOP,  80, ACQSYS, NONDIFF,       false },
  {ECH_D_EQUIPACQ,      EGA_EGA_D_STREQUIPACQ,   80, EQUIP,  NONDIFF,       true  },
  {ECH_D_ACQSYSACQ,     EGA_EGA_D_STRACQSYSACQ,  80, ACQSYS, NONDIFF,       false },
  {ECH_D_ALATHRES,      EGA_EGA_D_STRALATHRES,   80, ACQSYS, DIFF,          true  },
  {ECH_D_TELECMD,       EGA_EGA_D_STRTELECMD,   101, INFO,   NOT_SPECIFIED, true  },
  {ECH_D_TELEREGU,      EGA_EGA_D_STRTELEREGU,  101, INFO,   NOT_SPECIFIED, true  },
  {ECH_D_SERVCMD,       EGA_EGA_D_STRSERVCMD,    80, ACQSYS, NOT_SPECIFIED, false },
  {ECH_D_GLOBDWLOAD,    EGA_EGA_D_STRGLOBDWLOAD, 80, ACQSYS, NOT_SPECIFIED, false },
  {ECH_D_PARTDWLOAD,    EGA_EGA_D_STRPARTDWLOAD, 80, ACQSYS, NOT_SPECIFIED, false },
  {ECH_D_GLOBUPLOAD,    EGA_EGA_D_STRGLOBUPLOAD, 80, ACQSYS, NOT_SPECIFIED, false },
  {ECH_D_INITCMD,       EGA_EGA_D_STRINITCMD,    82, ACQSYS, NOT_SPECIFIED, false },
  {ECH_D_GCPRIMARY,     EGA_EGA_D_STRGCPRIMARY,  80, ACQSYS, NONDIFF,       true  },
  {ECH_D_GCSECOND,      EGA_EGA_D_STRGCSECOND,   80, ACQSYS, NONDIFF,       true  },
  {ECH_D_GCTERTIARY,    EGA_EGA_D_STRGCTERTIARY, 80, ACQSYS, NONDIFF,       true  },
  {ECH_D_DIFFPRIMARY,   "D_DIFFPRIMARY",         80, ACQSYS, DIFF,          true  },
  {ECH_D_DIFFSECOND,    "D_DIFFSECONDARY",       80, ACQSYS, DIFF,          true  },
  {ECH_D_DIFFTERTIARY,  "D_DIFFTRERTIARY",       80, ACQSYS, DIFF,          true  },
  {ECH_D_INFODIFFUSION, EGA_EGA_D_STRINFOSDIFF,  80, ACQSYS, NONDIFF,       true  },
  {ECH_D_DELEGATION,    EGA_EGA_D_STRDELEGATION, 80, ACQSYS, NOT_SPECIFIED, true  }
};

// ==========================================================================================================
EGSA::EGSA()
  : m_ext_smad(NULL),
    m_config(NULL)
{
}

// ==========================================================================================================
EGSA::~EGSA()
{
  detach();

  delete m_ext_smad;
  delete m_config;
}

// ==========================================================================================================
int EGSA::init()
{
  int rc = 0;

  // TODO: должен быть список подчиненных систем сбора, взять оттуда
  const char* sa_name = "BI4500";
//  const char* smad_name = "DB_LOCAL_SA.sqlite";
//  sa_object_type_t sa_type = TYPE_LOCAL_SA;

  m_ext_smad = new ExternalSMAD("EGSA_SMAD.sqlite");
  smad_connection_state_t ext_state = m_ext_smad->connect();
  LOG(INFO) << "EGSA init " << ext_state;
  rc = (STATE_OK == ext_state)? 0 : 1;

  // TEST - подключиться к SMAD для каждой подчиненной системы
  // 2016/04/13 Пока у нас одна система, для опытов
#if 1
  m_config = new EgsaConfig();

  SystemAcquisition *sa_instance = new SystemAcquisition(TYPE_LOCAL_SA, GOF_D_SAC_NATURE_EELE, sa_name);
  m_sa_list.insert(std::pair<std::string, SystemAcquisition*>(sa_name, sa_instance));

#else
  InternalSMAD* int_smad = new InternalSMAD(smad_name);
  smad_connection_state_t int_state = int_smad->attach(sa_name, sa_type);
  if (STATE_OK == int_state) {
    m_internal_smad_list.insert(std::pair<std::string, InternalSMAD*>(sa_name, int_smad))
    LOG(INFO) << "Attach to '" << sa_name << "'";
  }
  else {
    LOG(ERROR) << "Fail attach to '" << sa_name << "'";
  }
#endif

  return rc;
}

// ==========================================================================================================
// Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD 
int EGSA::detach()
{
  int rc = 0;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for (system_acquisition_list_t::iterator it = m_sa_list.begin();
       it != m_sa_list.end();
       it++)
  {
    LOG(INFO) << "TODO: set " << (*it).first << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: datach " << (*it).first << " SMAD";
  }

  return rc;
}

// ==========================================================================================================
int EGSA::run()
{
  volatile int stop;

  stop = init();

  while (!stop)
  {
    // TODO: пробежаться по всем зарегистрированным системам сбора.
    // Если они в активном состоянии, получить от них даннные
    for (system_acquisition_list_t::iterator it = m_sa_list.begin();
        it != m_sa_list.end();
        it++)
    {
      switch((*it).second->state())
      {
        case SA_STATE_OPER:
          LOG(INFO) << (*it).first << "state is OPERATIVE";

          (*it).second->pop(m_list);

          for (sa_parameters_t::iterator it = m_list.begin();
               it != m_list.end();
               it++)
          {
            switch ((*it).type)
            {
              case SA_PARAM_TYPE_TM:
                LOG(INFO) << "name:" << (*it).name << " type:" << (*it).type << " g_val:" << (*it).g_value;
                break;

              case SA_PARAM_TYPE_TS:
              case SA_PARAM_TYPE_TSA:
              case SA_PARAM_TYPE_AL:
                LOG(INFO) << "name:" << (*it).name << " type:" << (*it).type << " i_val:" << (*it).i_value;
                break;

              default:
                LOG(ERROR) << "name: " << (*it).name << " unsupported type:" << (*it).type;
            }
          }
          break;

        case SA_STATE_PRE_OPER:
          LOG(INFO) << (*it).first << "state is PRE OPERATIVE";
          break;

        case SA_STATE_UNREACH:
          LOG(INFO) << (*it).first << " state is UNREACHABLE";
          break;

        case SA_STATE_UNKNOWN:
          LOG(INFO) << (*it).first << " state is UNKNOWN";
          break;

        default:
          LOG(ERROR) << (*it).first << " state unsupported: " << (*it).second->state();
          break;
      }
    }
    sleep (1);
  }

  return stop;
}
