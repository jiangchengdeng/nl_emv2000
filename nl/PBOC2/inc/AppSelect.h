#ifndef _APPSELECT_H_
#define _APPSELECT_H_

#define TermAIDFile		"termaid.dat"
#define MAXAIDLISTNUM	10

typedef struct
{
	char	Enable;
	char	AID[16];			//AID(应用标识符)
	char	AIDLen;				//AID长度
	char	AppLabel[16];		//应用标签
	char	AppLabelLen;		//应用标签长度
	char	AppPreferedName[16];	//应用优先名称
	char	AppPreferedNameLen;	//应用优先名称长度
	char	AppPriorityID;	//应用优先权标识符
}TAIDList;
#define  AIDLISTLEN	53

//#define USEAIDLIST -2000
extern int CreateAppList(char AllowPartialAID, TAIDList oAIDList[], int *oAIDNum) ;
extern int GetTermAidList(void);

#endif
