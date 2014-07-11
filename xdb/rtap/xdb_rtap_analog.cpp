#include <assert.h>
#include "glog/logging.h"

#include "config.h"

#include "xdb_core_error.hpp"
#include "xdb_rtap_common.h"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_analog.hpp"

using namespace xdb;

RtAnalog::RtAnalog(Analog_t* tm)
{
  LOG(INFO) << "RtAnalog()";
  assert(tm);
}

RtAnalog::RtAnalog(Analog_t&)
{
  LOG(INFO) << "RtAnalog()";
}

RtAnalog::~RtAnalog()
{
  LOG(INFO) << "~RtAnalog()";
}

#if 0
TM::TM (db_info* p_handler, Telemeasurement* p_data)
{
  MCO_RET     rc = 0;
  mco_trans_h t;
  TM          obj;
  Header      header;
  Timestamp   timestamp;
  TestDB_oid  oid;

  g_assert(p_handler != NULL);
  g_assert(p_data != NULL);

  mco_trans_start(p_handler->db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);

  while(1==1)
  {
    oid.id = p_data->id;

    rc = TM_new(t, &oid, &obj);
    if (rc) break;

    rc = TM_VAL_put       (&obj, p_data->val);
    if (rc) break;
    rc = TM_VALACQ_put    (&obj, p_data->valacq);
    if (rc) break;
    rc = TM_VALMANUAL_put (&obj, p_data->valmanual);
    if (rc) break;
    rc = TM_VALID_put     (&obj, p_data->valid);
    if (rc) break;
    rc = TM_VALIDACQ_put  (&obj, p_data->validacq);
    if (rc) break;

    rc = TM_HEADER_read_handle    (&obj, &header);      /* get Header object */
    if (rc) break;
    rc = Header_OBJCLASS_put      (&header, p_data->objclass);
    if (rc) break;
    rc = Header_UNIVNAME_put      (&header, p_data->univname,
                                    (uint2)MIN(strlen(p_data->univname), UNIVNAME_LENGTH));
    if (rc) break;
    rc = Header_CODE_put          (&header, p_data->code,
                                    (uint2)MIN(strlen(p_data->code), CODE_LENGTH));
    if (rc) break;
    rc = Header_LABEL_put         (&header, p_data->label,
                                    (uint2)MIN(strlen(p_data->label), LABEL_LENGTH));
    if (rc) break;
    rc = Header_SHORTLABEL_put    (&header, p_data->shortlabel,
                                    (uint2)MIN(strlen(p_data->shortlabel), SHORTLABEL_LENGTH));
    if (rc) break;
    rc = Header_LINK_SA_put       (&header, p_data->id_SA);
    if (rc) break;
    rc = TM_HEADER_write_handle   (&obj, &header);     /* putback Header object */
    if (rc) break;

    rc = TM_MNVALPHY_put(&obj, p_data->mnvalphy);
    if (rc) break;
    rc = TM_MXVALPHY_put(&obj, p_data->mxvalphy);
    if (rc) break;

    rc = TM_TIMESTAMP_read_handle(&obj, &timestamp); /* get Timestamp object */
    if (rc) break;
    rc = Timestamp_HAPPENING_DT_put(&timestamp, p_data->date_mark);
    if (rc) break;
    rc = Timestamp_HAPPENING_TM_put(&timestamp, p_data->time_mark);
    if (rc) break;
    rc = TM_TIMESTAMP_write_handle(&obj, &timestamp);/* putback Timestamp object */
    if (rc) break;


    rc = TM_UNITY_DISPLAY_REF_put(&obj, p_data->id_unity_display);
    if (rc) break;
    rc = TM_UNITY_ORIGIN_REF_put(&obj, p_data->id_unity_origin);
    if (rc) break;


    /* Previous processing was clear */
    //g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Creating TM \"%s\" oid=%d objclass=%d",
//    g_printf("Creating TM \"%s\" oid=%d objclass=%d\n", p_data->univname, oid.id, p_data->objclass);
    Telemeasurement_print(p_data);

    rc = mco_trans_commit(t);
    return rc; /* normally exit from cycle */
  }

  if (rc) 
    mco_trans_rollback(t);

  return rc;
}

MCO_RET  Telemeasurement_by_description_new (db_info* p_handler, PointDescription_t* p_description)
{
  MCO_RET     rc = 0;
  Telemeasurement data;
  AttributeInfo_t* p_attr_info = NULL;

  g_assert(p_handler != NULL);
  g_assert(p_description != NULL);

  memset((void*)&data, '\0', sizeof(Telemeasurement)); /* TODO remove this! +++++ для отладки ++++ */

  data.id       = GetNewDatabaseOID();
  data.objclass = p_description->objclass;
  strcpy(data.code, p_description->code);

  /*
    TODO
    Пройтись по всем атрибутам в списке attr_info_list и присвоить значения 
    в соответствующие поля
   */
  if ((p_attr_info = GetAttrByName(p_description, "UNIVNAME")) != NULL)
    strcpy(data.univname, p_attr_info->value.val_bytes);
  if ((p_attr_info = GetAttrByName(p_description, "LABEL")) != NULL)
    strcpy(data.label, p_attr_info->value.val_bytes);
  if ((p_attr_info = GetAttrByName(p_description, "SHORTLABEL")) != NULL)
    strcpy(data.shortlabel, p_attr_info->value.val_bytes);
  if ((p_attr_info = GetAttrByName(p_description, "CODE")) != NULL)
    strcpy(data.code, p_attr_info->value.val_bytes);
  if ((p_attr_info = GetAttrByName(p_description, "VAL")) != NULL)
    data.val = p_attr_info->value.val_float;
  if ((p_attr_info = GetAttrByName(p_description, "VALID")) != NULL)
    data.valid = p_attr_info->value.val_int8;
  if ((p_attr_info = GetAttrByName(p_description, "MNVALPHY")) != NULL)
    data.mnvalphy = p_attr_info->value.val_float;
  if ((p_attr_info = GetAttrByName(p_description, "MXVALPHY")) != NULL)
    data.mxvalphy = p_attr_info->value.val_float;

  if ((p_attr_info = GetAttrByName(p_description, "L_SA")) != NULL)
  {
    /* если атрибут найден и имя системы сбора задано (значение - ненулевая строка) */
    if (p_attr_info->value.val_bytes && p_attr_info->value.val_bytes[0]!='\0')
    {
      /* получить идентификатор СС по ее имени */
      if (!GetSaIdByDescr(p_handler, p_attr_info, /* OUT */ &data.id_SA))
      {
        /* имя системы сбора указано, но не найдено в справочнике */
        g_printf("СС '%s' для параметра '%s' не найдена в перечне используемых\n",
                 p_attr_info->value.val_bytes, data.univname);
      }
    }
    else data.id_SA = 0; /* имя системы сбора не указано */
  }

  if ((p_attr_info = GetAttrByName(p_description, "UNITY")) != NULL)
  {
    /* если атрибут найден и описание физической размерности задано (значение - ненулевая строка) */
    if (p_attr_info->value.val_bytes && p_attr_info->value.val_bytes[0]!='\0')
    {
      /* получить идентификатор размерности по имени */
      if (GetUnityIdByDescr(p_handler, p_attr_info, &data.id_unity_origin))
      {
        /* по умолчанию физическая размерность и размерность отображения совпадают */
        data.id_unity_display = data.id_unity_origin;
      }
      else
      {
        g_printf("Размерность '%s' для параметра '%s' не найдена в словаре\n",
                 p_attr_info->value.val_bytes, data.univname);
        data.id_unity_origin = data.id_unity_display = 0; /* размерность указана, но не найдена в словаре */
      }
    }
    else 
    {
        data.id_unity_origin = data.id_unity_display = 0; /* размерность не указана */
    }
  }

  /* 
    Получить имя родительской точки - через UNIVNAME данной точки
    потому что в файле instances_total.dat используется алиас родителя, а не univname
   */
  GetParentPointName2(data.univname, 
                      data.objclass,
                      p_description->parent_alias,
                      data.parent);

  rc = Telemeasurement_new(p_handler, &data);

  return rc;
}


MCO_RET  Telemeasurement_delete_by_id (db_info* p_handler, Telemeasurement* p_data)
{
  MCO_RET rc = 0;
  return rc;
}

MCO_RET  Telemeasurement_find_by_id (db_info* p_handler, Telemeasurement* p_data)
{
  MCO_RET rc = 0;
  return rc;
}

MCO_RET  Telemeasurement_update_by_id (db_info* p_handler, Telemeasurement* p_data)
{
  MCO_RET rc = 0;
  return rc;
}


MCO_RET  Telemeasurement_delete_all (db_info* p_handler)
{
  MCO_RET rc = 0;
  return rc;
}

MCO_RET  Telemeasurement_list_all (db_info* p_handler)
{
  MCO_RET rc = 0;
  return rc;
}

int Telemeasurement_print(Telemeasurement* p_data)
{
  g_printf("id:%d objclass:%d %s \"%s\" \"%s\" parent:%s id_sa:%d val:%.4f valacq:%.4f "
        "valmanual:%.4f valid:%d validacq:%d [%.4f:%.4f] id_unity:%d id_unity_orig:%d\n",
        p_data->id,
        p_data->objclass,
        p_data->univname,
        p_data->shortlabel, 
        p_data->label,
        p_data->parent,
        p_data->id_SA,
        p_data->val,
        p_data->valacq,
        p_data->valmanual,
        (unsigned int)p_data->valid,
        (unsigned int)p_data->validacq,
        (float)p_data->mnvalphy,
        (float)p_data->mxvalphy,
        (unsigned int)p_data->id_unity_display,
        (unsigned int)p_data->id_unity_origin);

  return 0;
}



/* p_line looks like "(0|1000|KUMW)" */
int ParseDimensionString(db_info* p_handler, const char* p_line, Telemeasurement* p_data)
{
  Unity         unity;
  int           result  = 0;
  GError        *err    = NULL;
  GMatchInfo    *match_info;
  gchar         *low    = NULL;
  gchar         *high   = NULL;
  gchar         *code   = NULL;
  gchar         *line_utf8 = NULL;
  gint          bytes_read;
  gint          bytes_written;
  GRegex        *simple_data_regex;

  /* формат выражения отличается для различных кодировок */
  switch (p_handler->encoding) {
    case DB_ENCODING_UTF8:
        /* NB применять только если код размерности импользует не латиницу */
        simple_data_regex = g_regex_new("([0-9.]+)\\|([0-9.]+)\\|(\\S+)",
                                G_REGEX_OPTIMIZE | G_REGEX_DOTALL, 0, &err);
    break;

    case DB_ENCODING_ISO8859_5:
        simple_data_regex = g_regex_new("([-+0-9.]+)\\|([-+0-9.]+)\\|(KU[-/%A-Za-z0-9]+)",
                                G_REGEX_OPTIMIZE | G_REGEX_DOTALL, 0, &err);
  }

  if (err)
    g_error("%s\n", err->message);

  p_data->mnvalphy = 0;
  p_data->mxvalphy = 0;
  p_data->id_unity_origin = 0;

  /* TODO convert all literal input data into UTF-8 */
  /* '-1' using for NULL-terminated strings! */
  line_utf8 = g_convert(p_line, -1, "utf-8", "iso8859-5", &bytes_read, &bytes_written, &err);
  if (!line_utf8)
  {
    g_error("Can't change string conversion to utf-8: %s\n", err->message);
  }
  else
  {
//    g_debug("\"%s\" read:%d written:%d", line_utf8, bytes_read, bytes_written);

    if (!g_regex_match(simple_data_regex, line_utf8, 0, &match_info))
    {
      g_error("unable to match dimension data from \"%s\"\n", line_utf8);
    }
    else
    {
      low = g_strchomp(g_match_info_fetch(match_info, 1));
      high = g_strchomp(g_match_info_fetch(match_info, 2));
      code = g_strchomp(g_match_info_fetch(match_info, 3));
      p_data->mnvalphy = (float) g_ascii_strtod(low, NULL);
      p_data->mxvalphy = (float) g_ascii_strtod(high, NULL);

      /* TODO this is possible only if "code" will always be latin! +++++ GEV */
      strcpy(unity.code, code);
      result = Unity_get_by_code(p_handler, &unity);
      if (!result)
      {
        g_debug("UNITY not found for univname=%s low=%.4f high=%.4f code=\"%s\"",
            p_data->univname,
            p_data->mnvalphy,
            p_data->mxvalphy,
            unity.code);
      }
      else
      {
        p_data->id_unity_origin = unity.id;
        result = 1;
      }

      g_free(low);
      g_free(high);
      g_free(code);
    }

    g_free(line_utf8);
  }

  return result;
}

#endif
