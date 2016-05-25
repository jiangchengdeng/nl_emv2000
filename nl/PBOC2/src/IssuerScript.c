#include <string.h>
#include "posapi.h"
#include "posdef.h"
#include "tvrtsi.h"
#include "ic.h"
#include "AppData.h"
#include "tools.h"

#define MAX_CMD 6	//PBOC借记/贷记规范支持的脚本命令,6个


typedef struct
{
	char * pCmd ;
	int iCmdLen ;
}ScriptCmd ;

static ScriptCmd _ScriptCmd[MAX_CMD] ;
char  _ISResult[5 * MAX_SCRIPT]  ;
char ScriptNum ;

int ParseScript(char * buff, int len, char * ISID)
{
	int  tag86len = 0, i;
	int k = 0 ;
	char * p = buff ;
	
	memset(_ScriptCmd, 0, sizeof(_ScriptCmd));
	
	//脚本标识
	if (*buff  == 0x9f)
	{
		if (*(buff+1) != 0x18)
			return FAIL ;
		memcpy(ISID, buff + 3 , 4)  ;
		p += 7 ;  //指向86串的开始
		tag86len = len - 7 ; //86串的长度
	}
	else
	{
		tag86len =len ; //86串的长度
	}

	if (len > 128)
	{
		ERR_MSG("脚本长度超过支持的长度") ;
		return FAIL ;
	}
	
	//解析86命令
	i = 0 ;
	k = 0 ; 
	while ( i < tag86len )
	{
		if (*(p + i) != 0x86)
		{
			return FAIL ; //解析失败
		}
		if (*(p + i + 1) < 4)	//APDU最少4个字节
		{
			return FAIL ;
		}
		switch (p[i + 3])
		{
		case 0x1E:
		case 0x18:
		case 0x16:
		case 0x24:
			if (p[i + 2] == 0x84)
				break ;
			else
				return FAIL ; //规范不支持的脚本命令，不能让其执行，解析失败
		case 0xDA:
		case 0xDC:	
			if (p[i + 2] == 0x04)
				break ;
			else
				return FAIL ;
		}
		_ScriptCmd[k].pCmd = p + i + 2 ;
		_ScriptCmd[k].iCmdLen = p[i + 1] ;
		i += (p[i + 1] + 2) ;
		k++;
	}

	if (i != tag86len)
	{
		return FAIL ; //长度解析错误
	}
	
	return SUCC;
}

//int iTime 执行时间 TRUE--第二次Generate AC之后执行
//					 FALSE--第二次Generate AC之前执行
int  _IssuerScript(char * pData , int len, char * ISResult)
{
	int ret ;
	int port = IC_GetPort() ;
	int retlen ;
	char resp[256] ;
	int i;

	SetTSI(SCRIPT_PROC_COMPLETION, 1) ;

	if (ParseScript(pData, len, ISResult + 1) == FAIL)
	{
		ERR_MSG("  脚本解析错误");
		return FAIL;
	}

	for (i = 0 ; i <= MAX_CMD; i ++) 
	{
		if(_ScriptCmd[i].iCmdLen == 0)
		{
			break;
		}	
		memset(resp, 0x00, sizeof(resp)) ;
		ret = iccrw_new(port , _ScriptCmd[i].iCmdLen, 
			_ScriptCmd[i].pCmd, &retlen, resp ) ;
		
/*		clrscr() ;
		printf("执行结果= %d\n", ret) ;
		printf("SW:%02x%02x", resp[retlen-2] , resp[ret-1]) ;
		getkeycode(0) ;
*/
		if( (ret == 0) && (resp[retlen -2] == 0x90 ||resp[retlen -2] == 0x62 ||
			resp[retlen -2] == 0x63))
		{
				continue ; //继续下一条命令
		}
		ISResult[0] = (i+1) > 15 ? 15 : i+1;
		ISResult[0] |= 0x10 ;
		return FAIL;
	}
	ISResult[0]  = 0x20 ;
	return SUCC ;
}

void IssuerScript(int iTime)
{
	char * pData ;
	int len ;
	int ret ;
	char buff[2] ;
	
	if(iTime)	//第二次Generate AC之后	
		buff[0] = 0x7F ;
	else //第二次Generate AC之前
		buff[0] = 0x3F ;
	buff[1] = 0x01 ;

	while((pData = GetAppData(buff, &len)) != NULL)
	{
		ret = _IssuerScript(pData, len, _ISResult + ScriptNum * 5) ;
		if (ret == FAIL)
		{
			if (iTime)
				SetTVR(SCRIPT_FAIL_AFTER_LAST_GEN_AC, 1) ;
			else
				SetTVR(SCRIPT_FAIL_BEFORE_LAST_GEN_AC, 1) ;
		}
		ScriptNum ++ ;
		buff[1] ++ ;
	}

	
}
