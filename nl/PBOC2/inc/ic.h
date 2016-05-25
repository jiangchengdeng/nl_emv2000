#ifndef _IC_H_
#define _IC_H_

typedef enum
{
	ICPORT1,
	ICPORT2
}TICPort;

typedef enum
{
	SELECTMODE_FIRST,
	SELECTMODE_NEXT
}TSelectMode;

typedef enum
{
	VERIFYMODE_NORMAL,
	VERIFYMODE_ENCRYPT
}TVerifyMode;

typedef enum
{
	GETDATAMODE_ATC,
	GETDATAMODE_ONLINEATC,
	GETDATAMODE_PASSWORDRETRY,
	GETDATAMODE_LOGFORMAT
}TGetDataMode;

typedef enum
{
	ACTYPE_AAC,
	ACTYPE_ARQC,
	ACTYPE_TC,
	ACTYPE_AAR,
	ACTYPE_RFU
}TACType;

#define		IC1_EXIST			0x01
#define		IC1_POWER			0x02
#define		IC2_EXIST			0x04
#define		IC2_POWER			0x08

#define		ICERR_NONE			0
#define		ICERR_BASE			(-2000)
#define		ICERR_ISDRAW		(ICERR_BASE-1)		//ICø®±ª∞Œ≥ˆ
#define		ICERR_NOICCARD		(ICERR_BASE-2)		//ICø®Œ¥≤Â»Î
#define		ICERR_POWERUP		(ICERR_BASE-3)		//ICø®…œµÁ ß∞‹
#define		ICERR_POWERDOWN		(ICERR_BASE-4)		//ICø®œ¬µÁ ß∞‹
#define		ICERR_NORMALPROCESS			(ICERR_BASE-11)		//
#define		ICERR_NOINFORMATION			(ICERR_BASE-12)		//
#define		ICERR_ERRORRESPONSEDATA		(ICERR_BASE-13)		//
#define		ICERR_INVALIDFILELEN		(ICERR_BASE-14)		//
#define		ICERR_INVALIDSELECTFILE		(ICERR_BASE-15)		//
#define		ICERR_INVALIDFCI			(ICERR_BASE-16)		//
#define		ICERR_AUTHFAIL				(ICERR_BASE-17)		//
#define		ICERR_VERIFYFAIL			(ICERR_BASE-18)		//
#define		ICERR_STATEFLAGNOTCHANGED	(ICERR_BASE-19)		//
#define		ICERR_MEMORYERROR			(ICERR_BASE-20)		//
#define		ICERR_LENERROR				(ICERR_BASE-21)		//
#define		ICERR_UNSUPPORTSAFEMESSAGE	(ICERR_BASE-22)		//
#define		ICERR_CANTPROCESS			(ICERR_BASE-23)		//
#define		ICERR_CMDNOTACCEPT			(ICERR_BASE-24)		//
#define		ICERR_UNCOMPATIBLEFILESTRU	(ICERR_BASE-25)		//
#define		ICERR_SAFESTATEDISSATISFY	(ICERR_BASE-26)		//
#define		ICERR_VERIFYBLOCKED			(ICERR_BASE-27)		//
#define		ICERR_INVALIDREFERENCE		(ICERR_BASE-28)		//
#define		ICERR_DISSATISFYCONDITION	(ICERR_BASE-29)		//
#define		ICERR_NOTCURRENTEF			(ICERR_BASE-30)		//
#define		ICERR_MISSSAFEMESSAGE		(ICERR_BASE-31)		//
#define		ICERR_INVALIDSAFEMESSAGE	(ICERR_BASE-32)		//
#define		ICERR_INVALIDPARA			(ICERR_BASE-33)		//
#define		ICERR_FUNCTIONNOTSUPPORT	(ICERR_BASE-34)		//
#define		ICERR_FILENOTFOUND			(ICERR_BASE-35)		//
#define		ICERR_RECORDNOTFOUND		(ICERR_BASE-36)		//
#define		ICERR_NOTENOUGHSPACE		(ICERR_BASE-37)		//
#define		ICERR_INVALIDP1P2			(ICERR_BASE-38)		//
#define		ICERR_REFERDATANOTFOUND		(ICERR_BASE-39)		//
#define		ICERR_INVALIDLEN			(ICERR_BASE-40)		//
#define		ICERR_INVALIDDATA			(ICERR_BASE-41)		//
#define		ICERR_NOTENOUGHMONEY		(ICERR_BASE-42)		//
#define		ICERR_INVALIDMAC			(ICERR_BASE-43)		//
#define		ICERR_APPBLOCKEDPERMANENTLY	(ICERR_BASE-44)		//
#define		ICERR_TRANDCOUNTERMAX		(ICERR_BASE-45)		//
#define		ICERR_KEYINDEXNOTSUPPORT	(ICERR_BASE-46)		//
#define		ICERR_MACCANTUSE			(ICERR_BASE-47)		//
#define		ICERR_INVALIDCLA			(ICERR_BASE-48)		//
#define		ICERR_CODENOTSUPPORT		(ICERR_BASE-49)		//
#define		ICERR_RECEIVETIMEOUT		(ICERR_BASE-50)		//
#define		ICERR_PARITYCHECKERROR		(ICERR_BASE-51)		//
#define		ICERR_CHECKSUMERROR			(ICERR_BASE-52)		//
#define		ICERR_NOFCI					(ICERR_BASE-53)		//
#define		ICERR_NOSFORKF				(ICERR_BASE-54)		//
#define		ICERR_UNKNOW		(ICERR_BASE-100)

extern void IC_SetPort(TICPort Port);
extern int IC_IsCardExist(void);
extern int IC_IsCardPowerUped(void);
extern int IC_PowerUp(void);
extern int IC_PowerDown(void);
extern int IC_ReadRecord(char SFI, char RecordNo, char *oTLVStr);
extern int IC_SelectByAID(const char *AID, int AIDLen, TSelectMode SM, char *oTLVStr);
extern int IC_SelectPSE(char *oSFI);
extern int IC_SelectDDF(char *AID, int AIDLen, char *oSFI);
extern int IC_SelectADF(char *AID, int AIDLen, TSelectMode SM, char *PDOL, int *PDOLLen);
extern int IC_GetChallenge(char *pOut);
extern int IC_ExternalAuthenticate(char *iData, char DataLen);
extern int IC_InternalAuthenticate(char *iData, char iDataLen, char *oData);
extern int IC_Verify(char *iData, char iDataLen, TVerifyMode VM);
extern int IC_GetProcessOptions(char *iData, char iDataLen, char *oData);
extern int IC_GetData(TGetDataMode GDM, char *oData);
extern int IC_GenerateAC(TACType ACType, int iNeedCDA, char *iData, int iDateLen, char *oData);
extern void IC_GetLastSW(char *SW1, char *SW2);
extern int IC_GetPort(void) ;
extern int Get_response(char len , char *response) ;
#endif
