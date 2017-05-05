/*
 * ITG
 * author: Eugeniy Gorovoy
 * email: eugeni.gorovoi@gmail.com
 *
 * Внимание: набор используемых на объекте управления СЕ различается
 * в зависимости от типа объекта
 * В ЛПУ:
 *   GOFTSCAT2
 *   GOFTSCDA
 *   GOFTSCGR2
 *   GOFTSCSA
 *   GOFTSCVA
 *   GOFTSCX2
 *   --------
 *   GOFVALAL
 *   GOFVALRE
 *   GOFVALTI
 *
 * В ГТП:
 *   GOFVALRE
 *   GOFVALTI
 *   GOFVALTSCDIR
 *
 *
 */
#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"
#include "xdb_impl_database.hpp"
#include "xdb_impl_db_rtap.hpp"

using namespace xdb;

/*
 * Включение динамически генерируемых определений
 * структуры данных для внутренней базы RTAP.
 *
 * Регенерация осуществляется командой mcocomp
 * на основе содержимого файла rtap_db.mco
 */
#include "dat/rtap_db.hpp"

MCO_RET DatabaseRtapImpl::GOFVALTI(mco_trans_h t,
        XDBPoint*   obj,
        AnalogInfoType& ai,
        const char* tag,
        objclass_t  objclass,
        ValidChange h_Validchange,  // 00 VALIDCHANGE
        double&     g_Valacq,       // 01 VALACQ
        timestamp&  d_Dathourm,     // 02 DATEHOURM
        double&     g_ValManual,    // 03 VALMANUAL
        Validity    b_Validacq,     // 04 VALIDACQ
        double&     g_Val,          // 05 VAL
        Validity    h_Valid,        // 06 VALID
        SystemState o_SaState,      // 07 SASTATE
        SystemState o_OldSaSt,      // 08 OLDSASTATE
        bool        b_InhibLocal,   // 09 INHIBLOCAL
        bool        b_Inhib         // 10 INHIB
        )
{
  static const char *fname = "GOFVALTI";
  MCO_RET rc = MCO_S_NOTFOUND;
  // Признак того, был ли модифицирован один из атрибутов данной точки,
  // подлежащий проверке в группах подписки
  bool point_was_modified = false;
  Validity h_Valid_Out = h_Valid;
//  Boolean logic;

  do
  {
    // Выбор алгоритма действия в зависимости от нового значения VALIDCHANGE
    switch (h_Validchange)
    {
      // ------------------------------------------------------------
      case VALIDCHANGE_FAULT:   /* 0 */
        if (o_SaState == SS_WORK)
        {
          switch(h_Valid)
          {
            case VALIDITY_VALID:
            case VALIDITY_NO_INSTRUM:
              h_Valid_Out = VALIDITY_FAULT;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
              // TODO: update the information list SIL_TRT_D_MSG_FAULT_INF_LIS_PUT
            break;

            case VALIDITY_FAULT:
            case VALIDITY_INQUIRED:
              if (b_Validacq == VALIDITY_VALID)
              {
                // TODO: Add message into the invalid information list SIL_TRT_D_MSG_FAULT_INF_LIS_PUT

                // the acquired value is the first after a SA unaccessibility
                b_Validacq = VALIDITY_FAULT;
                rc = XDBPoint_VALIDITY_ACQ_put(obj, b_Validacq);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALIDACQ<<"', rc="<<rc; break; }
              }
            break;

            default:
                b_Validacq = VALIDITY_FAULT;
                rc = XDBPoint_VALIDITY_ACQ_put(obj, b_Validacq);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALIDACQ<<"', rc="<<rc; break; }
          } // конец switch разбора достоверности

          point_was_modified = true;

        } // конец обработки СС в рабочем состоянии
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_VALID:   /* 1 */
        if (o_SaState == SS_WORK /*GOF_D_BDR_SASTATE_OP*/)
        {
          switch(h_Valid)
          {
            case VALIDITY_INHIBITION:
            case VALIDITY_MANUAL:
            case VALIDITY_FAULT_INHIB:
            case VALIDITY_FAULT_FORCED:
                b_Validacq = VALIDITY_VALID;
                rc = XDBPoint_VALIDITY_ACQ_put(obj, b_Validacq);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALIDACQ<<"', rc="<<rc; break; }
                point_was_modified = true;
            break;

            case VALIDITY_INQUIRED:
            case VALIDITY_NO_INSTRUM:
                g_Val = g_Valacq;
                rc = AnalogInfoType_VAL_put(&ai, g_Val);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

                h_Valid_Out = VALIDITY_VALID;
                rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

                // If the fault was on the SA, remove the individual fault
                if (b_Validacq == VALIDITY_FAULT)
	            {
                  // TODO: был сбой в СС, удалить ТИ из списка сбойных
                  // Remove message from the invalid information list

                  // Обновить
                  b_Validacq = VALIDITY_VALID;
                  rc = XDBPoint_VALIDITY_ACQ_put(obj, b_Validacq);
                  if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALIDACQ<<"', rc="<<rc; break; }
                }
                point_was_modified = true;
            break;

            case VALIDITY_FAULT:
              g_Val = g_Valacq;
              rc = AnalogInfoType_VAL_put(&ai, g_Val);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

              h_Valid_Out = VALIDITY_VALID;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

              // If the fault was on the SA, remove the individual fault
              if (b_Validacq == VALIDITY_FAULT)
	          {
                  // TODO: был сбой в СС, удалить ТИ из списка сбойных

                  // Обновить
                  b_Validacq = VALIDITY_VALID;
                  rc = XDBPoint_VALIDITY_ACQ_put(obj, b_Validacq);
                  if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALIDACQ<<"', rc="<<rc; break; }
              }
              point_was_modified = true;
            break;

            case VALIDITY_VALID:
              g_Val = g_Valacq;
              rc = AnalogInfoType_VAL_put(&ai, g_Val);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }
              point_was_modified = true;
            break;

            default:
              LOG(ERROR) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
          }
        } // Конец действий если СС в рабочем режиме
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_FORCED:  /* 2 */
        g_Val = g_ValManual;

        rc = AnalogInfoType_VAL_put(&ai, g_Val);
        if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

        switch(h_Valid)
        {
          case VALIDITY_VALID:
            h_Valid_Out = VALIDITY_MANUAL;
            // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT

            // Set the local inhibition into Database
            b_InhibLocal = true;
            rc = XDBPoint_INHIBLOCAL_put(obj, TRUE);
            if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }
          break;

          case VALIDITY_INHIBITION:
            h_Valid_Out = VALIDITY_MANUAL;

            // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT

            if (b_InhibLocal == true)
            {
              // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_SUP
            }
            else
            {
              // Set the local inhibition into Database
              b_InhibLocal = true;
              rc = XDBPoint_INHIBLOCAL_put(obj, TRUE);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }
            }
          break;

          case VALIDITY_FAULT:
          case VALIDITY_INQUIRED:
            if ( b_Validacq == true )
            {
              h_Valid_Out = VALIDITY_MANUAL;
            }
            else
            {
              h_Valid_Out = VALIDITY_FAULT_FORCED;
            }

            // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT

            // Set the local inhibition into Database
            b_InhibLocal = true;
            rc = XDBPoint_INHIBLOCAL_put(obj, TRUE);
            if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }
          break;

          case VALIDITY_FAULT_INHIB:
            h_Valid_Out = VALIDITY_FAULT_FORCED;
            // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT

            if (b_InhibLocal == true)
            {
              // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_SUP
            }
            else
            {
              // Set the local inhibition into Database
              b_InhibLocal = true;
              rc = XDBPoint_INHIBLOCAL_put(obj, TRUE);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }

            }
          break;

          case VALIDITY_MANUAL:
          case VALIDITY_FAULT_FORCED:
            if (b_InhibLocal == true)
            {
              // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_MOD
            }
          break;

          case VALIDITY_NO_INSTRUM:
             // TODO: Send message to update the information list SIL_TRT_D_MSG_MANUAL_INF_LIS_MOD
          break;

          case VALIDITY_GLOBAL_FAULT:
            // ???
          break;
        }

        point_was_modified = true;

      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_INHIB:   /* 3 */
        switch(h_Valid)
        {
          case VALIDITY_VALID:
          case VALIDITY_FAULT:
          case VALIDITY_INQUIRED:
            if (h_Valid == VALIDITY_VALID)
            {
              h_Valid_Out = VALIDITY_INHIBITION;
              // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_PUT
            }
            else if (h_Valid == VALIDITY_FAULT)
            {
              if (b_Validacq == VALIDITY_VALID)
              {
                h_Valid_Out = VALIDITY_INHIBITION;
              }
              else
              {
                h_Valid_Out = VALIDITY_FAULT_INHIB;
              }
              // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_PUT
            }
            else if (h_Valid == VALIDITY_INQUIRED)
            {
              if (b_Validacq == VALIDITY_VALID)
              {
                h_Valid_Out = VALIDITY_MANUAL;
              }
              else
              {
                h_Valid_Out = VALIDITY_FAULT_FORCED;
              }

              // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT

              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
              point_was_modified = true;

              // TODO: Set the local inhibition into Database
              b_InhibLocal = true;
              //1LOG(INFO) << fname << ": set " <<tag<< "."<<RTDB_ATT_INHIBLOCAL << " = " << (int) b_InhibLocal;
              rc = XDBPoint_INHIBLOCAL_put(obj, TRUE);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }
            }
          break;

          default:
            LOG(ERROR) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
        }
        rc = MCO_S_OK;
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_MANUAL:  /* 4 */
        switch (h_Valid)
        {
          case VALIDITY_INQUIRED:
          case VALIDITY_MANUAL:
          case VALIDITY_FAULT_FORCED:
          case VALIDITY_NO_INSTRUM:
          case VALIDITY_FAULT:
          case VALIDITY_INHIBITION:
          case VALIDITY_FAULT_INHIB:

            g_Val = g_ValManual;

            rc = AnalogInfoType_VAL_put(&ai, g_Val);
            if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

            if (h_Valid == VALIDITY_NO_INSTRUM)
            {
              // TODO: Send message to update the information list SIL_TRT_D_MSG_MANUAL_INF_LIS_MOD
            }
            else if ((h_Valid == VALIDITY_MANUAL) || (h_Valid == VALIDITY_FAULT_FORCED))
            {
              if (b_InhibLocal == true)
              {
                // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_MOD
              }
            }
            else if (h_Valid == VALIDITY_FAULT)
            {
              h_Valid_Out = VALIDITY_INQUIRED;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
            }
            else if (h_Valid == VALIDITY_INHIBITION)
            {
              h_Valid_Out = VALIDITY_MANUAL;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

              if (b_InhibLocal == true)
              {
                // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_SUP
                // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT
              }
            }
            else if (h_Valid == VALIDITY_FAULT_INHIB)
            {
              h_Valid_Out = VALIDITY_FAULT_FORCED;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
              // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_SUP
              // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_PUT
            }

            point_was_modified = true;
          break;

          default:
            LOG(ERROR) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
        }
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_END_INHIB:   /* 5 */
      case VALIDCHANGE_END_FORCED:  /* 6 */
        switch (h_Valid)
        {
          case VALIDITY_FAULT_INHIB:
		  case VALIDITY_FAULT_FORCED:
		  case VALIDITY_MANUAL:
		  case VALIDITY_INHIBITION:

            // Inhibition is over if both the SA and the upper level equipment
            // are no more inhibited

            if ((o_SaState != SS_INHIB) && (b_Inhib == false))
            {
              //  Last acquisition was valid
              if (b_Validacq == true)
              {
                if ((h_Valid == VALIDITY_FAULT_INHIB) || (h_Valid == VALIDITY_FAULT_FORCED))
                {
                  // Acquistion before inhibition was in fault
                  // TODO: Send message to update the fault information list SIL_TRT_D_MSG_FAULT_INF_LIS_SUP
                }

                if (o_SaState == SS_WORK)
                {
                  h_Valid_Out = VALIDITY_VALID;
                }
                else
                {
                  // TODO: проверить корректность присваивания h_Valid_Out в VALIDITY_FAULT
                  if ((h_Valid == VALIDITY_FAULT_INHIB)
                   || (h_Valid == VALIDITY_INHIBITION)
                   || (h_Valid == VALIDITY_FAULT_FORCED)
                   || (h_Valid == VALIDITY_MANUAL))
                  {
                    h_Valid_Out = VALIDITY_FAULT;
                  }
                }

                // Write the current validity into Database
                rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
              }
              else // b_Validacq == false
              {
                if ((h_Valid == VALIDITY_INHIBITION) || (h_Valid == VALIDITY_MANUAL))
                {
                  // Acquistion before inhibition was valid
                  // TODO: Send message to update the information list SIL_TRT_D_MSG_FAULT_INF_LIS_PUT
                }

                // TODO: проверить корректность присваивания h_Valid_Out в VALIDITY_FAULT
                if ((h_Valid == VALIDITY_FAULT_INHIB)
                 || (h_Valid == VALIDITY_INHIBITION)
                 || (h_Valid == VALIDITY_FAULT_FORCED)
                 || (h_Valid == VALIDITY_MANUAL))
                 {
                   h_Valid_Out = VALIDITY_FAULT;
                 }

                 rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
                 if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }
              } // end if b_Validacq == false

            } // end if SA inhib

            g_Val = g_Valacq;
            rc = AnalogInfoType_VAL_put(&ai, g_Val);
            if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

            if (b_InhibLocal == true)
            {
              if (h_Valid == VALIDITY_FAULT_INHIB || h_Valid == VALIDITY_INHIBITION)
              {
                // TODO: Send message to update the information list SIL_TRT_D_MSG_INHIB_INF_LIS_SUP
              }
              else
              {
                // TODO: Send message to update the information list SIL_TRT_D_MSG_FORCED_INF_LIS_SUP
              }

              // Reset the local inhibition into Database
              b_InhibLocal = false;
              //1LOG(INFO) << fname << ": set " <<tag<< "."<<RTDB_ATT_INHIBLOCAL << " = " << (int) b_InhibLocal;
              rc = XDBPoint_INHIBLOCAL_put(obj, FALSE);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_INHIBLOCAL<<"', rc="<<rc; break; }
            }

            point_was_modified = true;
          break;

          default:
            LOG(WARNING) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
        }

      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_INHIB_GBL:   /* 7 */

        switch(h_Valid)
        {
          case VALIDITY_FAULT_INHIB:
          case VALIDITY_FAULT_FORCED:
          case VALIDITY_MANUAL:
          case VALIDITY_INHIBITION:

          // No other inhibition (SA, eqt on another level, local) must remain
          if ((o_SaState != SS_INHIB) && (b_Inhib == false) && (b_InhibLocal == false))
          {
            if (b_Validacq == true)
            {
              h_Valid_Out = VALIDITY_VALID;
              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

              if (h_Valid == VALIDITY_FAULT_INHIB || h_Valid == VALIDITY_FAULT_FORCED)
              {
                // Acquistion before inhibition was in fault
                // TODO: Send message to update the fault information list SIL_TRT_D_MSG_FAULT_INF_LIS_SUP
              }
            }
            else if (b_Validacq == false)
            {
              if ((h_Valid == VALIDITY_INHIBITION)
		       || (h_Valid == VALIDITY_FAULT_INHIB)
		       || (h_Valid == VALIDITY_MANUAL)
		       || (h_Valid == VALIDITY_FAULT_FORCED))
              {
                h_Valid_Out = VALIDITY_FAULT;
              }

              rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
              if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

              if (h_Valid == VALIDITY_INHIBITION || h_Valid == VALIDITY_MANUAL)
              {
                // Acquistion before inhibition was not in fault
                // TODO: Send message to update the fault information list SIL_TRT_D_MSG_FAULT_INF_LIS_PUT
              }
            }
          }

          // Write the current value into Database
          g_Val = g_Valacq;
          rc = AnalogInfoType_VAL_put(&ai, g_Val);
          if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

          point_was_modified = true;

          break;

          default:
            LOG(WARNING) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
        } // end switch
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_END_INHIB_GBL:   /* 8 */

        if (h_Valid == VALIDITY_VALID)
        {
          h_Valid_Out = VALIDITY_INHIBITION;
        }
        else if (h_Valid == VALIDITY_FAULT)
        {
          h_Valid_Out = VALIDITY_FAULT_INHIB;
        }
        else if (h_Valid == VALIDITY_INQUIRED)
        {
          h_Valid_Out = VALIDITY_FAULT_FORCED;
        }

        rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
        if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

        point_was_modified = true;
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_NULL:    /* 9 */
        // Ничего не делать
        rc = MCO_S_OK;
        point_was_modified = false;
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_FAULT_GBL:   /* 10 */
        if (h_Valid == VALIDITY_VALID)
        {
          h_Valid_Out = VALIDITY_FAULT;
          rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
          if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

          point_was_modified = true;
        }
        // NB: VALIDACQ is not updated and no lines are added into the fault invalid information list
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_INHIB_SA:    /* 11 */
        if (h_Valid == VALIDITY_VALID)
        {
          h_Valid_Out = VALIDITY_INHIBITION;
        }
        else if (h_Valid == VALIDITY_FAULT)
        {
          if (b_Validacq == VALIDITY_VALID)
          {
            h_Valid_Out = VALIDITY_INHIBITION;
          }
          else
          {
            h_Valid_Out = VALIDITY_FAULT_INHIB;
          }
        }
        else if (h_Valid_Out == VALIDITY_INQUIRED)
        {
          if (b_Validacq == VALIDITY_VALID)
          {
            h_Valid_Out = VALIDITY_MANUAL;
          }
          else
          {
            h_Valid_Out = VALIDITY_FAULT_FORCED;
          }
        }

        rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
        if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

        point_was_modified = true;
      break;

      // ------------------------------------------------------------
      case VALIDCHANGE_END_INHIB_SA:    /* 12 */
        switch(h_Valid)
        {
          case VALIDITY_FAULT_INHIB:
          case VALIDITY_FAULT_FORCED:
          case VALIDITY_MANUAL:
          case VALIDITY_INHIBITION:

            if (b_Inhib == false && b_InhibLocal == false)
            {
              if (b_Validacq == true && o_SaState == SS_WORK)
              {
                if (h_Valid == VALIDITY_FAULT_INHIB || h_Valid == VALIDITY_FAULT_FORCED)
                {
                  // Acquistion before inhibition was in fault
                  // TODO: Send message to update the fault information list SIL_TRT_D_MSG_FAULT_INF_LIS_SUP
                }
                h_Valid_Out = VALIDITY_VALID;

                rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

                g_Val = g_Valacq;
                rc = AnalogInfoType_VAL_put(&ai, g_Val);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VAL<<"', rc="<<rc; break; }

                point_was_modified = true;
              }
              else
              {
                if ((h_Valid == VALIDITY_INHIBITION || h_Valid == VALIDITY_MANUAL)
                 && (b_Validacq == false))
                {
                  // TODO: Send message to update the information list SIL_TRT_D_MSG_FAULT_INF_LIS_PUT
                }

                if ((h_Valid == VALIDITY_INHIBITION)
                 || (h_Valid == VALIDITY_FAULT_INHIB)
                 || (h_Valid == VALIDITY_MANUAL)
                 || (h_Valid == VALIDITY_FAULT_FORCED))
                {
                  h_Valid_Out = VALIDITY_FAULT;
                }

                rc = XDBPoint_VALIDITY_put(obj, h_Valid_Out);
                if (rc) { LOG(ERROR) << "Write '"<<tag<< "."<<RTDB_ATT_VALID<<"', rc="<<rc; break; }

                point_was_modified = true;
              }
           } /* end if SA inhib ou Eqt inhib */

          break;

          default:
            LOG(ERROR) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALID << "=" << h_Valid;
        }

      break;

      default:
        LOG(ERROR) << fname << ": Unhandled "<<tag<<"." << RTDB_ATT_VALIDCHANGE << "=" << h_Validchange;
    }

  } while(false);

  // TODO: Если код завершения операции с БДРВ успешный, и один из атрибутов данной
  // точки был модифицирован, то взвести флаг наличия изменений в тех группах
  // подписки, где встречается данная точка

  // Если была ошибка, но обработка VALIDCHANGE не проводилась - сбросить ошибку.
  // Это произошло из-за значения rc по-умолчанию (MCO_S_NOTFOUND)
  if ((rc) && (false == point_was_modified))
  {
    rc = MCO_S_OK;
  }
  else
  {
    // Взведем признак модификации атрибутов для последующей проверки группой подписки
    rc = XDBPoint_is_modified_put(obj, TRUE);
    if (rc)
    {
      LOG(ERROR) << "Set '"<<tag<<"' modified flag, rc="<<rc;
    }
    else LOG(INFO) << "Set modify flag for '"<<tag<<"'"; //1
  }


  return rc;
}

// NB: Работает только на уровне ЛПУ
#warning "GOFVALAL is applicable to DIPL only"
MCO_RET DatabaseRtapImpl::GOFVALAL(mco_trans_h t,
        XDBPoint *obj,
        DiscreteInfoType&,
        const char* tag,
        objclass_t objclass,
        ValidChange,
        uint64_t&,
        timestamp&,
        uint64_t&,
        Validity,
        uint64_t&,
        Validity,
        SystemState,
        SystemState)
{
  static const char *fname = "GOFVALAL";
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(objclass)
  {
    case TS:
    case TSA:
    case VA:
    case ATC:
    case AUX1:
    case AUX2:
    case AL:
    case ICS:
      break;

    default:
      LOG(ERROR) << fname << ": '" << tag
                 << "' for objclass " << objclass
                 << " is not supported";
  }
  return rc;
}

// Поместить факт изменения состояния объекта в таблицу EQV
MCO_RET DatabaseRtapImpl::GOFVALTSC(mco_trans_h t,
        XDBPoint *obj,
        DiscreteInfoType&,
        const char* tag,
        objclass_t objclass,
        ValidChange,
        uint64_t&,
        timestamp&,
        uint64_t&,
        Validity,
        uint64_t&,
        Validity,
        SystemState,
        SystemState)
{
  static const char *fname = "GOFVALTSC";
  MCO_RET rc = MCO_S_NOTFOUND;

  switch(objclass)
  {
    case TS:
    case TSA:
    case VA:
    case ATC:
    case AUX1:
    case AUX2:
    case AL:
    case ICS:
      break;

    default:
      LOG(ERROR) << fname << ": '" << tag
                 << "' for objclass " << objclass
                 << " is not supported";
      break;
  }

  return rc;
}

// Контроль состояния кранов
// TODO: Прочитать атрибуты состояния концевиков ZSVA|ZSVB и вычислить атрибуты VALID и VAL крана.
// ZSVA : ZSVB : VALICHANGE : VALIDACQ : VALID : VAL
//    0 :    0 :          1 :        1 :     0 :   недостоверно (открывается/закрывается?)
//    0 :    1 :          1 :        1 :     1 :   закрыт
//    1 :    0 :          1 :        1 :     1 :   открыт
//    1 :    1 :          1 :        1 :     0 :   недостоверно (открывается/закрывается?)
//    0 :    0 :          2 :        1 :     2 :   недостоверно (открывается/закрывается?)
//    0 :    1 :          2 :        1 :     2 :   закрыт
//    1 :    0 :          2 :        1 :     2 :   открыт
//    1 :    1 :          2 :        1 :     2 :   недостоверно (открывается/закрывается?)
//
// Для остальных VALIDCHANGE:
MCO_RET DatabaseRtapImpl::GOFTSCVA(mco_trans_h t,
        XDBPoint   *obj,
        DiscreteInfoType& di,
        const char *tag,
        objclass_t  objclass,
        ValidChange h_Validchange,  // 00 VALIDCHANGE
        Validity    b_Validacq      // 01 VALIDACQ
        )
{
  static const char *fname = "GOFTSCVA";
  MCO_RET rc = MCO_S_NOTFOUND;
  ValidChange h_AcqVty = VALIDCHANGE_FAULT; // local_Validchange;
  ValidChange h_AcqVty_Re = VALIDCHANGE_FAULT; // результирующая достоверность

  if (objclass == VA)
  {
    if ( h_AcqVty != VALIDCHANGE_VALID &&
	     h_AcqVty != VALIDCHANGE_FORCED)
    {
      h_AcqVty_Re = VALIDCHANGE_FAULT;
    }

  }
  else
  {
    LOG(ERROR) << fname << ": '" << tag
               << "' for objclass " << objclass
               << " is not supported";
  }

  return rc;
}

