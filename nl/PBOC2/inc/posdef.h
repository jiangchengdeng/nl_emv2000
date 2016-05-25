#ifndef _POSDEF_H_
#define _POSDEF_H_

#include "error.h"

#define TermParaFile	"termpara.dat"
#define TermConfigFile	"termconf.dat"
#define BlackListFile	"black.dat"		//卡黑名单
#define	BlackCertFile	"blkcert.dat"	//发卡行证书黑名单

//卡黑名单结构
typedef struct
{
	char len;			//匹配长度
	char cardno[10];	//卡号
	char rfu;
}TBlackCard;

//发卡行证书黑名单
typedef struct
{
	char RID[5];		
	char index;			//公钥索引
	char serialnum[3];	//证书序列号
	char rfu[3];		
}TBlackCert;

//AID结构
typedef struct
{
	char len;			//AID长度
	char aid[16];		//应用标识
	char name[32];		//应用名称
	char priority;		//优先级
}TAID;

//终端配置，在POS下发时一次性设置，不允许修改
typedef __packed struct
{
	char TermCap[3] ;			//终端性能 '9F33'
	char TermCapAppend[5] ;		//终端附加性能 '9F40'  
	char IFDSerialNum[9] ;		//IFD序列号 '9F1E', '\0'结尾
	char TermCountryCode[2] ;	//终端国家代码 '9F1A'
	char TermID[9] ;			//终端标识 '9F1C'，'\0'结尾
	char TermType ;				//终端类型 '9F35'
	char TransCurrCode[2] ;		//交易货币 '5F2A'
	char TransCurrExp;			//交易货币指数 '5F36'
}TTermConfig ;

//终端参数，允许修改
typedef __packed struct
{
	char TAC_Default[5] ;	 	//终端行为代码-缺省 
	char TAC_Decline[5] ;	 	//终端行为代码-拒绝
	char TAC_Online[5] ;	 	//终端行为代码-联机

	char FloorLimit[4];			//终端最低限额 '9F1B'
	char TargetPercent;			//随机选择目标百分数  
	char MaxTargetPercent;		//偏置选择的最大目标百分数
	char ThresholdValue[4];		//偏置随机选择阀值
	
	char MerchantTypeCode[2] ; 	// 商户分类码 '9F15'
	char MerchantID[16] ;	   	// 商户标识 '9F16'，'\0'结尾

	char DefaultDDOL[128] ; 	//缺省DDOL
	char DefaultDDOLen ;
	char DefaultTDOL[128] ; 	//缺省TDOL
	char DefaultTDOLen ;
	char PartAIDFlag;			//部分AID选择标志(TRUE/FALSE)
	char AcqID[6];				//收单行标识 '9F01'
	char AcqLen;				//收单行标识长度

	char AppVersion[2] ;		//应用版本号 '9F09'
	char PosEntry ;				//销售点输入方式 '9F39'
	char TranRefCurrCode[2] ;	//交易参考货币代码 '9F3C'
	char TranRefCurrExp ;		//交易参考货币指数 '9F3D'

	char cAllowForceOnline ;
	char cAllowForceAccept ;
}TTermParam ;

typedef struct
{
	//标签'8E',持卡人验证方法列表
	int CVMx ;		//X金额域
	int CVMy ;		//Y金额域
	char * CVRList ; 	//持卡人验证规则列表
	char nCVRCount ;  //持卡人验证规则列表 字节数
	//应用交互特征 '82'
	char * pAIP ;
	//应用货币代码 '9F42'
	char * pAppCurrCode ;	
	//交易货币代码 '5F2A'
	char * pTransCurrCode ;	
	//持卡人证件类型 '9f62'
	char CardIDType ;
	//持卡人证件号 '9F61'
	char * CardID ;
	char CardIDLen ;
	int  iAmount ; //交易金额
	//是否现金交易(ATM0x01, 有人值守现金0x02,或返现0x04)
	char IsCash ;	
	char cvmcap ;	//CVM 性能 '9F33'
}TCVML ;

typedef struct
{ 
	char nCanOnline ;
	char nNeedCDA ;
	char * TAC_Default ;
	char * TAC_Decline ;
	char * TAC_Online ;

	char * IAC_Default ;
	char * IAC_Decline ;
	char * IAC_Online ;
}TTermActAna ;

typedef struct
{
	char * pPAN ;
	char * Lower_COL ;		//连续脱机交易下限
	char * Upper_COL ;		//连续脱机交易上限
	char * pAIP ;
	char * FloorLimt ;
	char * ThresholdValue ;
	char    TargetPercent;			
	char    MaxTargetPercent;	
}TTermRisk ;

typedef struct 
{
	char iCanOnline ;
	char iNeedCDA ;
	char iTransType ;

	char * E_PIN ;
}TTransProc ;

typedef struct
{
	char * IAC_Default ;
	char * TAC_Default ;
	char * pAutheResCode ;
	char  TransProcR ;
	char  iCDAFail ;
	char  iNeedCDA ;
	char TVR[5] ;
}TCompletion ;

#define DEBUGMODE				//调试模式

extern TTermConfig g_termconfig;		//终端配置
extern TTermParam  g_termparam;		//终端参数
extern int iNeedSignature ;

#define TSC_OFF 0
#define TSC_LEN 4
#define TERM_PARAMNUM_OFF 4 
#define TERM_PARAMNUM_LEN 1
#define USEAIDLIST -9999

#define MAX_SCRIPT 4
extern char  _ISResult[5 * MAX_SCRIPT]  ;
extern char ScriptNum ;
extern char iAcceptForced ;

#endif
