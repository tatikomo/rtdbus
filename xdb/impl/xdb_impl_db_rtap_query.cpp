/* =====================================================================
 * ITG
 * author: Eugeniy Gorovoy
 * email: eugeni.gorovoi@gmail.com
 * Группа функций запроса информации по образцу.
 *  rtQUERY_PTS_IN_CATEG                : queryPointsOfSpecifiedCategory
 *  rtQUERY_PTS_IN_CLASS                : queryPointsOfSpecifiedClass
 *  rtQUERY_SBS_LIST_ARMED              : querySbsArmedGroup
 *  rtQUERY_SBS_POINTS_ARMED            : querySbsPoints
 *  rtQUERY_SBS_POINTS_DISARM           : querySbsPoints
 *  rtQUERY_SBS_READ_POINTS_ARMED       : querySbsPoints
 *  rtQUERY_SBS_POINTS_DISARM_BY_LIST   : querySbsDisarmSelectedPoints
 * ======================================================================
 */

#include <unordered_set>

#include <assert.h>
#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "helper.hpp"

#include "xdb_common.hpp"
#include "xdb_impl_common.h"
#include "xdb_impl_error.hpp"

#include "xdb_impl_db_rtap.hpp"

using namespace xdb;

// ===========================================================================
ProcessingType_t xdb::get_type_by_objclass(const objclass_t objclass)
{
  const char* fname = "get_type_by_objclass";
  ProcessingType_t result = PROCESSING_UNKNOWN;

  switch(xdb::ClassDescriptionTable[objclass].val_type) {
    case xdb::DB_TYPE_LOGICAL:
      result = PROCESSING_BINARY;
      break;

    case xdb::DB_TYPE_INT16:
    case xdb::DB_TYPE_UINT16:
    case xdb::DB_TYPE_INT32:
    case xdb::DB_TYPE_UINT32:
      result = PROCESSING_DISCRETE;
      break;

    case xdb::DB_TYPE_DOUBLE:
    case xdb::DB_TYPE_FLOAT:
      result = PROCESSING_ANALOG;
      break;

    case xdb::DB_TYPE_UNDEF:
      // Молча игнорировать
      break;

    default:
      result = PROCESSING_UNKNOWN;
      LOG(ERROR) << fname << ": unsupported objclass " << objclass;
  }

  return result;
}

// =================================================================================
// Группа функций запроса информации
const Error& DatabaseRtapImpl::Query(mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc;

  setError(rtE_RUNTIME_ERROR);

  switch(info.action.query)
  {
    // Прочитать список точек, удовлетворяющих заданным критериям
    case rtQUERY_PTS_IN_CATEG:
        rc = queryPointsOfSpecifiedCategory(handler, info);
        if (rc) { LOG(ERROR) << "Get points list of category: " << info.addrCnt; }
        else clearError();
        break;

    // Прочитать список точек указанного класса
    case rtQUERY_PTS_IN_CLASS:
        rc = queryPointsOfSpecifiedClass(handler, info);
        if (rc) { LOG(ERROR) << "Get points list of class: " << info.addrCnt; }
        else clearError();
        break;

    // Вернуть список групп подписки, содержащие точки с модифицированными атрибутами
    case rtQUERY_SBS_LIST_ARMED:
        rc = querySbsArmedGroup(handler, info);
        if (rc) { LOG(ERROR) << "Get active subscription groups list"; }
        else clearError();
        break;

    // Вернуть список точек с модифицированными атрибутами, для указанной группы
    case rtQUERY_SBS_POINTS_ARMED:
        rc = querySbsPoints(handler, info, info.action.query);
        if (rc) { LOG(ERROR) << "Get activated points for selected subscription group"; }
        else clearError();
        break;

    // Для указанной группы Точек сбросить признак модификации
    case rtQUERY_SBS_POINTS_DISARM:
        rc = querySbsPoints(handler, info, info.action.query);
        if (rc) { LOG(ERROR) << "Deactivating points for selected subscription group"; }
        else clearError();
        break;

    // Для указанной группы прочитать значения модифицированных атрибутов
    case rtQUERY_SBS_READ_POINTS_ARMED:
        rc = querySbsPoints(handler, info, info.action.query);
        if (rc) { LOG(ERROR) << "Reading modified points for selected subscription group"; }
        else clearError();
        break;

    // Сбросить флаг модификации для измененных точек из данного списка unordered_map
    case rtQUERY_SBS_POINTS_DISARM_BY_LIST:
        rc = querySbsDisarmSelectedPoints(handler, info);
        if (rc) { LOG(ERROR) << "Deactivating selected modified points for all subscription groups"; }
        else clearError();
        break;

    default:
        setError(rtE_NOT_IMPLEMENTED);
  }

  return getLastError();
}

// =================================================================================
// Прочитать список точек указанной категории
// Входные данные:
// 1) buffer - ссылка на map_id_name_t, содержащего карту autoid_t и имени тега
// 2) addrCnt - битовый массив искомых категорий точек
MCO_RET DatabaseRtapImpl::queryPointsOfSpecifiedCategory (mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  // NB: возможно несоответствие размерности, category_type_t д.б. эквивалентен info.addrCnt
  category_type_t category_type = static_cast<category_type_t>(info.addrCnt);

  // NB: пока не обыгрывается возможность комбинации битовых флагов поля category_type
  switch(category_type)
  {
    case CATEGORY_HAS_HISTORY:
      rc = queryPointsWithHistory(handler, info);
      break;

    default:
      LOG(ERROR) << "Query unsupported Points category type: " << category_type;
  }

  return rc;
}

// =================================================================================
// Прочитать список точек указанного класса
// Входные данные:
// 1) buffer - ссылка на map_id_name_t, с искомого OBJCLASS Точек (для запроса одержащего карту autoid_t и имени тега
// 2) addrCnt - objclass искомых точек
MCO_RET DatabaseRtapImpl::queryPointsOfSpecifiedClass (mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  mco_cursor_t csr;
  mco_trans_h t;
  autoid_t point_aid = 0;
  // Текущий экземпляр Точки
  XDBPoint point_instance;
  // Значение OBJCLASS текущего экземпляра
  objclass_t objclass_instance;
  // Результат сравнения с атрибутом objclass_needed
  int compare_result = 0;
  // Входной параметр - адрес хеша с выходными данными
  map_id_name_t *points_map = static_cast<map_id_name_t*>(info.buffer);
//  map_id_name_t::iterator points_map_iterator;
  // Ай-яй-яй! Стыдно должно быть передавать тип объекта через значение несоответствующего аргумента!
  objclass_t objclass_needed = static_cast<objclass_t>(info.addrCnt);
  // Буфер для чтения значения тега Точки
  const uint2 tag_size = sizeof(wchar_t)*TAG_NAME_MAXLEN;
  char tag[tag_size + 1];

  assert(info.buffer);
  points_map->clear();

  do
  {
    rc = mco_trans_start(handler, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = XDBPoint_SK_by_objclass_index_cursor(t, &csr);
    if (MCO_S_OK == rc)
    {
      rc = XDBPoint_SK_by_objclass_search(t, &csr, MCO_EQ, objclass_needed);
      if (rc && (rc != MCO_S_NOTFOUND)) { LOG(ERROR) << "Search in point's tree by OBJCLASS cursor, rc=" << rc; break; }

      if (rc != MCO_S_NOTFOUND)
      {
        // Прочитать очередной экземпляр XDBPoint
        rc = mco_cursor_first(t, &csr);
        if (rc) { LOG(ERROR) << "Get first XDBPoint item, rc=" << rc; break; }

        // Цикл по всем Точкам
        while (MCO_S_OK == rc)
        {
          compare_result = 0;
          rc = XDBPoint_SK_by_objclass_compare(t, &csr, objclass_needed, &compare_result);
          if (rc) { LOG(ERROR) << "Compare OBJCLASS in tree index cursor, rc=" << rc; break; }

          if (0 == compare_result) // Найден объект соответствующего класса
          {
            rc = XDBPoint_from_cursor(t, &csr, &point_instance);
            if (rc) { LOG(ERROR) << "Get instance from tree index cursor, rc=" << rc; break; }

            rc = XDBPoint_OBJCLASS_get(&point_instance, &objclass_instance);
            if (rc) { LOG(ERROR) << "Get point's id=" <<point_aid<<" OBJCLASS, rc=" << rc; break; }

            rc = XDBPoint_autoid_get(&point_instance, &point_aid);
            if (rc) { LOG(ERROR) << "Get point's id, rc=" << rc; break; }

            rc = XDBPoint_TAG_get(&point_instance, tag, tag_size);
            if (rc) { LOG(ERROR) << "Get point's id="<<point_aid<<" TAG, rc=" << rc; break; }

            // Занесем реквизиты текущей точки в список
            points_map->insert(std::pair<autoid_t, std::string>(point_aid, tag));
//1            LOG(INFO) << "queryPointsOfSpecifiedClass insert " << objclass_needed << ":" << point_aid << ":" << tag;
          }

          rc = mco_cursor_next(t, &csr);
          if (rc && (rc != MCO_S_CURSOR_END)) { LOG(ERROR) << "Next XDBPoint cursor, rc=" << rc; break; }

        } // Конец проверки, что первый элемент курсора установлен

      } // Конец проверки, есть ли точки такого класса?

#if (EXTREMEDB_VERSION >=40)
      rc = mco_cursor_close(t, &csr);
      if (rc) { LOG(ERROR) << "Close XDBPoint cursor, rc=" << rc; break; }
#endif

    } // Конец успешного создания курсора по индексу модификации

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while (false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
// Интерфейс получения списка групп с активированными (измененными) атрибутами точек
// Каждая точка может находиться в нескольких группах. Необходимо для каждой точки
// определить этот набор групп, и каждая последующая точка должна его расширять.
// Группа должна находиться в состоянии ENABLE
//
// Для этого можно использовать std::map(идентификатор группы, название группы)
MCO_RET DatabaseRtapImpl::querySbsArmedGroup (mco_db_h& handler, rtDbCq& info)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  mco_cursor_t csr_point;
  mco_cursor_t csr_sbs;
  mco_trans_h t;
  autoid_t point_aid = 0;
  autoid_t sbs_aid = 0;
  XDBPoint point_instance;
  SBS_GROUPS_ITEM sbs_item_instance;
  SBS_GROUPS_STAT sbs_stat_instance;
  // Состояние Группы Подписки
  InternalState state;
  Boolean mod = TRUE;
  // Результат сравнения с атрибутом is_modified
  int compare_modified_result = 0;
  // Результат сравнения с атрибутом tag_id
  int compare_id_result = 0;
  char sbs_name[LABEL_MAXLEN + 1];
  uint2 sbs_name_size = LABEL_MAXLEN;

  map_id_name_t *sbs_map = static_cast<map_id_name_t*>(info.buffer);
//  const map_id_name_t::iterator sbs_map_iterator;

  assert(info.buffer);

  sbs_map->clear();

  do
  {
    rc = mco_trans_start(handler, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Найти все изменившиеся точки
    rc = XDBPoint_SK_by_modified_index_cursor(t, &csr_point);
    if (MCO_S_OK == rc)
    {
      // У изменившихся взведен флаг is_modified
      rc = XDBPoint_SK_by_modified_search(t, &csr_point, MCO_EQ, TRUE);
      if (rc && (rc != MCO_S_NOTFOUND)) { LOG(ERROR) << "Find modified XDBPoint since last sbs poll, rc="<<rc; break; }

      if (rc != MCO_S_NOTFOUND)
      {
         // Прочитать очередной экземпляр XDBPoint
         rc = mco_cursor_first(t, &csr_point);
         if (rc) { LOG(ERROR) << "Get first XDBPoint item, rc=" << rc; break; }

         // Цикл по всем Точкам
         while (MCO_S_OK == rc)
         {
           compare_modified_result = 0;
           rc = XDBPoint_SK_by_modified_compare(t, &csr_point, mod, &compare_modified_result);
           if (rc) { LOG(ERROR) << "Compare XDBPoint modif status, rc=" << rc; break; }

           if (0 == compare_modified_result) // Найден элемент, принадлежащий нужной группе
           {
             rc = XDBPoint_from_cursor(t, &csr_point, &point_instance);
             if (rc) { LOG(ERROR) << "Get XDBPoint modif status, rc=" << rc; break; }

             // point_instance - модифицированная Точка
             // Найти группы, куда она входит

             // point_aid - идентификатор модифицированной Точки
             rc = XDBPoint_autoid_get(&point_instance, &point_aid);
             if (rc) { LOG(ERROR) << "Get XDBPoint aid, rc=" << rc; break; }

             // Открыть курсор в таблице Групп Подписки
             rc = SBS_GROUPS_ITEM_SK_by_tag_id_index_cursor(t, &csr_sbs);
             if (rc) { LOG(ERROR) << "Get XDBPoint aid, rc=" << rc; break; }

             // Найти все группы подписки, в которые входит Точка с идентификатором point_aid
             rc = SBS_GROUPS_ITEM_SK_by_tag_id_search(t, &csr_sbs, MCO_EQ, point_aid);
             if (rc != MCO_S_NOTFOUND)
             {
               compare_id_result = 0;

               // Проверить очередной экземпляр SBS_GROUPS_ITEM
               rc = mco_cursor_first(t, &csr_sbs);
               if (rc) { LOG(ERROR) << "Get first SBS_GROUPS_ITEM, rc="<<rc; break; };

               while (MCO_S_OK == rc)
               {
                 rc = SBS_GROUPS_ITEM_SK_by_tag_id_compare(t, &csr_sbs, point_aid, &compare_id_result);
                 if (rc) { LOG(ERROR) << "Compare SBS_GROUPS_ITEM with point id="<<point_aid<<", rc=" << rc; break; }

                 if (0 == compare_id_result) // Найден элемент, принадлежащий нужной группе
                 {
                   rc = SBS_GROUPS_ITEM_from_cursor(t, &csr_sbs, &sbs_item_instance);
                   if (rc) { LOG(ERROR) << "Get current item from SBS_GROUPS_ITEM cursor, rc=" << rc; break; }

                   rc = SBS_GROUPS_ITEM_group_id_get(&sbs_item_instance, &sbs_aid);
                   if (rc) { LOG(ERROR) << "Get point id from SBS_GROUPS_ITEM, rc=" << rc; break; }

                   // Если идентификатор данной группы не известен, запоминаем ее
                   const map_id_name_t::const_iterator sbs_map_iterator = sbs_map->find(sbs_aid);
                   if (sbs_map_iterator == sbs_map->end())
                   {
                     // Получить название группы
                     rc = SBS_GROUPS_STAT_autoid_find(t, sbs_aid, &sbs_stat_instance);
                     if (rc) { LOG(ERROR) << "Locate SBS from SBS_GROUPS_STAT with id="<<sbs_aid<<", rc=" << rc; break; }

                     // Проверить состояние группы, нужны только ENABLE
                     rc = SBS_GROUPS_STAT_state_get(&sbs_stat_instance, &state);
                     if (rc) { LOG(ERROR) << "Get SBS state from SBS_GROUPS_STAT with id="<<sbs_aid<<", rc=" << rc; break; }

                     // Подходят только Группы в состоянии ENABLE
                     if (ENABLE == state)
                     {
                       // Обнулить название группы, поскольку в буфере может остаться мусор,
                       // а name_get() не пишет завершающий '\0'
                       memset(sbs_name, '\0', LABEL_MAXLEN);
                       rc = SBS_GROUPS_STAT_name_get(&sbs_stat_instance, sbs_name, sbs_name_size);
                       if (rc) { LOG(ERROR) << "Get SBS name from SBS_GROUPS_STAT with id="<<sbs_aid<<", rc=" << rc; break; }

                       LOG(INFO) << "Memory new SBS '" << sbs_name << "'";
                       sbs_map->insert(std::pair<autoid_t, std::string>(sbs_aid, sbs_name));
                     }
#if defined VERBOSE
                     else // группа заблокирована
                     {
                       LOG(INFO) << "Skip suspended SBS '"<< sbs_map_iterator->second <<"'";
                     }
#endif
                   }
#if defined VERBOSE
                   else // группа уже отметилась
                   {
                     LOG(INFO) << "Skip already known SBS '"<< sbs_map_iterator->second <<"'";
                   }
#endif
                 }
                 rc = mco_cursor_next(t, &csr_sbs);
                 if (rc && (rc != MCO_S_CURSOR_END)) { LOG(ERROR) << "Next SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
               }

             } // Конец поиска групп подписки, содержащих искомую Точку

#if (EXTREMEDB_VERSION >=40)
             rc = mco_cursor_close(t, &csr_sbs);
             if (rc) { LOG(ERROR) << "Close SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
#endif

           } // Конец проверки того, что искомая Точка модифицирована

           rc = mco_cursor_next(t, &csr_point);
           if (rc && (rc != MCO_S_CURSOR_END)) { LOG(ERROR) << "Next XDBPoint cursor, rc=" << rc; break; }

         } // Конец цикла проверки всех Точек на модификации

      } // Конец проверки, есть ли модифицированные точки?

#if (EXTREMEDB_VERSION >=40)
      rc = mco_cursor_close(t, &csr_point);
      if (rc) { LOG(ERROR) << "Close XDBPoint cursor, rc=" << rc; break; }
#endif

    } // Конец успешного создания курсора по индексу модификации

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while (false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
// Пройти по списку Точек, и сбросить им флаг модификации
// Входные параметры:
//   info.buffer - указатель на std::unordered_set<std::string>
MCO_RET DatabaseRtapImpl::querySbsDisarmSelectedPoints(mco_db_h& handler, rtDbCq& info)
{
  mco_trans_h t;
  XDBPoint instance;
  MCO_RET rc = MCO_E_UNSUPPORTED;

  assert(info.buffer);
  std::unordered_set<std::string> *selected_points =
            static_cast< std::unordered_set<std::string>* >(info.buffer);

  do
  {
    LOG(INFO) << "querySbsDisarmSelectedPoints #points:" << selected_points->size();

    rc = mco_trans_start(handler, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    for (std::unordered_set<std::string>::const_iterator it = selected_points->begin();
         it != selected_points->end();
         ++it)
    {
      rc = XDBPoint_SK_by_tag_find(t,
                                   (*it).c_str(),
                                   (*it).size(), //xdb::AttrTypeDescription[RTDB_ATT_IDX_UNIVNAME].size,
                                   &instance);
      LOG(INFO) << "querySbsDisarmSelectedPoints find : " << (*it).c_str()
                << " size: " << xdb::AttrTypeDescription[RTDB_ATT_IDX_UNIVNAME].size
                << " rc=" << rc;
      if (rc)
      {
        LOG(ERROR) << "Finding point by tag '" << (*it) << "', rc=" << rc;
        break;
      }

      rc = XDBPoint_is_modified_put(&instance, FALSE);
      if (rc)
      {
        LOG(ERROR) << "Clear modified flag for tag '" << (*it) << "', rc=" << rc;
        break;
      }
    }

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return rc;
}

// =================================================================================
// Получить перечень активированных точек для указанной группы
// Входные параметры:
//  handler - идентификатор БДРВ
//  info - структура запроса
//   info.tags - вектор строк (std::vector для совместимости с другими вызовами Query),
//               содержит один элемент с названием проверяемой группы.
//
// Выходные параметры для соответствующих запросов:
//  rtQUERY_SBS_POINTS_ARMED:  info.buffer - карта соответствия (std::map) "идентификатор точки" <-> "тег точки"
//  rtQUERY_SBS_POINTS_DISARM: info.buffer - карта соответствия (std::map) "идентификатор точки" <-> "тег точки"
//  rtQUERY_SBS_READ_POINTS_ARMED: info.buffer - std::vector<xdb::AttributeInfo_t*>
MCO_RET DatabaseRtapImpl::querySbsPoints(mco_db_h& handler, rtDbCq& info, TypeOfQuery action)
{
  MCO_RET rc = MCO_E_UNSUPPORTED;
  mco_cursor_t csr_point;
  mco_cursor_t csr_sbs;
  mco_trans_h t;
  autoid_t point_aid = 0;
  autoid_t sbs_aid_looked = 0;
  autoid_t sbs_aid_current = 0;
  XDBPoint point_instance;
  SBS_GROUPS_ITEM sbs_item_instance;
  SBS_GROUPS_STAT sbs_stat_instance;
  Boolean mod = TRUE;
  // Результат сравнения с атрибутом is_modified
  int compare_modified_result = 0;
  // Результат сравнения с атрибутом tag_id
  int compare_id_result = 0;
  char point_name[TAG_NAME_MAXLEN + 1];
  uint2 point_name_size = TAG_NAME_MAXLEN;
  const map_id_name_t::iterator sbs_map_iterator;
  map_id_name_t *sbs_map = NULL;
  MCO_TRANS_TYPE trans_type = MCO_READ_ONLY;
  SubscriptionPoints_t* points_list = NULL; // Используется для rtQUERY_SBS_READ_POINTS_ARMED
  xdb::PointDescription_t* point_info = NULL; // Используется для rtQUERY_SBS_READ_POINTS_ARMED

  // Этот запрос использует параметр buffer для передачи значений
  switch(action)
  {
    case rtQUERY_SBS_POINTS_ARMED:
      assert(info.buffer);
      sbs_map = static_cast<map_id_name_t*>(info.buffer);
      sbs_map->clear();
      trans_type = MCO_READ_ONLY;
      break;
    case rtQUERY_SBS_READ_POINTS_ARMED:
      assert(info.buffer);
      points_list = static_cast<SubscriptionPoints_t*>(info.buffer);
      trans_type = MCO_READ_ONLY;
      break;
    case rtQUERY_SBS_POINTS_DISARM:
      trans_type = MCO_READ_WRITE;
      break;
    default: ; // Остальные запросы не используют rtDbCq.info.buffer
  }

  assert(info.tags);
  assert(info.tags->size() == 1);

  LOG(INFO) << "querySbsPoints with action: " << action;

  do
  {
    rc = mco_trans_start(handler, trans_type, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    // Найти информацию по требуемой группе
    rc = SBS_GROUPS_STAT_PK_by_name_find(t, info.tags->at(0).c_str(), info.tags->at(0).size(), &sbs_stat_instance);
    if (rc) { LOG(ERROR) << "Locating SBS '"<< info.tags->at(0) <<"', rc=" << rc; break; }

    rc = SBS_GROUPS_STAT_autoid_get(&sbs_stat_instance, &sbs_aid_looked);
    if (rc) { LOG(ERROR) << "Get SBS '"<< info.tags->at(0) <<"' id, rc=" << rc; break; }

    // Найти все изменившиеся точки и отметить только те, что входят в искомую группу
    rc = XDBPoint_SK_by_modified_index_cursor(t, &csr_point);

    if (MCO_S_OK == rc)
    {
      // У изменившихся взведен флаг is_modified
      rc = XDBPoint_SK_by_modified_search(t, &csr_point, MCO_EQ, TRUE);
      if (rc && (rc != MCO_S_NOTFOUND)) { LOG(ERROR) << "Find modified XDBPoint since last sbs poll, rc="<<rc; break; }

      if (rc != MCO_S_NOTFOUND)
      {
         // Прочитать очередной экземпляр XDBPoint
         rc = mco_cursor_first(t, &csr_point);
         if (rc) { LOG(ERROR) << "Get first XDBPoint item, rc=" << rc; break; }

         // Цикл по всем Точкам
         while (MCO_S_OK == rc)
         {
           compare_modified_result = 0;
           rc = XDBPoint_SK_by_modified_compare(t, &csr_point, mod, &compare_modified_result);
           if (rc) { LOG(ERROR) << "Compare XDBPoint modif status, rc=" << rc; break; }

           if (0 == compare_modified_result) // Найден изменившийся элемент, принадлежащий пока неизвестной группе
           {
             rc = XDBPoint_from_cursor(t, &csr_point, &point_instance);
             if (rc) { LOG(ERROR) << "Get XDBPoint modif status, rc=" << rc; break; }

             // point_instance - модифицированная Точка
             // Найти группы, куда она входит

             // point_aid - идентификатор модифицированной Точки
             rc = XDBPoint_autoid_get(&point_instance, &point_aid);
             if (rc) { LOG(ERROR) << "Get XDBPoint aid, rc=" << rc; break; }

             // Открыть курсор в таблице Групп Подписки
             rc = SBS_GROUPS_ITEM_SK_by_tag_id_index_cursor(t, &csr_sbs);
             if (rc) { LOG(ERROR) << "Get XDBPoint aid, rc=" << rc; break; }

             // Найти все группы подписки, в которые входит Точка с идентификатором point_aid
             rc = SBS_GROUPS_ITEM_SK_by_tag_id_search(t, &csr_sbs, MCO_EQ, point_aid);
             if (rc != MCO_S_NOTFOUND)
             {
               compare_id_result = 0;

               // Проверить очередной экземпляр SBS_GROUPS_ITEM
               rc = mco_cursor_first(t, &csr_sbs);
               if (rc) { LOG(ERROR) << "Get first SBS_GROUPS_ITEM, rc="<<rc; break; };

               while (MCO_S_OK == rc)
               {
                 rc = SBS_GROUPS_ITEM_SK_by_tag_id_compare(t, &csr_sbs, point_aid, &compare_id_result);
                 if (rc) { LOG(ERROR) << "Compare SBS_GROUPS_ITEM with point id="<<point_aid<<", rc=" << rc; break; }

                 if (0 == compare_id_result) // Найдена запись о принадлежности текущего элемента некоторой группе
                 {
                   // Проверить, та ли это группа?
                   // Идентификатор должен совпасть с sbs_aid_looked

                   rc = SBS_GROUPS_ITEM_from_cursor(t, &csr_sbs, &sbs_item_instance);
                   if (rc) { LOG(ERROR) << "Get current item from SBS_GROUPS_ITEM cursor, rc=" << rc; break; }

                   rc = SBS_GROUPS_ITEM_group_id_get(&sbs_item_instance, &sbs_aid_current);
                   if (rc) { LOG(ERROR) << "Get point id from SBS_GROUPS_ITEM, rc=" << rc; break; }

                   // Если идентификатор текущей группы совпадает с нужным, то это интересующая нас точка
                   if (sbs_aid_current == sbs_aid_looked)
                   {
                     // Определим, что нужно делать?
                     switch(action)
                     {
                       // Получить карту точек с идентификаторами и тегами
                       case rtQUERY_SBS_POINTS_ARMED:
                       // -----------------------------------------------
                         // Получить название Точки в point_name
                         // В буфере мог остаться мусор, а name_get() не пишет завершающий '\0'
                         memset(point_name, '\0', TAG_NAME_MAXLEN);

                         rc = XDBPoint_TAG_get(&point_instance, point_name, point_name_size);
                         if (rc) { LOG(ERROR) << "Get TAG for point id="<<point_aid<<", rc=" << rc; break; }

                         LOG(INFO) << "Memory new pair {id:tag} "<< point_aid << ":'" << point_name << "'";
                         sbs_map->insert(std::pair<autoid_t, std::string>(point_aid, point_name));
                         break;

                       // Сбросить флаг модификации для данной точки
                       case rtQUERY_SBS_POINTS_DISARM:
                       // -----------------------------------------------
                         rc = XDBPoint_is_modified_put(&point_instance, FALSE);
                         if (rc) { LOG(ERROR) << "Clear modified flag for point id="<<point_aid<<", rc=" << rc; break; }
                         break;

                       // Прочитать значение значимых атрибутов и занести их в список AttributeInfo_t
                       case rtQUERY_SBS_READ_POINTS_ARMED:
                       // -----------------------------------------------
                         point_info = new xdb::PointDescription_t;
                         // Загрузка нужных группе подписки атрибутов текущей Точки
                         rc = LoadPointInfo(handler, t, point_aid, point_info);
                         // В случае единичной ошибки чтения продолжить работу, пропустив сбойную точку
                         if (rc)
                         {
                           LOG(WARNING) << "Loading attributes for point id="<<point_aid<<", rc=" << rc;
                           // TODO: может быть их тоже вносить в список, но с "больным" качеством?
                           delete point_info;
                         }
                         else
                         {
                           LOG(INFO) << "add tag "<<point_info->tag<<" with "<<point_info->attributes.size() << " attributes";
                           points_list->push_back(point_info);
                         }
                         break;

                       default:
                         LOG(ERROR) << "Unsupported action code: " << action;
                     }
                   }
#if defined VERBOSE
                   else // группа не совпадает, пропустить её
                   {
                     LOG(INFO) << "Skip another SBS with id="<< sbs_aid_current <<" for point id=" << point_aid;
                   }
#endif
                 }
                 rc = mco_cursor_next(t, &csr_sbs);
                 if (rc && (rc != MCO_S_CURSOR_END)) { LOG(ERROR) << "Next SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
               }

             } // Конец поиска групп подписки, содержащих искомую Точку

#if (EXTREMEDB_VERSION >=40)
             rc = mco_cursor_close(t, &csr_sbs);
             if (rc) { LOG(ERROR) << "Close SBS_GROUPS_ITEM cursor, rc=" << rc; break; }
#endif

           } // Конец проверки того, что искомая Точка модифицирована

           rc = mco_cursor_next(t, &csr_point);
           if (rc && (rc != MCO_S_CURSOR_END)) { LOG(ERROR) << "Next XDBPoint cursor, rc=" << rc; break; }

         } // Конец цикла проверки всех Точек на модификации

      } // Конец проверки, есть ли модифицированные точки?

#if (EXTREMEDB_VERSION >=40)
      rc = mco_cursor_close(t, &csr_point);
      if (rc) { LOG(ERROR) << "Close XDBPoint cursor, rc=" << rc; break; }
#endif

    } // Конец успешного создания курсора по индексу модификации

    rc = mco_trans_commit(t);
    if (rc) { LOG(ERROR) << "Commitment transaction, rc=" << rc; break; }

  } while (false);

  if (rc)
  {
    mco_trans_rollback(t);
    setError(rtE_RUNTIME_ERROR);
  }

  return rc;
}

// =================================================================================
// Получить список Точек, имеющих предысторию
// Анализируется значение атрибута XDBPoint.HistoryType:
//   PER_NONE      - предыстория не собирается
//   PER_1_MINUTE  - только минутная
//   PER_5_MINUTES - минутная и пятиминутная
//   PER_HOUR      - минутная, 5-минутная и часовая
//   PER_DAY       - вплоть до суточной включительно
//   PER_MONTH     - до месячной включительно
MCO_RET DatabaseRtapImpl::queryPointsWithHistory (mco_db_h& handler, rtDbCq& info)
{
  const char *fname = "queryPointsWithHistory";
  MCO_RET rc = MCO_E_UNSUPPORTED;
  mco_cursor_t csr;
  mco_trans_h t;
  // Текущий экземпляр Точки
  XDBPoint point_instance;
  // Значение OBJCLASS текущего экземпляра
  objclass_t objclass_instance;
  AnalogInfoType ai;
  // Глубина хранения предыстории экземпляра
  HistoryType htype = PER_NONE;
  // Входной параметр - адрес хеша с выходными данными
  map_name_id_t *points_map = static_cast<map_name_id_t*>(info.buffer);
//  map_name_id_t::iterator points_map_iterator;
  // Буфер для чтения значения тега Точки
  const uint2 tag_size = sizeof(wchar_t)*TAG_NAME_MAXLEN;
  char tag[tag_size + 1];

  assert(info.buffer);
  points_map->clear();

  do {
    rc = mco_trans_start(handler, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction: " << mco_ret_string(rc,NULL); break; }

    rc = XDBPoint_list_cursor(t, &csr);
    if (rc) { LOG(ERROR) << "Get Points list: " << mco_ret_string(rc,NULL); break; }

    while((MCO_S_OK == rc) || (rc != MCO_S_CURSOR_EMPTY) || (rc != MCO_S_CURSOR_END)) {
      rc = XDBPoint_from_cursor(t, &csr, &point_instance);

      if (rc) { LOG(ERROR) << "Get point instance from cursor: " << mco_ret_string(rc,NULL); break; }

      // Проверить тип атрибута VAL - аналоговый, дискретный или бинарный?
      rc = XDBPoint_OBJCLASS_get(&point_instance, &objclass_instance);
      if (rc) {
        LOG(ERROR) << "Unable to get point's objclass: " << mco_ret_string(rc,NULL);
        // break пропущен специально
      }

      // Интересуют Точки только аналогового типа
      if (PROCESSING_ANALOG == get_type_by_objclass(objclass_instance)) {

        // Проверить признак глубины хранения предыстории
        rc = XDBPoint_ai_read_handle(&point_instance, &ai);
        if (rc) {
          LOG(ERROR) << "Unable to get point's history type depth: " << mco_ret_string(rc,NULL);
        }
        else {
          rc = AnalogInfoType_HISTOTYPE_get(&ai, &htype);
          if (rc) {
            LOG(ERROR) << "Unable to get point's history type depth: " << mco_ret_string(rc,NULL);
            // break пропущен специально
          }
        }

        switch(htype) {
          case PER_NONE:
            // такая точка нам не интересна
            break;

          case PER_MONTH:
          case PER_DAY:
          case PER_HOUR:
          case PER_5_MINUTES:
          case PER_1_MINUTE:
            rc = XDBPoint_TAG_get(&point_instance, tag, tag_size);
            if (rc) {
              LOG(ERROR) << "Unable to get point's tag: " << mco_ret_string(rc,NULL);
              // break пропущен специально
              // Продолжить работу, даже если была ошибка чтения тега историзируемых Точек
            }
            else {
              // Занесем реквизиты текущей точки в список
              // NB: вместо идентификатора Точки заносим значение глубины хранения истории
              points_map->insert(std::pair<std::string, autoid_t>(tag, static_cast<autoid_t>(htype)));
#ifdef VERBOSE
              LOG(INFO) << "Get historized point '" << tag << "', history depth=" << htype;
#endif
            }
            break;
        }
      } // Конец обработки Точки аналогового типа

      rc = mco_cursor_next(t, &csr);
      // Всего лишь конец курсора, очистим ошибку и выйдем из цикла
      if (MCO_S_CURSOR_END == rc) { rc = MCO_S_OK; break; }
    }

    if (MCO_S_OK == rc) {
      rc = mco_trans_commit(t);
      if (rc) { LOG(ERROR) << "Commitment transaction: " << mco_ret_string(rc,NULL); break; }
    }
  } while (false);

  if (rc) {
    LOG(ERROR) << "Query points with history traitments: " << mco_ret_string(rc,NULL);
    mco_trans_rollback(t);
  }

  LOG(INFO) << fname << ": Loaded " << points_map->size() << " history element(s)";

  return rc;
}

