

#include "posapi.h"
#include "posdef.h"
#include <string.h>
#include "ic.h"
#include "tvrtsi.h"
#include "AppData.h"
#include "DOLProcess.h"
#include "OnlineProc.h"
#include "Completion.h"
#include "issuerscript.h"
#include "tools.h"

#define LOGFILE "proc.log"
#define LOGLONG 64

extern int AnalyzeACData(char * response, int len, int NeedCDA) ;
extern int Online(int iTransType, char * pPIN) ;

void _PreCompletion(TCompletion * pCompletion, int TransR)
{
	char * pData ;

	pData = GetAppData("\x9f\x27", NULL) ;

	pCompletion->IAC_Default = GetAppData("\x9f\x0d",NULL) ;
	pCompletion->TAC_Default = g_termparam.TAC_Default ;
	pCompletion->pAutheResCode = GetAppData("\x8A", NULL) ;
	pCompletion->iCDAFail = 0 ;
	pCompletion->TransProcR = TransR ;
	GetTVR(pCompletion->TVR) ;
	pCompletion->iNeedCDA = 0 ;

	
}

TACType TermJudge(TCompletion * pCompletion)
{
	int i ;
	int len ;
	char * p ;
	char * pAuth = GetAppData("\x8A", NULL) ;
	switch(pCompletion->TransProcR)
	{
	case ONLINE_FAIL:
		for (i = 0 ; i < 5 ; i ++)
		{
			if ((pCompletion->IAC_Default[i] & pCompletion->TVR[i]) ||
				(pCompletion->TAC_Default[i] & pCompletion->TVR[i]))
				{
					memcpy(pAuth, "\x5A\x33",2) ;
					return ACTYPE_AAC ;
				}
		}
		if (i == 5)
		{
			memcpy(pAuth, "\x59\x33",2) ;
			return ACTYPE_TC ;
		}
		break ;
	case ONLINE_SUCC :
		if( (!memcmp(pCompletion->pAutheResCode, "\x30\x30",2)) ||
			(!memcmp(pCompletion->pAutheResCode, "\x31\x30",2)) ||
			(!memcmp(pCompletion->pAutheResCode, "\x31\x31",2)))
		{
			return ACTYPE_TC ;
		}
		else if ( (!memcmp(pCompletion->pAutheResCode, "\x30\x31",2)) ||
			(!memcmp(pCompletion->pAutheResCode, "\x30\x32",2)))
		{
			while(1)
			{
				clrscr() ;
				printf("请联系你的银行:\n") ;
				p = GetAppData("\x5A", &len) ;
				printf("---应用主帐号---\n") ;
				for (i = 0 ; i < len ; i++)
					printf("%02x", p[i]) ;
				getkeycode(0) ;
				clrscr() ;
				printf("请选择:\n") ;
				printf("1.脱机批准\n") ;
				printf("2.脱机拒绝\n") ;
				i = getkeycode(0) ;
				if (i == 0x31)
				{
					return  ACTYPE_TC ;
				}
				else if(i == 0x32)
				{
					return ACTYPE_AAC ;
				}
			}
		}
		else if (!memcmp(pCompletion->pAutheResCode, "\x30\x35",2))
		{
			return ACTYPE_AAC ;
		}
		else
		{
			pCompletion->TransProcR = ONLINE_FAIL ;
			i = TermJudge(pCompletion) ;
			Online(5, NULL) ;
			pCompletion->TransProcR = ONLINE_SUCC ;
			return (TACType)i ;
		}
	case AAR_OFFLINE_APPROVE :
		memcpy(pAuth, "\x59\x32", 2) ;
		return ACTYPE_TC ;
	case AAR_OFFLINE_REFUSE :
		memcpy(pAuth, "\x5A\x32", 2) ;
		return ACTYPE_AAC ;
	default:
		break ;
	}
	return ACTYPE_AAC ;
}
int _Completion(TCompletion * pCompletion)
{
	int i ;
	TACType iACType ;
	char dolvalue[256] ;
	char resp[256] ;
	char *p;
	char * pAuthRes ;

	//第一次Generate AC返回结果处理
	pAuthRes = GetAppData("\x8A", NULL) ;
	//脱机
	if (pCompletion->TransProcR == ACTYPE_TC) 
	{	
		if (pCompletion->iCDAFail)
		{
			memcpy(pAuthRes, "\x5A\x31",2) ;
			return REFUSE_OFFLINE;
		}
		else//TC且CDA不执行或成功
		{
			memcpy(pAuthRes, "\x59\x31",2) ;
			return APROVE_OFFLINE;
		}
	}
	
	if (pCompletion->TransProcR == ACTYPE_AAC)//AAC
	{
		memcpy(pAuthRes, "\x5A\x31",2) ;
		return REFUSE_OFFLINE ;
	}
	

	//脚本处理1
	if (pCompletion->TransProcR == ONLINE_SUCC)
	{
		clrscr() ;
		printf("\n脚本处理1...\n") ;
		msdelay(100);
		IssuerScript(FALSE) ;
	}

	if (pCompletion->iCDAFail) //CDA 失败
	{
		return REFUSE_OFFLINE ;
	}
	else
	{
		iACType = TermJudge(pCompletion);
	}

	memset(dolvalue, 0x00, sizeof(dolvalue)) ;
	DOLProcess(CDOL2, dolvalue);

	memset(resp, 0x00, sizeof(resp)) ;
	i = IC_GenerateAC(iACType, pCompletion->iNeedCDA, dolvalue + 1, dolvalue[0], resp);

	if (AnalyzeACData(resp, i, pCompletion->iNeedCDA) == FAIL)
		return FAIL ;

	//脚步处理2
	if (pCompletion->TransProcR == ONLINE_SUCC)
	{
		clrscr() ;
		printf("\n脚本处理2...\n") ;
		msdelay(100);
		IssuerScript(TRUE) ;
	}
	
	p = GetAppData("\x9f\x27", NULL);
	
	if ( (p[0] & 0xC0) != 0x40 || iACType == ACTYPE_AAC)
	{
		if (iACType == ACTYPE_TC && pCompletion->TransProcR == ONLINE_SUCC)
			Online(5, NULL) ;
		return REFUSE_OFFLINE ;
	}
	
	if (pCompletion ->iNeedCDA)
	{
		return REFUSE_OFFLINE ;
		//执行CDA
	}
	else
		return APROVE_OFFLINE ;
}

int SaveLog(char iForceAccept)
{
	int fp ;
	char buff[256] ;
	char * p ;
	int len ;

	if ((fp = fopen(LOGFILE, "w")) == FAIL)
	{
		clrscr() ;
		printf("无法打开交易日志文件") ;
		return FAIL ;
	}
	memset(buff, 0x00, sizeof(buff)) ;
	fseek(fp, 0L, SEEK_END) ;

	p = GetAppData("\x5A", &len) ;
	buff[0] = len ;
	memcpy(buff + 1, p, len) ;

	if (p = GetAppData("\x9F\x36", NULL)) ;
		memcpy(buff + 11, p, 2) ;

	if (p = GetAppData("\x9F\x27", NULL)) ;
		buff[13] = *p ;

	if (p= GetAppData("\x9f\x02", NULL) )
		memcpy(buff + 14, p, 6) ;
	
	if (p= GetAppData("\x9f\x03", NULL) )
		memcpy(buff + 20, p, 6) ;
	
	GetTVR(buff + 26) ;
	GetTSI(buff + 31) ;

	if (p = GetAppData("\x8A", NULL) )
		memcpy(buff  + 34, p, 2) ;

	if (p = GetAppData("\x9A", NULL) )
		memcpy(buff + 36, p, 3) ;
	
	if (p = GetAppData("\x9f\x21", NULL) )
		memcpy(buff + 39, p, 3) ;

	buff[42] = ScriptNum * 5 ;
	memcpy(buff + 43, _ISResult, buff[42]) ;

	buff[63] = iForceAccept ;
	if (fwrite(buff, LOGLONG, fp) == FAIL)
	{
		clrscr() ;
		printf("日志写入错误") ;
		fclose(fp) ;
		return FAIL ;
	}
	len = ftell(fp) ;
	len /=  LOGLONG ;
	fclose(fp) ;
	return len ;
}

void ParseLog(char * buf)
{
	char * p ;
	SetAppData("\x5A", buf + 1, buf[0]) ;
	SetAppData("\x9F\x36", buf + 11, 2) ;
	SetAppData("\x9F\x27", buf + 13, 1) ;
	SetAppData("\x9f\x02",  buf + 14, 6) ;
	SetAppData("\x9f\x03", buf + 20, 6) ;

	p = GetAppData("\x95", NULL) ;
	memcpy(p, buf + 26, 5) ;
	p = GetAppData("\x9b", NULL) ;
	memcpy(p, buf + 31, 3) ;

	SetAppData("\x8A", buf + 34, 2);
	SetAppData("\x9A", buf + 36, 3) ;
	SetAppData("\x9f\x21", buf + 39, 3) ;

	ScriptNum = buf[42] / 5 ;
	memcpy(_ISResult, buf+43, buf[42]);

	iAcceptForced = buf[63] ;
}

int GetLog(int i, char * buf)
{
	int fp ;
	int len ;
	
	if ((fp = fopen(LOGFILE, "r")) == FAIL)
		return FAIL ;

	fseek(fp, 0L, SEEK_END) ;
	len = ftell(fp) ;
	len /= LOGLONG ;

	if (i >= len)
		return FAIL ;
	
	fseek(fp, i * LOGLONG, SEEK_SET) ;
	fread(buf, LOGLONG, fp) ;
	fclose(fp) ;
	return SUCC ;
}

int sign(void)
{
	char *p;
	char print_buf[64];//最后的打印信息存入该缓冲区
	int tpstatus;
	char * pData ;
	int len, i ;

	while(1)
	{
		clrscr() ;
		printf("_____打印_____\n\n请稍后...");
		//底层设置
		clrprintbuf();
		delay(10);
		
/*
		setprintmode(0,0);
		setprintfont(DOT8x16+(HZ12x16<<8));
*/

		setprintfont(DOT16x16 + (HZ32x16 << 8));		


		print("\r");

		memset(print_buf,0,sizeof(print_buf));
		sprintf(print_buf,"     NL8300 PBOC 测试\r\r");
		print(print_buf);

		memset(print_buf,0,sizeof(print_buf));

		pData = GetAppData("\x5A", &len) ;
		sprintf(print_buf, "\r主账号:") ;
		print(print_buf) ;
		p=print_buf ;
		for (i = 0 ;i < len; i++)
		{
			sprintf(p, "%02x", pData[i]) ;
			p += 2 ;
		}
		*p = 0x00 ;
		print(print_buf);

		memset(print_buf,0,sizeof(print_buf));
		pData = GetAppData("\x4F", &len) ;
		sprintf(print_buf, "\r应用标识:") ;
		print(print_buf);
		p=print_buf ;
		for (i = 0 ;i < len; i++)
		{
			sprintf(p, "%02x", pData[i]) ;
			if (p[0] >= 'a' && p[0] <= 'z')
				p[0] = p[0] - 0x20 ;
			if (p[1] >= 'a' && p[1] <= 'z')
				p[1] = p[1] - 0x20 ;
			p += 2 ;
		}
		*p = 0x00 ;
		print(print_buf);
		
		memset(print_buf,0,sizeof(print_buf));
		pData = GetAppData("\x9A", &len) ;
		sprintf(print_buf,
				"\r日期: %02x%02x/%02x/%02x",
				pData[0] > 50? 0x19:0x20 ,pData[0],pData[1],pData[2]); 
		print(print_buf);

		memset(print_buf,0,sizeof(print_buf));
		pData = GetAppData("\x9F\x21", &len) ;
		sprintf(print_buf,
				"\r时间: %02x:%02x:%02x",
				 pData[0],pData[1],pData[2]); 
		print(print_buf);
		
		memset(print_buf,0,sizeof(print_buf));
		pData = GetAppData("\x9F\x36", NULL) ;
		sprintf(print_buf,"\r交易序号:%02x%02x",pData[0],pData[1]);
		print(print_buf);

		memset(print_buf,0,sizeof(print_buf));
		p = GetAppData("\x81",NULL) ;
		C4ToInt((unsigned int *)&len, (unsigned char *)p) ;
		sprintf(print_buf,"\r授权金额: %10d.%02d元",
			len/100,len%100);
		print(print_buf) ;

		memset(print_buf,0,sizeof(print_buf));
		getvar((char * )&i, TSC_OFF, TSC_LEN) ;
		sprintf(print_buf, "\r终端计数器:%d", i) ;
		print(print_buf) ;
		
		if(iNeedSignature == 1)
		{
			memset(print_buf,0,sizeof(print_buf));
			sprintf(print_buf, "\r\r\r持卡人签名: _____________\r\r\r" ) ;
			print(print_buf); 
		}
		
		memset(print_buf,0,sizeof(print_buf));
		p = GetAppData("\x95", NULL) ;
		sprintf(print_buf,"\r\rTVR:%02x %02x %02x %02x %02x",p[0],p[1],p[2],p[3],p[4]) ;
		print(print_buf) ;
		
		memset(print_buf,0,sizeof(print_buf));
		p = GetAppData("\x9B", NULL) ;
		sprintf(print_buf,"\rTSI:%02x %02x ",p[0],p[1]) ;
		print(print_buf) ;

		for(i = 0 ; i < ScriptNum ; i++)
		{
			memset(print_buf,0,sizeof(print_buf));
			sprintf(print_buf, "\r脚本%d:%02x%02x%02x%02x%02x",
				i+1, _ISResult[5*i],_ISResult[5*i+1],_ISResult[5*i+2],
				_ISResult[5*i+3],_ISResult[5*i+4]) ;
			print(print_buf) ;
		}

		
		//通知底层开始打印
		kbhit();
		tpstatus=print("\f");
		if(tpstatus==TPNOPAPER )	continue;
		else if(tpstatus==TPQUIT)	return QUIT;
		// 等待打印完毕
		while(1)
		{
			tpstatus=getprinterstatus();
			if(tpstatus!=TPNOTREADY)//若打印机不在打印
			{
				if(tpstatus==TPPRINTOVER)	break;
				else	return FAIL;
			}
		}
		//底层打印时,用户程序延迟两秒钟返回主界面
		break;
	}
	return SUCC;
}
void Completion(TCompletion * pCompletion)
{
	int ret ;
	int state ;
	int len ;
	char iForcedAccept = 0 ;
	
	ret = _Completion( pCompletion) ;
	IC_PowerDown() ;
	clrscr();
	if (ret == FAIL)
	{
		printf("\n结束交易过程出错\n交易中止，可拔卡") ;
		getkeycode(1) ;
	}
	else
	{
		if (ret == REFUSE_OFFLINE && g_termparam.cAllowForceAccept)
		{
			clrscr() ;
			printf("交易被拒绝，是否强迫接受交易?\n") ;
			printf("1.是，2.否") ;
			while(1)
			{
				state = getkeycode(0) - 0x30;
				if (state == 1 || state == 2)
					break ;
			}
			if (state == 1)
			{
				ret = APROVE_OFFLINE ;
				iForcedAccept = 1 ;
			}
		}
		
		if (ret == APROVE_OFFLINE)
			len = SaveLog(iForcedAccept) ;
/*		
		if (pCompletion->TransProcR == ONLINE_SUCC)
		{	
			clrscr() ;
			printf(" 有%d条交易记录\n", len) ;
			printf("  批数据采集...") ;
			for (state = 0 ; state < len ; state++)
			{
				GetLog(state, buff) ;
				ParseLog(buff);
				Online(3, NULL) ;
			}
			fdel(LOGFILE) ;
		}
*/
		if (ret == APROVE_OFFLINE)
		{
			printf("\n  批准交易\n请移出IC卡") ;
			getkeycode(1) ;
			sign() ;
		}
		else 
		{
			printf("\n  拒绝交易\n请移出IC卡") ;
			getkeycode(1) ;
		}

	}
}
