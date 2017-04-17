#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "exchange_egsa_init.hpp"

// ==========================================================================================================
ega_ega_odm_t_RequestEntry g_requests_table[] = {
// Идентификатор запроса
// |                    Название запроса
// |                    |                         Приоритет
// |                    |                         |   Тип объекта - информация, СС, оборудование
// |                    |                         |   |       Режим сбора - дифференциальный или нет
// |                    |                         |   |       |              Признак отношения запроса:
// |                    |                         |   |       |              к технологическим данным (1)
// |                    |                         |   |       |              к состоянию системы сбора (0)
// |                    |                         |   |       |              |      Вложенные запросы
// |                    |                         |   |       |              |      |
  {ECH_D_GENCONTROL,    EGA_EGA_D_STRGENCONTROL,  80, ACQSYS, NONDIFF,       true,  {} },
  {ECH_D_INFOSACQ,      EGA_EGA_D_STRINFOSACQ,    80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_URGINFOS,      EGA_EGA_D_STRURGINFOS,    80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_GAZPROCOP,     EGA_EGA_D_STRGAZPROCOP,   80, ACQSYS, NONDIFF,       false, {} },
  {ECH_D_EQUIPACQ,      EGA_EGA_D_STREQUIPACQ,    80, EQUIP,  NONDIFF,       true,  {} },
  {ECH_D_ACQSYSACQ,     EGA_EGA_D_STRACQSYSACQ,   80, ACQSYS, NONDIFF,       false, {} },
  {ECH_D_ALATHRES,      EGA_EGA_D_STRALATHRES,    80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_TELECMD,       EGA_EGA_D_STRTELECMD,    101, INFO,   NOT_SPECIFIED, true,  {} },
  {ECH_D_TELEREGU,      EGA_EGA_D_STRTELEREGU,   101, INFO,   NOT_SPECIFIED, true,  {} },
  {ECH_D_SERVCMD,       EGA_EGA_D_STRSERVCMD,     80, ACQSYS, NOT_SPECIFIED, false, {} },
  {ECH_D_GLOBDWLOAD,    EGA_EGA_D_STRGLOBDWLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },
  {ECH_D_PARTDWLOAD,    EGA_EGA_D_STRPARTDWLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },
  {ECH_D_GLOBUPLOAD,    EGA_EGA_D_STRGLOBUPLOAD,  80, ACQSYS, NOT_SPECIFIED, false, {} },
  {ECH_D_INITCMD,       EGA_EGA_D_STRINITCMD,     82, ACQSYS, NOT_SPECIFIED, false, {} },
  {ECH_D_GCPRIMARY,     EGA_EGA_D_STRGCPRIMARY,   80, ACQSYS, NONDIFF,       true,  {} },
  {ECH_D_GCSECOND,      EGA_EGA_D_STRGCSECOND,    80, ACQSYS, NONDIFF,       true,  {} },
  {ECH_D_GCTERTIARY,    EGA_EGA_D_STRGCTERTIARY,  80, ACQSYS, NONDIFF,       true,  {} },
  {ECH_D_DIFFPRIMARY,   EGA_EGA_D_STRDIFFPRIMARY, 80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_DIFFSECOND,    EGA_EGA_D_STRDIFFSECOND,  80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_DIFFTERTIARY,  EGA_EGA_D_STRDIFFTERTIARY,80, ACQSYS, DIFF,          true,  {} },
  {ECH_D_INFODIFFUSION, EGA_EGA_D_STRINFOSDIFF,   80, ACQSYS, NONDIFF,       true,  {} },
  {ECH_D_DELEGATION,    EGA_EGA_D_STRDELEGATION,  80, ACQSYS, NOT_SPECIFIED, true,  {} }
};

// ==========================================================================================================
int get_request_by_id(ech_t_ReqId _id, ega_ega_odm_t_RequestEntry*& _entry)
{
  int rc = NOK;

  if ((ECH_D_GENCONTROL <= _id) && (_id <= ECH_D_DELEGATION)) {
    rc = OK;
    _entry = &g_requests_table[_id];
  }
  else {
    _entry = NULL;
  }

  return rc;
}

// ==========================================================================================================
int get_request_by_name(const std::string& _name, ega_ega_odm_t_RequestEntry*& _entry)
{
  for (int id = ECH_D_GENCONTROL; id <= ECH_D_DELEGATION; id++) {
    if (id == g_requests_table[id].e_RequestId) {
      if (0 == _name.compare(g_requests_table[id].s_RequestName)) {
        _entry = &g_requests_table[id];
        return OK;
      }
    }
    else {
      // Нарушение структуры таблицы - идентификатор запроса не монотонно возрастающая последовательность
      assert(0 == 1);
    }
  }

  _entry = NULL;
  return NOK;
}

// ==========================================================================================================
