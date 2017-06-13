#include <iostream>

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#define OK                  100
#define NOK                 101

#define ECH_D_STYPEIDLG     5
#define ECH_D_ELEMIDLG		5
#define ECH_D_COMPIDLG      5
#define ECH_D_COMPELEMNB    20
#define ECH_D_IEDPERICD     ECH_D_COMPELEMNB
#define GOF_D_FILE_MAXLINE  100

#define ESG_ESG_D_LOGGEDTEXTLENGTH      149

#define ESG_INE_CNF_D_MAX_LINELG		80
#define ESG_INE_CNF_D_LG_CHAR   		1
#define ESG_INE_CNF_D_LG_INT    		5
#define ESG_INE_CNF_D_LG_REAL    		20

#define ESG_ESG_ODM_D_UPEXCHCOMPSUB     1
#define ESG_ESG_D_EMPTYFIELD            0
#define ESG_INE_CNF_D_SEP_ENDOFLINE		"\n"
#define ESG_INE_CNF_D_SEP_PREFDATA		"="
#define ESG_INE_CNF_D_SEP_DATADATA		","
#define ESG_INE_CNF_D_POINTTCHAR		'.'

#define ESG_INE_CNF_D_PREF_STRUNAME		"NAME"
#define ESG_INE_CNF_D_PREF_STRUFIELD	"FIELD"
#define ESG_INE_CNF_D_PREF_STRUICD		"ICD"
#define ESG_INE_CNF_D_PREF_STRULENGTH	"LENGTH"
#define ESG_INE_CNF_D_PREF_RECORDBEGIN	"BEGIN"
#define ESG_INE_CNF_D_PREF_RECORDEND	"END"
#define ESG_INE_CNF_D_PREF_VERSIONNUM	"VERSION"

/*----------------------------------------------------------------------------*/
typedef char gof_t_AttName[32];
typedef char ech_typ_t_SubTypeId [ECH_D_STYPEIDLG + 1];

typedef struct {
  int             i_AttType;
  int             i_AttLG;
  gof_t_AttName   s_AttName;
} ech_typ_t_AttElem;

typedef struct {
  ech_typ_t_SubTypeId     s_SubTypeId;
  int             i_AttNumber;
  ech_typ_t_AttElem       ar_AttElem [ECH_D_COMPELEMNB];
} ech_typ_t_SubTypeElem;

typedef struct {
  char  s_ExchCompId[ECH_D_COMPIDLG + 1];
  int   i_NbData;
  char  as_ElemDataArr[ECH_D_IEDPERICD][ECH_D_ELEMIDLG + 1];
  char  s_AssocSubType[ECH_D_STYPEIDLG + 1];
} esg_esg_odm_t_ExchCompElem;

/*----------------------------------------------------------------------------*/
int esg_esg_odm_UpdateExchCompArr(const char* s_IExchCompId,
	const int i_IUpdateType,
	const esg_esg_odm_t_ExchCompElem *pr_IExchCompElem)
{
  std::cout << "esg_esg_odm_UpdateExchCompArr(" << s_IExchCompId << ") " << pr_IExchCompElem->s_AssocSubType << std::endl;
  /*for (int i = 0; i<pr_IExchCompElem->i_NbData; i++) {
    std::cout << "\t" << pr_IExchCompElem->as_ElemDataArr << ", " << pr_IExchCompElem->s_AssocSubType;
  }
  std::cout << std::endl;*/
}

/*----------------------------------------------------------------------------*/
int ech_typ_ReserveSubTypeArr()
{
  return 0;
}

/*----------------------------------------------------------------------------*/
void esg_ine_cnf_CompactLine (char* s_Line)
{
	char    *p;		/* work variable	      */
	char    *s_dummy;	/* work variable	      */

	s_dummy = s_Line ;

	p = strchr(s_Line, '#') ;
	if ( p != NULL)
	{
		*p = 0 ;
	}
	p = s_Line ;

	while (*p != 0)
	{
		if (isspace(*p) == 0)
		{
			*s_Line = *p ;
			s_Line++ ;
		}

		p++ ;
	}

	*s_Line = 0;
}/*-END esg_ine_cnf_CompactLine ----------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*  FUNCTION			esg_ine_cnf_SubTypesRead 	  	      */
/*  FULL MAME			Read Exchanged Sub-types Configuration Data   */
/*----------------------------------------------------------------------------*/
/*  ROLE 								      */
/*	The purpose of this routine is to read and store the sub-types        */
/*	configuration data used by the ESDx CSCI.    		              */
/*	Only the number of stored elements is returned if it is given as null.*/
/*	Else the pointed array of sub-types data is updated.    	      */
/*									      */
/*----------------------------------------------------------------------------*/
/*  RETURNED EXCEPTIONS							      */
/*	report	incorrect_open						      */
/*	report	incorrect_reading					      */
/*									      */
/*----------------------------------------------------------------------------*/
/*  NOMINAL PROCESSING							      */
/*	- Open the sub-types configuration data file       	              */
/*	- If open ok							      */
/*	        read version number of the file 		              */
/*	        read each record of the file (1 record = 1 sub-type)          */
/*	- If open or reading not ok	 				      */
/*	        return error   						      */
/*									      */
/*----------------------------------------------------------------------------*/
/*  ERRORS AND EXCEPTIONS PROCESSING					      */
/*	If the sub-types configuration file cannot be open, 		      */
/*      the routine stores this error in the system "error log file" and      */
/*      stops.							    	      */
/*	If the sub-types configuration file cannot be read, 		      */
/*      the routine stores this error in the system "error log file" and      */
/*      continues.						   	      */
/*									      */
/*----------------------------------------------------------------------------*/

/*-------------------*/

/* Returned VALUE    */

/*-------------------*/
int esg_ine_cnf_SubTypesRead(
/*------------------*/
/* Input Parameters */
/*------------------*/
                              ech_typ_t_SubTypeElem * pr_ISubTypeArr,

/* array of fields read :	              */
/* updated if NbField is not null in input    */

/*-------------------*/
/* In-out Parameters */
/*-------------------*/
                              int& i_IONbFieldRead
/* number of fields read :		      */
/* updated if given null in input	      */
    )
{

  /*----------------------------------------------------------------------------*/

  static const char *s_FctName = "esg_ine_cnf_SubTypesRead";

  char s_Trace[150 + 1];        /* printed trace            */
  char s_Trace1[350 + 1];       /* printed trace           */

  int i_RetStatus;              /* routine report             */
  int i_Status;                 /* called routine report      */
  char *s_EnvVar;               /* environment variable       */
  char s_FileDir[GOF_D_FILE_MAXLINE + 1];

  /* internal config. file dir. */
  char s_FileName[GOF_D_FILE_MAXLINE + 1];

  /* global config. file name   */
  FILE *pi_fd;                  /* config. file descriptor    */
  char s_ReadLine[ESG_INE_CNF_D_MAX_LINELG + 1];

  /* line read from file        */
  int i_UsedSubTypeNb;      /* work variable             */
  char *pc_p;                   /* work variable              */
  bool b_EndRecord;             /* flag for end of record     */
  char s_SubTypeName[ECH_D_COMPIDLG + 1];

  /* structure name             */
  char s_SubTypeICD[ECH_D_COMPIDLG + 1];

  /* associated ICD name        */
  char s_VersionNum[30];

  /* version number             */
  char as_AttType[ECH_D_COMPELEMNB][ESG_INE_CNF_D_LG_INT + 1];

  /* attributes list types      */
  gof_t_AttName as_AttName[ECH_D_COMPELEMNB];

  /* array of attributes names for one sub-type         */
  esg_esg_odm_t_ExchCompElem r_SubTypeEntry;

  /* struct. entry (oper. data) */
  ech_typ_t_SubTypeElem r_SubTypeElem;  /* sub-type element           */
  int i_NbAtts;             /* associated types nb        */
  int i_i;                  /* loop counter               */
  bool b_NameFound;             /* flag for name found        */
  bool b_FieldFound;            /* flag for at least one field found       */
  bool b_ICDFound;              /* flag for ICD found         */
  char s_LoggedText[ESG_ESG_D_LOGGEDTEXTLENGTH + 1];

  char as_AttLength[ECH_D_COMPELEMNB][ESG_INE_CNF_D_LG_INT + 1];

  /* attributes list length */
  /* array of indicators of length field found for each attribute */
  bool ab_LengthFound[ECH_D_COMPELEMNB];

  int i_IxAtt;              /* current attribute index */

  /*............................................................................ */

  i_RetStatus = OK;
  i_UsedSubTypeNb = 0;
  i_IxAtt = 0;
  i_NbAtts = 0;
  b_EndRecord = false;
  b_NameFound = false;
  b_FieldFound = false;
  b_ICDFound = false;

  /* (CD)   Check pointer parameter as null                                     */
  /*----------------------------------------------------------------------------*/
  if ((pr_ISubTypeArr == NULL) && (i_IONbFieldRead > 0))
  {
    i_RetStatus = NOK;
    std::cout << s_FctName << ": ESG_ESG_D_ERR_UNEXPARAMVAL" << std::endl;
  }
  else
  {
    /* (CD)   Open the exchanged structures configuration data file               */
    /*----------------------------------------------------------------------------*/
    i_UsedSubTypeNb = 0;

    strcpy(s_FileName, "dat/EGSA/ESG_LOCSTRUCTS.DAT");

    if (i_RetStatus == OK)
    {
      pi_fd = fopen(s_FileName, "r");

      /* (CD)   If open is not correct                                              */
      /* (CD)       Store error into log file                                       */
      /*----------------------------------------------------------------------------*/
      if (pi_fd == NULL)
      {
        i_RetStatus = NOK;
        std::cout << s_FctName << ": ESG_ESG_D_ERR_CANNOTOPENFILE: " << s_FileName;
      }
      else
      {
        /* (CD)         While not end of file                                         */
        /* (CD)           Read one line of the file                                   */
        /* (CD)               If reading is correct                                   */
        /*----------------------------------------------------------------------------*/
        while (fgets(s_ReadLine, ESG_INE_CNF_D_MAX_LINELG, pi_fd) != NULL)
        {
          /* (CD)                   cut blank characters (esg_ine_cnf_CompactLine)      */
          /*----------------------------------------------------------------------------*/
          esg_ine_cnf_CompactLine(s_ReadLine);

          if (s_ReadLine[0])
          {
            /* (CD)                   If line corresponds to the version number           */
            /*----------------------------------------------------------------------------*/
            if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_VERSIONNUM)) != NULL)
            {
              /* read version number */
              /* ------------------- */
              s_VersionNum[0] = '\0';
              pc_p = strtok(s_ReadLine, ESG_INE_CNF_D_SEP_PREFDATA);
              pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_ENDOFLINE);
              if (pc_p != NULL)
              {
                strcpy(s_VersionNum, pc_p);
              }
            } /* end if pc_p = strstr != NULL */
            else if (strcmp(s_ReadLine, ESG_INE_CNF_D_PREF_RECORDBEGIN) == 0)
            {
              /* (CD)                       Read the rest of the record                   */
              /*--------------------------------------------------------------------------*/
              i_NbAtts = 0;
              b_NameFound = false;
              b_ICDFound = false;
              b_FieldFound = false;
              for (i_i = 0; i_i < ECH_D_COMPELEMNB; i_i++)
              {
                ab_LengthFound[i_i] = false;
              }

              b_EndRecord = false;

              while (b_EndRecord == false)
              {
                if (fgets(s_ReadLine, ESG_INE_CNF_D_MAX_LINELG, pi_fd) != NULL)
                {
                  esg_ine_cnf_CompactLine(s_ReadLine);

                  /* structure name */
                  /* -------------- */
                  if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_STRUNAME)) != NULL)
                  {
                    s_SubTypeName[0] = '\0';
                    pc_p = strtok(s_ReadLine, ESG_INE_CNF_D_SEP_PREFDATA);
                    pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_ENDOFLINE);
                    if (pc_p != NULL)
                    {
                      strcpy(s_SubTypeName, pc_p);
                    }

                    if ((0 < strlen(s_SubTypeName)) && (strlen(s_SubTypeName) <= ECH_D_COMPIDLG))
                    {
                      b_NameFound = true;
                    }
                  }

                  /* composed structure associated */
                  /* ----------------------------- */
                  if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_STRUICD)) != NULL)
                  {
                    s_SubTypeICD[0] = '\0';
                    pc_p = strtok(s_ReadLine, ESG_INE_CNF_D_SEP_PREFDATA);
                    pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_ENDOFLINE);
                    if (pc_p != NULL)
                    {
                      strcpy(s_SubTypeICD, pc_p);
                    }

                    if ((strlen(s_SubTypeICD) > 0) && (strlen(s_SubTypeICD) <= ECH_D_COMPIDLG))
                    {
                      b_ICDFound = true;
                    }
                  }

                  /* structure field */
                  /* --------------- */
                  if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_STRUFIELD)) != NULL)
                  {
                    if (i_NbAtts < ECH_D_COMPELEMNB)
                    {
                      /* att. name */
                      /* --------- */
                      as_AttName[i_NbAtts][0] = '\0';
                      pc_p = strtok(s_ReadLine, ESG_INE_CNF_D_SEP_PREFDATA);
                      pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_DATADATA);
                      if (pc_p != NULL)
                      {
                        strcpy(as_AttName[i_NbAtts], pc_p);
                      }

                      /* att. type */
                      /* --------- */
                      as_AttType[i_NbAtts][0] = '\0';
                      pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_ENDOFLINE);
                      if (pc_p != NULL)
                      {
                        strcpy(as_AttType[i_NbAtts], pc_p);
                      }
                      else
                      {
                        /* we consider that att.name was not there, so att. type was read instead of att. name    */
                        strcpy(as_AttType[i_NbAtts], as_AttName[i_NbAtts]);
                        as_AttName[i_NbAtts][0] = '\0';
                      }

                      if ((strlen(as_AttType[i_NbAtts]) > 0) && (strlen(as_AttType[i_NbAtts]) <= ESG_INE_CNF_D_LG_INT))
                      {
                        b_FieldFound = true;
                      }

                      /* check type as digits */
                      for (i_i = 0; i_i < strlen(as_AttType[i_NbAtts]); i_i++)
                      {
                        if (isdigit((int) as_AttType[i_NbAtts][i_i]) == 0)
                        {
                          /* log error */
                          i_RetStatus = NOK;
                          std::cout << s_FctName << ": ESG_ESG_D_ERR_BADCONFFILE: Non digital field " << as_AttType[i_NbAtts] << std::endl;

                          sprintf(as_AttType[i_NbAtts], "%d", ESG_ESG_D_EMPTYFIELD);

                          b_FieldFound = false;
                        }
                      }

                      if (b_FieldFound == true)
                      {
                        i_NbAtts = i_NbAtts + 1;
                      }
                    }           /* end if i_NbAtts < ECH_D_COMPELEMNB */
                  }             /* end if */

                  /* (CD) treatment of the optional attribute length field */
                  /* (CD) This field is used for string attributes */
                  /* (CD) Warning : the index of the length fields array is NbAtts-1 as the */
                  /* (CD) NbAtts value has been increased precedently for the type field */

                  /* length field */
                  /* --------------- */
                  if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_STRULENGTH)) != NULL)
                  {
                    if (i_NbAtts > 0)
                    {
                      i_IxAtt = i_NbAtts - 1;
                      as_AttLength[i_IxAtt][0] = '\0';
                      pc_p = strtok(s_ReadLine, ESG_INE_CNF_D_SEP_PREFDATA);
                      pc_p = strtok(NULL, ESG_INE_CNF_D_SEP_ENDOFLINE);
                      if (pc_p != NULL)
                      {
                        strcpy(as_AttLength[i_IxAtt], pc_p);
                      }

                      if ((strlen(as_AttLength[i_IxAtt]) > 0) && (strlen(as_AttLength[i_IxAtt]) <= ESG_INE_CNF_D_LG_INT))
                      {
                        ab_LengthFound[i_IxAtt] = true;
                      }

                      /* check type as digits */
                      for (i_i = 0; i_i < strlen(as_AttLength[i_IxAtt]); i_i++)
                      {
                        if (isdigit((int) as_AttLength[i_IxAtt][i_i]) == 0)
                        {
                          i_RetStatus = NOK;
                          std::cout << s_FctName << ": ESG_ESG_D_ERR_BADCONFFILE: Non digital field " << as_AttLength[i_IxAtt] << std::endl;

                          sprintf(as_AttLength[i_IxAtt], "%d", ESG_ESG_D_EMPTYFIELD);

                          ab_LengthFound[i_IxAtt] = false;
                        }
                      }
                    }
                  }

                  /* end of record */
                  /* ------------- */
                  if ((pc_p = strstr(s_ReadLine, ESG_INE_CNF_D_PREF_RECORDEND)) != NULL)
                  {
                    b_EndRecord = true;
                  }

                  /* (CD)                       Store into global variables data read           */
                  /*----------------------------------------------------------------------------*/
                  if ((b_EndRecord == true) && (b_NameFound == true) && (b_FieldFound == true))
                  {
                    /* decode read values */
                    /* ------------------ */
                    memset(&r_SubTypeEntry, 0, sizeof(r_SubTypeEntry));
                    memset(&r_SubTypeElem, 0, sizeof(r_SubTypeElem));

                    /* name */
                    strcpy(r_SubTypeEntry.s_AssocSubType, s_SubTypeName);
                    strcpy(r_SubTypeElem.s_SubTypeId, s_SubTypeName);

                    /* ICD */
                    if (b_ICDFound)
                    {
                      strcpy(r_SubTypeEntry.s_ExchCompId, s_SubTypeICD);
                    }

                    /* fields */
                    for (i_i = 0; i_i < i_NbAtts; i_i++)
                    {
                      strcpy(r_SubTypeElem.ar_AttElem[i_i].s_AttName, as_AttName[i_i]);
                      sscanf(as_AttType[i_i], "%d", &(r_SubTypeElem.ar_AttElem[i_i].i_AttType));
                      if (ab_LengthFound[i_i] == true)
                      {
                        sscanf(as_AttLength[i_i], "%d", &(r_SubTypeElem.ar_AttElem[i_i].i_AttLG));
                      }
                      else
                      {
                        r_SubTypeElem.ar_AttElem[i_i].i_AttLG = 0;
                      }
                    }
                    r_SubTypeElem.i_AttNumber = i_NbAtts;

                    s_Trace[0] = '\0';
                    std::cout << "SubTyp=" << s_SubTypeName << " ICD=" << s_SubTypeICD << " NbFields=" << r_SubTypeElem.i_AttNumber << std::endl;
                    for (i_i = 0; i_i < i_NbAtts; i_i++)
                    {
                      if (ab_LengthFound[i_i] == true)
                      {
                        sprintf(s_Trace1,
                                " (index:%d type:%d name:%s len:%d)",
                                i_i,
                                r_SubTypeElem.ar_AttElem[i_i].i_AttType,
                                r_SubTypeElem.ar_AttElem[i_i].s_AttName,
                                r_SubTypeElem.ar_AttElem[i_i].i_AttLG);
                        strcat(s_Trace, s_Trace1);
                      }

                      if (s_Trace[0])
                        std::cout << s_FctName << ": " << s_Trace << std::endl;
                    }

                    /* if ok, store into global variables ;  */
                    /* ------------------------------------- */
                    if (i_IONbFieldRead > 0)
                    {
                      memcpy((void *) (pr_ISubTypeArr + i_UsedSubTypeNb), &r_SubTypeElem, sizeof(r_SubTypeElem));
                    }

                    i_UsedSubTypeNb = i_UsedSubTypeNb + 1;

                    if (b_ICDFound)
                    {
                      i_Status = esg_esg_odm_UpdateExchCompArr(r_SubTypeEntry.s_ExchCompId,
                                                        ESG_ESG_ODM_D_UPEXCHCOMPSUB,
                                                        &r_SubTypeEntry);
                    }
                  }

                  /* check if record is correct */
                  /* -------------------------- */
                  if ((b_EndRecord == true) && ((b_NameFound != true) || (b_FieldFound != true)))
                  {
                    i_RetStatus = NOK;
                    std::cout << s_FctName << ": ESG_ESG_D_ERR_BADCONFFILE: Mandatory field not found" << std::endl;
                  }
                }
                else
                {
                  /* End of file entcountered */
                  /* ------------------------ */
                  b_EndRecord = true;

                  /* log error */
                  i_RetStatus = NOK;
                  std::cout << s_FctName << ": ESG_ESG_D_ERR_BADCONFFILE: " << s_FileName << std::endl;

                }               /* end else fgets != NULL */

              }                 /* end while b_EndRecord == false */

            }

          }                     /* end if strcmp(s_ReadLine, ESG_ESG_D_EMPTYSTRING) != 0 */

        }                       /* end while fgets */

        /* (CD)       Close exchanged structures configuration file                   */
        /*----------------------------------------------------------------------------*/
        fclose(pi_fd);

        /* (CD)       Check if at least one structure has been read                   */
        /*----------------------------------------------------------------------------*/
        if (i_UsedSubTypeNb <= 0)
        {
          i_RetStatus = NOK;
          std::cout << s_FctName << ": ESG_ESG_D_ERR_BADCONFFILE: Nb sub-type found = " << i_UsedSubTypeNb << std::endl;
        }
        else
        {

          if (i_IONbFieldRead <= 0)
          {
            i_IONbFieldRead = i_UsedSubTypeNb;
          }                     /* end if */
          else if (i_RetStatus == OK)
          {
            std::cout << "ech_typ_ReserveSubTypeArr()" << std::endl;

            //i_RetStatus = ech_typ_ReserveSubTypeArr (pr_ISubTypeArr, i_IONbFieldRead);
          }                     /* end else if */
        }                       /* end else i_UsedSubTypeNb <= 0 */
      }                         /* end if pi_fd  != NULL */
    }                           /* end if i_RetStatus  == OK */
  }                             /* end else i_IONbFieldRead */

  return (i_RetStatus);

}/*-END esg_ine_cnf_SubTypesRead----------------------------------------------*/

int main()
{
  ech_typ_t_SubTypeElem mass[100];
  int ssz = 0;

  esg_ine_cnf_SubTypesRead(mass, ssz);

  std::cout << std::endl << "RESULTS:" << std::endl;

  for (int i=0; i<ssz; i++) {
    std::cout << i+1 << "/" << ssz << ":" << mass[i].s_SubTypeId << ":" << mass[i].i_AttNumber << std::endl;

    for (int k=0; k<mass[i].i_AttNumber; k++) {
      std::cout << "\t" << mass[i].ar_AttElem[k].i_AttType << ":" << mass[i].ar_AttElem[k].s_AttName << std::endl;
    }
  }
}
