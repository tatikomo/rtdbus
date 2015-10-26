#include <iostream>
#include <string>

#include <time.h>   // struct timeval
#include <sys/time.h> // gettimeofday
#include <string.h> // memset

#include "ce_unittest.hpp"

// Глобальный вектор значений атрибутов
// Циклически изменяются значения, читаемые затем в функции rtPopValue
rtValue     g_val[SIL_TRT_CAL_VALAL_MAXPARAMS];
rtIndexer   g_indexer[SIL_TRT_CAL_VALAL_MAXPARAMS];
// Векторы для резервирования значений 
rtValue     g_val_bak[SIL_TRT_CAL_VALAL_MAXPARAMS];
rtIndexer   g_indexer_bak[SIL_TRT_CAL_VALAL_MAXPARAMS];
// Используется в rtPopValue для записи в добавленное поле name.
// RTAP не содержит этого поля, я добавил его для трассировки, чтобы определять
// атрибут не по его индексу, а по названию, что наглядно проще.
// Список названий атрибутов на основании порядка заполнения pr_val[]
unsigned int g_stack_idx = 0;
const char* g_attribute_name[] = {
    "VALIDCHANGE",
    "VALACQ",
    "DATEHOURM",
    "VALIDACQ",
    "VAL",
    "VALID",
    "SASTATE",
    "OLDSASTATE",
    "INHIBLOCAL",
    "INHIB"
};

// сохранить текущие значения в резервной копии
void push()
{
  memcpy(&g_val_bak,     g_val,     sizeof(rtValue)   * SIL_TRT_CAL_VALAL_MAXPARAMS);
  memcpy(&g_indexer_bak, g_indexer, sizeof(rtIndexer) * SIL_TRT_CAL_VALAL_MAXPARAMS);
}

// восстановить значения из резервной копии
void pop()
{
  memcpy(&g_val,     g_val_bak,     sizeof(rtValue)   * SIL_TRT_CAL_VALAL_MAXPARAMS);
  memcpy(&g_indexer, g_indexer_bak, sizeof(rtIndexer) * SIL_TRT_CAL_VALAL_MAXPARAMS);
}

int rtCeRead(rtIndexer& ind, void* quality, void* value)
{
//
  switch (ind.ain)
  {
      case VALIDCHANGE_IDX:     // 0
      case VALACQ_IDX:          // 1
      case VALIDACQ_IDX:        // 3
      case VAL_IDX:             // 4
      case VALID_IDX:           // 5
      case INHIBLOCAL_IDX:      // 8
      case INHIB_IDX:           // 9
        *(rtUInt16*)value = *(rtUInt16*)g_val[ind.ain].operand.data;
        std::cout << "rtCeRead("<<ind.plin<<":"<<(unsigned int)ind.ain<<")="<< *(rtUInt16*)value << std::endl;
        break;
      case DATEHOURM_IDX:       // 2
        *(rtUInt32*)value = *(rtUInt32*)g_val[ind.ain].operand.data;
        std::cout << "rtCeRead("<<ind.plin<<":"<<(unsigned int)ind.ain<<")="<<*(rtUInt32*)value << std::endl;
        break;
      case SASTATE_IDX:         // 6
      case OLDSASTATE_IDX:      // 7
        *(rtUInt8*)value = *(rtUInt8*)g_val[ind.ain].operand.data;
        std::cout << "rtCeRead("<<ind.plin<<":"<<(unsigned int)ind.ain<<")="<<*(rtUInt16*)value << std::endl;
        break;
  }

  *(rtDbQuality*) quality = rtATTR_OK;

  return rtSUCCESS;
}

rtValue* rtPopValue()
{
  // Эмулированные значения 
  rtUInt8  val_8  = 0;
  rtUInt16 val_16 = 0;
  rtUInt32 val_32 = 0;
  // стек эмулируется сдвигом в массиве
  rtValue* item = &g_val[g_stack_idx];

  // TODO: если это не VALIDCHANGE или SASTATE, в operand.data возвращать rtIndexer
  if ((0 == strcmp(item->name, "VALIDCHANGE"))
   || (0 == strcmp(item->name, "SASTATE")))
  {
    switch(item->operand.deType)
    {
      case rtUINT8:
        val_32 = *(rtUInt8*) item->operand.data;
        break;
      case rtUINT16:
        val_32 = *(rtUInt16*) item->operand.data;
        break;
      case rtUINT32:
        val_32 = *(rtUInt32*) item->operand.data;
        break;
      default:
        std::cerr << "E: rtPopValue - unsuported operand type="<<item->operand.deType;
    }
    std::cout << " " << val_32;
  }
  else
  {
    memcpy((void*)item->operand.data, &g_indexer[g_stack_idx], sizeof(rtIndexer));
  }

  g_stack_idx++;

  return item;
}

int rtCeTrigger(rtPlin plin, rtAin ain, int phase)
{
  std::cout << "rtCeTrigger("<<plin<<":"<<(unsigned int)ain<<", "<<phase<<")" << std::endl;
  return rtSUCCESS;
}

void rtIncValueStack()
{
#if 0
  std::cout << "rtIncValueStack()" << std::endl;
#else
  std::cout << std::endl;
#endif
  return;
}

int rtCeWriteOutput(rtIndexer& ind, void* data)
{
  std::cout << "rtCeWriteOutput(" << ind.plin<<":"<<(unsigned int)ind.ain << ", " << data << ")" << std::endl;
  return rtSUCCESS;
}

int rtCeFatalError(const char* msg, rtValue* data, rtDeType type, int)
{
  std::cout << "rtCeFatalError('" << msg
            << "', " << data
            << ", " << type
            << ")" << std::endl;
  return rtFAILED;
}

gof_t_Status gof_tim_p_GetTimeOfDay(rtTime* pd_date)
{
  gof_t_Status i_ret = GOF_D_NOERROR;
  struct timeval r_date;
 
  i_ret = gettimeofday(&r_date, NULL);
 
  /* value the gof_t_Date parameter */
  pd_date->tv_sec = r_date.tv_sec;
  pd_date->tv_usec = r_date.tv_usec;

  return i_ret;
}

gof_t_Status sig_ext_msg_p_OutSendInternMessage
(
  gof_t_Int32     i_MessType,
  gof_t_Int32     i_MessSize,
  gof_tp_Void     p_MessBody
)
{
  gof_t_Status i_ret = GOF_D_NOERROR;
  std::cout << "sig_ext_msg_p_OutSendInternMessage("
            << i_MessType << ", "
            << i_MessSize << ", "
            << p_MessBody
            << ")" << std::endl;
  return i_ret;
}

// TODO: занести в operand.data содержимое rtIndexer
void set_init_val()
{
  rtUInt8  val_8  = 0;
  rtUInt16 val_16 = 0;
  rtUInt32 val_32 = 0;

  for (int attr_idx = 0; attr_idx < SIL_TRT_CAL_VALAL_MAXPARAMS; attr_idx++)
  {
    g_val[attr_idx].quality = rtATTR_OK;

    memset(&g_indexer[attr_idx], '\0', sizeof(rtIndexer));
    memset(g_val[attr_idx].operand.data, '\0', sizeof(rtOperand));

    // У всех атрибутов одинаковый PLIN, но разные AIN-ы
    g_indexer[attr_idx].plin = 1001;
    g_indexer[attr_idx].ain = attr_idx;

#if 1
//    *(rtUInt16*)g_val[attr_idx].operand.data = g_indexer[attr_idx].ain;
#else
    memcpy(g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
#endif

    // этого поля нет в оригинальной структуре RTAP, добавлено для наглядности
    strcpy(g_val[attr_idx].name, g_attribute_name[attr_idx]);
    g_val[attr_idx].quality = rtATTR_OK;

    g_val[attr_idx].operand.operandType = rtSCALAR_ELEMENT;
    g_val[attr_idx].operand.numElems = 1;
    // По умолчанию у всех атрибутов тип 2 байта
    // Если это не так, изменим тип в следующем switch
    g_val[attr_idx].operand.deType = rtUINT16;

    switch (attr_idx)
    {
      case VALIDCHANGE_IDX:     // 0
        val_16 = GOF_D_BDR_VALIDCHANGE_VALID;
        // Для GOFVALAL VALIDCHANGE передается значением
        *(rtUInt16*)g_val[attr_idx].operand.data = val_16;
        break;
      case VALACQ_IDX:          // 1
        val_16 = 234;
        // Для GOFVALAL VALACQ передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case DATEHOURM_IDX:       // 2
        val_32 = time(0);
        g_val[attr_idx].operand.deType = rtUINT32;
        // Для GOFVALAL DATEHOURM передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case VALIDACQ_IDX:        // 3
        val_16 = GOF_D_BDR_VALID_VALID;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case VAL_IDX:             // 4
        val_16 = 123;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case VALID_IDX:           // 5
        val_16 = GOF_D_BDR_VALID_VALID;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case SASTATE_IDX:         // 6
        val_8 = GOF_D_BDR_SASTATE_OP;
        g_val[attr_idx].operand.deType = rtUINT8;
        // Для GOFVALAL SASTATE передается значением
        *(rtUInt8*)g_val[attr_idx].operand.data = val_16;
        break;
      case OLDSASTATE_IDX:      // 7
        val_8 = GOF_D_BDR_SASTATE_OP;
        g_val[attr_idx].operand.deType = rtUINT8;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case INHIBLOCAL_IDX:      // 8
        val_16 = 0;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
      case INHIB_IDX:           // 9
        val_16 = 0;
        // Для GOFVALAL передается как rtIndexer
        memcpy((void*)g_val[attr_idx].operand.data, &g_indexer[attr_idx], sizeof(rtIndexer));
        break;
    }
  }
}


void set_next_val(rtUInt16 validchange)
{
  g_val[0].quality = rtATTR_OK;
  g_val[0].operand.deType = rtUINT16;
  g_val[0].operand.operandType = rtSCALAR_ELEMENT;
  memcpy(g_val[0].operand.data, &validchange, sizeof(rtUInt16));
}

//
// Если в СЕ параметр передается в [], то он читается в виде значения
// Если в {}, то в виде rtIndexer
//////////////////////////////////////////////////////////////////////////////////////////////
rtInt sil_trt_cal_ValAl(rtUInt16 parm_count)
{
	rtChar *func_name = "GOFVALAL";	/* Funcion name */
	rtDbQuality outQuality;	/* return code  */
	rtInt i_indice;
	rtValue *pr_val[SIL_TRT_CAL_VALAL_MAXPARAMS];
	rtIndexer indexer;
	rtIndexer r_indexer;
	rtIndexer indexer_AL;

	/* index of parameters in out */
	rtIndexer r_Valacq;	/* Acquired value       : VALACQ */
	rtIndexer r_Datehourm;	/* Acquired date        : DATEHOURM */
	rtIndexer r_Validacq;	/* Last acqu. validity  : VALIDACQ */
	rtIndexer r_Val;	/* Current value        : VAL */
	rtIndexer r_Valid;	/* Current validity     : VALID */
	rtIndexer r_InhibLocal;	/* Local inhibition     : INHIBLOCAL */
	rtIndexer r_OldSaState;	/* Old state of SA      : OLDSASTATE */

	/* value of parameters */
	rtUInt16 h_Validchange;	/* Acquired validity    : VALIDCHANGE */
	rtUInt16 h_Valacq;	/* TS Acquired value    : VALACQ */
	rtTime d_Dathourm;	/* Acquired date        : DATEHOURM */
	rtLogical b_Validacq;	/* Last acqu. validity  : VALIDACQ */
	rtUInt16 i_Val;		/* TS Current value     : VAL */
	rtUInt16 h_Valid;	/* Current validity     : VALID */
	rtUInt16 h_Valid_Out;	/* Current validity     : VALID */
	rtUInt8 o_SaState;	/* State of SA          : SASTATE */
	rtUInt8 o_OldSaSt;	/* Old state of SA      : OLDSASTATE */
	rtLogical b_InhibLocal = GOF_D_FALSE;	/* Local inhibition : INHIBLOCAL */
	rtLogical b_Inhib = GOF_D_FALSE;	/* Eqt inhibition : ^.INHIB */

	rtDbQuality quality;
	rtUInt8 buffer;

//1	rtChar s_inhib[20];
	rtLogical b_inhib;

	gof_t_Int32 i_mess_type;
	sil_trt_t_mr_msg_InfoList r_msg_InfoList;

/*............................................................................*/

/*
 *******************************************************************************
 *     READ ALL THE PARAMETERS IN  
 *******************************************************************************
*/


	/* 
	 * Read all the parameters in, checking quality as we go. 
	 * This will result in pr_val[1] containing the first parameter 
	 * and in pr_val[1] containing the second parameter ... 
	 */

	outQuality = rtATTR_OK;
	for (i_indice = parm_count - 1; i_indice >= 0; i_indice--) {
		/*  Get the parameters  */

		pr_val[i_indice] = rtPopValue();

		/* set quality to error if error else suspect */

		if ((pr_val[i_indice]->quality != rtATTR_OK) ||
		    (outQuality != rtATTR_OK)) {
			if (outQuality == rtATTR_ERROR ||
			    pr_val[i_indice]->quality == rtATTR_ERROR) {
				outQuality = rtATTR_ERROR;
			} else {
				outQuality = rtATTR_SUSPECT;
			}
		}
	}

	/* 
	 * Set the stack pointer so the return value can be written
	 */

	rtIncValueStack();

	/* To recuperate the parameters */

	h_Validchange = *(rtUInt16 *) pr_val[0]->operand.data;
	r_Valacq = *(rtIndexer *) pr_val[1]->operand.data;
	r_Datehourm = *(rtIndexer *) pr_val[2]->operand.data;
	r_Validacq = *(rtIndexer *) pr_val[3]->operand.data;
	r_Val = *(rtIndexer *) pr_val[4]->operand.data;
	r_Valid = *(rtIndexer *) pr_val[5]->operand.data;
	o_SaState = *(rtUInt8 *) pr_val[6]->operand.data;
	r_OldSaState = *(rtIndexer *) pr_val[7]->operand.data;
	if (parm_count > 8) {
		r_InhibLocal = *(rtIndexer *) pr_val[8]->operand.data;
		b_Inhib = *(rtLogical *) pr_val[9]->operand.data;
	}

	/* If one of the qualities of the attributs is error then   */
	/*  return : h_Validchange ou VALIDCHANGE                   */

	pr_val[0]->operand.deType = rtUINT16;
	pr_val[0]->operand.operandType = rtSCALAR_ELEMENT;
	pr_val[0]->quality = outQuality;

	/* 
	 * If the quality is error then quit 
	 */
	if (pr_val[0]->quality == rtATTR_ERROR) {
		return rtSUCCESS;
	}

	if (parm_count <= 8) {
		return rtSUCCESS;
	}

	/* Read all the parameters values in database */

	/* Read :  d_Dathourm ou DATEHOURM */
	if (rtCeRead(r_Datehourm, &quality, (rtUInt8 *) & d_Dathourm) ==
	    rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}

	/* Read :  b_Validacq ou VALIDACQ */
	if (rtCeRead(r_Validacq, &quality, (rtUInt8 *) & b_Validacq) ==
	    rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}

	/* Read :  h_Valid ou VALID */
	if (rtCeRead(r_Valid, &quality, (rtUInt8 *) & h_Valid) == rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}
	h_Valid_Out = h_Valid;

	/* Read :  o_OldSaSt ou OLDSASTATE */
	if (rtCeRead(r_OldSaState, &quality, (rtUInt8 *) & o_OldSaSt) ==
	    rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}

	if (parm_count > 9) {
		/* Read :  b_InhibLocal ou INHIBLOCAL */
		if (rtCeRead(r_InhibLocal, &quality, (rtUInt8 *) & b_InhibLocal)
		    == rtFAILED) {
			return (rtCeFatalError
				(func_name, pr_val[0], rtUINT16, rtSUCCESS));
		}
	}

	/* Read :  h_Valacq ou VALACQ */
	if (rtCeRead(r_Valacq, &quality, (rtUInt8 *) & h_Valacq) == rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}

	/* Read :  i_Val ou VAL */
	if (rtCeRead(r_Val, &quality, (rtUInt8 *) & i_Val) == rtFAILED) {
		return (rtCeFatalError
			(func_name, pr_val[0], rtUINT16, rtSUCCESS));
	}

	/* To prepare the message about Information list */
	/* with value of teleinformation                 */
	r_msg_InfoList.Value.i_Value = i_Val;

#ifdef TRACE
	printf("Validchange = %d, Valid = %d\n", h_Validchange, h_Valid);
#endif

/*
 *******************************************************************************
 *      PREPARE THE INVALID LIST MESSAGE
 *******************************************************************************
*/
	{
		gof_t_Status i_Status;	/* cr of function call */
/*............................................................................*/
		/* To prepare the invalid list message , recuperate         */
		/* the plin of the teleinformation and the date updating    */

		r_msg_InfoList.i_deType = pr_val[1]->operand.deType;
		r_msg_InfoList.h_IndexTI = r_Valacq.plin;

		/* Recuperate the current date with Timeval format */
		i_Status = gof_tim_p_GetTimeOfDay(&r_msg_InfoList.d_Date);

		/* Default of global fault or inhibition is false i.e. individual */
		r_msg_InfoList.b_Global = GOF_D_FALSE;

	}

/*
 *******************************************************************************
 *     TREATMENT of Acquired_validity  
 *******************************************************************************
*/

	switch (h_Validchange) {

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   VALID  
 *******************************************************************************
*/
	case GOF_D_BDR_VALIDCHANGE_VALID:
		if (o_SaState == GOF_D_BDR_SASTATE_OP) {
			if (h_Valid == GOF_D_BDR_VALID_INHIB ||
			    h_Valid == GOF_D_BDR_VALID_FAULT_INHIB) {

				b_Validacq = GOF_D_TRUE;

				/* Write the last validity into Database */
				if (rtCeWriteOutput
				    (r_Validacq,
				     (rtUInt8 *) & b_Validacq) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}

			} else if (h_Valid == GOF_D_BDR_VALID_FAULT) {

				i_Val = h_Valacq;
				/* Write the current value into Database */
				if (rtCeWriteOutput(r_Val, (rtUInt8 *) & i_Val)
				    == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}

				h_Valid_Out = GOF_D_BDR_VALID_VALID;

				/* Write the current validity into Database */
				if (rtCeWriteOutput
				    (r_Valid,
				     (rtUInt8 *) & h_Valid_Out) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}

				/* If the fault was on the SA, remove the individual fault */

				if (b_Validacq == GOF_D_FALSE) {
					/* Remove message from the invalid information list */
					sig_ext_msg_p_OutSendInternMessage
					    (SIL_TRT_D_MSG_FAULT_INF_LIS_SUP,
					     sizeof(r_msg_InfoList),
					     &r_msg_InfoList);
					b_Validacq = GOF_D_TRUE;
					/* Write the last validity into Database */
					if (rtCeWriteOutput
					    (r_Validacq,
					     (rtUInt8 *) & b_Validacq) ==
					    rtFAILED) {
						return (rtCeFatalError
							(func_name, pr_val[0],
							 rtUINT16, rtSUCCESS));
					}
				}

				/* else the acquired value is the first after a SA unaccessibility */
				/* nothing to do */

			} else if (h_Valid == GOF_D_BDR_VALID_VALID) {

				i_Val = h_Valacq;
				/* Write the current value into Database */
				if (rtCeWriteOutput(r_Val, (rtUInt8 *) & i_Val)
				    == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}
			}
		}
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   FAULTY 
 *******************************************************************************
*/
	case GOF_D_BDR_VALIDCHANGE_FAULT:
		if (o_SaState == GOF_D_BDR_SASTATE_OP) {
			if (h_Valid == GOF_D_BDR_VALID_VALID) {
				h_Valid_Out = GOF_D_BDR_VALID_FAULT;

				/* Write the current validity into Database */
				if (rtCeWriteOutput
				    (r_Valid,
				     (rtUInt8 *) & h_Valid_Out) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}

				/* Send message to update the information list */
				sig_ext_msg_p_OutSendInternMessage
				    (SIL_TRT_D_MSG_FAULT_INF_LIS_PUT,
				     sizeof(r_msg_InfoList), &r_msg_InfoList);

			} else if (h_Valid == GOF_D_BDR_VALID_FAULT) {
				if (b_Validacq == GOF_D_TRUE)

					/* the acquired value is the first after a SA unaccessibility */
				{
					/* Add message into the invalid information list */
					sig_ext_msg_p_OutSendInternMessage
					    (SIL_TRT_D_MSG_FAULT_INF_LIS_PUT,
					     sizeof(r_msg_InfoList),
					     &r_msg_InfoList);
					b_Validacq = GOF_D_FALSE;
					/* Write the last validity into Database */
					if (rtCeWriteOutput
					    (r_Validacq,
					     (rtUInt8 *) & b_Validacq) ==
					    rtFAILED) {
						return (rtCeFatalError
							(func_name, pr_val[0],
							 rtUINT16, rtSUCCESS));
					}
				}

			}

			/* For all cases :              */
			/* Write the last validity into Database */

			b_Validacq = GOF_D_FALSE;
			if (rtCeWriteOutput
			    (r_Validacq,
			     (rtUInt8 *) & b_Validacq) == rtFAILED) {
				return (rtCeFatalError
					(func_name, pr_val[0], rtUINT16,
					 rtSUCCESS));
			}

		}
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   INHIBITED
 *******************************************************************************
*/

	case GOF_D_BDR_VALIDCHANGE_INHIB:
		{
			if (h_Valid == GOF_D_BDR_VALID_VALID ||
			    h_Valid == GOF_D_BDR_VALID_FAULT) {
				if (h_Valid == GOF_D_BDR_VALID_VALID) {
					h_Valid_Out = GOF_D_BDR_VALID_INHIB;

					/* Send message to update the information list */
					sig_ext_msg_p_OutSendInternMessage
					    (SIL_TRT_D_MSG_INHIB_INF_LIS_PUT,
					     sizeof(r_msg_InfoList),
					     &r_msg_InfoList);

				} else if (h_Valid == GOF_D_BDR_VALID_FAULT) {
					if (b_Validacq == GOF_D_TRUE) {
						h_Valid_Out =
						    GOF_D_BDR_VALID_INHIB;
					} else {
						h_Valid_Out =
						    GOF_D_BDR_VALID_FAULT_INHIB;
					}

					/* Send message to update the information list */
					sig_ext_msg_p_OutSendInternMessage
					    (SIL_TRT_D_MSG_INHIB_INF_LIS_PUT,
					     sizeof(r_msg_InfoList),
					     &r_msg_InfoList);

				}

				/* Write the current validity into Database */
				if (rtCeWriteOutput
				    (r_Valid,
				     (rtUInt8 *) & h_Valid_Out) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}

				/* Set the local inhibition into Database */
				b_InhibLocal = GOF_D_TRUE;
				if (rtCeWriteOutput
				    (r_InhibLocal,
				     (rtUInt8 *) & b_InhibLocal) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}
			}

		}
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   INHIBITED_GLOBAL
 *******************************************************************************
*/

	case GOF_D_BDR_VALIDCHANGE_INHIB_GBL:
		{

			r_msg_InfoList.b_Global = GOF_D_TRUE;

			if (h_Valid == GOF_D_BDR_VALID_VALID) {
				h_Valid_Out = GOF_D_BDR_VALID_INHIB;

			} else if (h_Valid == GOF_D_BDR_VALID_FAULT) {
				h_Valid_Out = GOF_D_BDR_VALID_FAULT_INHIB;

			}
			/* Write the current validity into Database */
			if (rtCeWriteOutput(r_Valid, (rtUInt8 *) & h_Valid_Out)
			    == rtFAILED) {
				return (rtCeFatalError
					(func_name, pr_val[0], rtUINT16,
					 rtSUCCESS));
			}
		}

		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   END_INHIB 
 *******************************************************************************
*/

	case GOF_D_BDR_VALIDCHANGE_END_INHIB:
		{

			if (h_Valid == GOF_D_BDR_VALID_FAULT_INHIB ||
			    h_Valid == GOF_D_BDR_VALID_INHIB) {

				/* Inhibition is over if both the SA and the upper level equipment */
				/* are no more inhibited */

				if ((o_SaState != GOF_D_BDR_SASTATE_INHIB)
				    && b_Inhib == GOF_D_FALSE) {
					/* Last acquisition was valid   */

					if (b_Validacq == GOF_D_TRUE) {

						if (h_Valid ==
						    GOF_D_BDR_VALID_FAULT_INHIB)
						{
							/* Acquistion before inhibition was in fault */
							/* Send message to update the fault information list */

							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_SUP,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);
						}
						if (o_SaState ==
						    GOF_D_BDR_SASTATE_OP) {
							h_Valid_Out =
							    GOF_D_BDR_VALID_VALID;

							/* Write the current value into Database */

							i_Val = h_Valacq;
							/* Write the current value into Database */
							if (rtCeWriteOutput
							    (r_Val,
							     (rtUInt8 *) &
							     i_Val) ==
							    rtFAILED) {
								return
								    (rtCeFatalError
								     (func_name,
								      pr_val[0],
								      rtUINT16,
								      rtSUCCESS));
							}
						} else {
							h_Valid_Out =
							    GOF_D_BDR_VALID_FAULT;
						}
						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}
					} else if (b_Validacq == GOF_D_FALSE)
						/* Last acquisition was not valid       */
					{
						if (h_Valid ==
						    GOF_D_BDR_VALID_INHIB) {
							/* Acquistion before inhibition was valid */
							/* Send message to update the information list */

							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_PUT,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);
						}
						if (h_Valid ==
						    GOF_D_BDR_VALID_INHIB
						    || h_Valid ==
						    GOF_D_BDR_VALID_FAULT_INHIB)
						{
							h_Valid_Out =
							    GOF_D_BDR_VALID_FAULT;
						}
						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}
					}

				}
				/* end if SA inhib ou Eqt inhib */
				if (b_InhibLocal == GOF_D_TRUE) {
					if (h_Valid ==
					    GOF_D_BDR_VALID_FAULT_INHIB
					    || h_Valid ==
					    GOF_D_BDR_VALID_INHIB) {
						/* Send message to update the information list */
						sig_ext_msg_p_OutSendInternMessage
						    (SIL_TRT_D_MSG_INHIB_INF_LIS_SUP,
						     sizeof(r_msg_InfoList),
						     &r_msg_InfoList);
					}

					/* Reset the local inhibition into Database */
					b_InhibLocal = GOF_D_FALSE;
					if (rtCeWriteOutput
					    (r_InhibLocal,
					     (rtUInt8 *) & b_InhibLocal) ==
					    rtFAILED) {
						return (rtCeFatalError
							(func_name, pr_val[0],
							 rtUINT16, rtSUCCESS));
					}
				}

			}

		}		/* End case END_INHIB */
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   END_INHIB_GBL
 *     The inhibition of an upper level has been removed
 *     But one inhibition on another level may remain.
 *******************************************************************************
*/

	case GOF_D_BDR_VALIDCHANGE_END_INHIB_GBL:
		{

			if (h_Valid == GOF_D_BDR_VALID_FAULT_INHIB ||
			    h_Valid == GOF_D_BDR_VALID_INHIB) {
/*	 No other inhibition (SA, eqt on another level, local) must remain */
				if (o_SaState != GOF_D_BDR_SASTATE_INHIB &&
				    b_Inhib == GOF_D_FALSE &&
				    b_InhibLocal == GOF_D_FALSE) {

					if (b_Validacq == GOF_D_TRUE) {
						h_Valid_Out =
						    GOF_D_BDR_VALID_VALID;

						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

						i_Val = h_Valacq;
						/* Write the current value into Database */
						if (rtCeWriteOutput
						    (r_Val,
						     (rtUInt8 *) & i_Val) ==
						    rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

						if (h_Valid ==
						    GOF_D_BDR_VALID_FAULT_INHIB)
						{
							/* Acquistion before inhibition was in fault */
							/* Send message to update the fault information list */

							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_SUP,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);

						}
					}

					else if (b_Validacq == GOF_D_FALSE) {
						if (h_Valid ==
						    GOF_D_BDR_VALID_INHIB
						    || h_Valid ==
						    GOF_D_BDR_VALID_FAULT_INHIB)
						{
							h_Valid_Out =
							    GOF_D_BDR_VALID_FAULT;
						}

						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

						if (h_Valid ==
						    GOF_D_BDR_VALID_INHIB) {
							/* Acquistion before inhibition was not in fault */
							/* Send message to update the fault information list */

							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_PUT,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);

						}

					}
				}
			}
			/* end if SA inhib  */
		}		/* End case END_INHIB_GBL */
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   END_INHIB_GBL
 *******************************************************************************
*/
/* Previous status was inhibited, new status is the one before inhibition     */
/* provided no message is accepted from the SA during inhibition	      */

	case GOF_D_BDR_VALIDCHANGE_END_INHIB_SA:
		{

			if (h_Valid == GOF_D_BDR_VALID_FAULT_INHIB ||
			    h_Valid == GOF_D_BDR_VALID_INHIB) {
				if (b_Inhib == GOF_D_FALSE
				    && b_InhibLocal == GOF_D_FALSE) {

					if (b_Validacq == GOF_D_TRUE
					    && o_SaState ==
					    GOF_D_BDR_SASTATE_OP) {
						if (h_Valid ==
						    GOF_D_BDR_VALID_FAULT_INHIB)
						{
							/* Acquistion before inhibition was in fault */
							/* Send message to update the fault information list */

							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_SUP,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);

						}
						h_Valid_Out =
						    GOF_D_BDR_VALID_VALID;

						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

						/* Write the current value into Database */

						i_Val = h_Valacq;
						/* Write the current value into Database */
						if (rtCeWriteOutput
						    (r_Val,
						     (rtUInt8 *) & i_Val) ==
						    rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

					} else {
						if (h_Valid ==
						    GOF_D_BDR_VALID_INHIB
						    && b_Validacq ==
						    GOF_D_FALSE) {
							/* Send message to update the information list */
							sig_ext_msg_p_OutSendInternMessage
							    (SIL_TRT_D_MSG_FAULT_INF_LIS_PUT,
							     sizeof
							     (r_msg_InfoList),
							     &r_msg_InfoList);
						}

						h_Valid_Out =
						    GOF_D_BDR_VALID_FAULT;
						/* Write the current validity into Database */
						if (rtCeWriteOutput
						    (r_Valid,
						     (rtUInt8 *) & h_Valid_Out)
						    == rtFAILED) {
							return (rtCeFatalError
								(func_name,
								 pr_val[0],
								 rtUINT16,
								 rtSUCCESS));
						}

					}

				}
				/* end if SA inhib ou Eqt inhib */
			}
		}
		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   INHIBITED_GLOBAL
 *******************************************************************************
*/
/*  Previous status can be operational or fault				      */
/*  New status is inhibited						      */

	case GOF_D_BDR_VALIDCHANGE_INHIB_SA:
		{
			r_msg_InfoList.b_Global = GOF_D_TRUE;

			if (h_Valid == GOF_D_BDR_VALID_VALID) {
				h_Valid_Out = GOF_D_BDR_VALID_INHIB;
			} else if (h_Valid == GOF_D_BDR_VALID_FAULT) {
				if (b_Validacq == GOF_D_TRUE) {
					h_Valid_Out = GOF_D_BDR_VALID_INHIB;
				} else {
					h_Valid_Out =
					    GOF_D_BDR_VALID_FAULT_INHIB;
				}
			}
			/* Write the current validity into Database */
			if (rtCeWriteOutput(r_Valid, (rtUInt8 *) & h_Valid_Out)
			    == rtFAILED) {
				return (rtCeFatalError
					(func_name, pr_val[0], rtUINT16,
					 rtSUCCESS));
			}
		}

		break;

/*
 *******************************************************************************
 *     CASE  h_AcquVti (VALIDCHANGE)   ==   GLOBAL FAULT
 *******************************************************************************
*/
/*  Previous status is operational, new status is fault			      */
	case GOF_D_BDR_VALIDCHANGE_FAULT_GBL:
		{
			if (h_Valid == GOF_D_BDR_VALID_VALID) {
				h_Valid_Out = GOF_D_BDR_VALID_FAULT;

				/* Write the current validity into Database */
				if (rtCeWriteOutput
				    (r_Valid,
				     (rtUInt8 *) & h_Valid_Out) == rtFAILED) {
					return (rtCeFatalError
						(func_name, pr_val[0], rtUINT16,
						 rtSUCCESS));
				}
			}

/*	Note : VALIDACQ is not updated and no lines are added into the fault 
	invalid information list					      */

		}
		break;

	case GOF_D_BDR_VALIDCHANGE_VIDE:
		{
		}
		break;

	default:
		{
		}
		break;

	}			/* end switch */

	if (h_Validchange != GOF_D_BDR_VALIDCHANGE_VIDE)

		/* Trigger the CE calculation after the writing of VALID attribute  */
		if (rtCeTrigger(r_Valid.plin, r_Valid.ain, GOF_D_FALSE) ==
		    rtFAILED) {
			return (rtCeFatalError
				(func_name, pr_val[0], rtUINT16, rtSUCCESS));
		}
#ifdef TRACE
	printf("Fin VALAL Valid = %d\n", h_Valid_Out);
#endif

/*
 *******************************************************************************
 *     END OF PROCESSING OF THE STATE OF SA 
 *******************************************************************************
*/

	/* Update Transition (VALIDCHANGE) with a NULL value             */
	/* for the return of the function                                */
	*(rtUInt16 *) pr_val[0]->operand.data = GOF_D_BDR_VALIDCHANGE_VIDE;

	return (rtSUCCESS);

}
//////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  int res;
  rtUInt16 validchange;
  rtUInt16 validacq;
  rtUInt16 sastate;
  rtUInt16 old_sastate;
  rtUInt16 inhiblocal;
  rtUInt16 inhib;


  std::cout << " VALIDCHANGE VALACQ DATEHOURM VALIDACQ VAL VALID SASTATE OLDSASTATE INHIBLOCAL INHIB"<<std::endl;
  // Цикл по всем значениям Достоверности VALIDACQ
  for(validacq = GOF_D_BDR_VALID_FAULT;
      validacq <= GOF_D_BDR_VALID_GLOBAL_FAULT;
      validacq++)
  {
    // Перед новой итерацией выставить первоначальные значения
    set_init_val();

    *(rtUInt16*) g_val[VALIDACQ_IDX].operand.data = validacq;

    for(sastate = GOF_D_BDR_SASTATE_FAULT;
        sastate <= GOF_D_BDR_SASTATE_OP;
        sastate++)
    {
      *(rtUInt16*) g_val[SASTATE_IDX].operand.data = sastate;

      for(old_sastate = GOF_D_BDR_SASTATE_FAULT;
          old_sastate <= GOF_D_BDR_SASTATE_OP;
          old_sastate++)
      {
        *(rtUInt16*) g_val[OLDSASTATE_IDX].operand.data = old_sastate;

        for(inhiblocal = 0;
            inhiblocal <= 1;
            inhiblocal++)
        {
          *(rtUInt16*) g_val[INHIBLOCAL_IDX].operand.data = inhiblocal;

          for(inhib = 0;
              inhib <= 1;
              inhib++)
          {
            *(rtUInt16*) g_val[INHIB_IDX].operand.data = inhib;
            std::cout << "VALIDACQ=" << validacq
                      << " SASTATE=" << sastate
                      << " OLDSASTATE=" << old_sastate
                      << " INHIBLOCAL=" << inhiblocal
                      << " INHIB=" << inhib
                      <<std::endl;

                    // Цикл по всем значениям VALIDCHANGE
                    for (validchange = GOF_D_BDR_VALIDCHANGE_FAULT;
                         validchange <= GOF_D_BDR_VALIDCHANGE_END_INHIB_SA;
                         g_stack_idx = 0, validchange++)
                    {
                        // выставить первоначальные условия в зависимости от значения VALIDCHANGE
                        set_next_val(validchange);

                        // TODO: Запомнить текущее состояние
                        push();
                        res = sil_trt_cal_ValAl(10);
                        // TODO: Восстановить состояние
                        pop();
                #if 0
                        std::cout << "\tVALIDCHANGE="<<validchange<<", GOFVALAL="<<res<<std::endl;
                #else
                        if (res)
                          std::cout << "GOFVALAL=" << res << std::endl;
                #endif
                    }
          } // INHIB
        } // INHIBLOCAL
      } // OLDSASTATE
    } // SASTATE
  } // VALIDACQ

  return res;
}

