/**
 * @file OnlineProc.c
 * @brief 联机处理

 * @version Version 1.0
 * @author 叶明统
 * @date 2005-08-20
 */
#include "posapi.h"
#include "posdef.h"
#include "tvrtsi.h"
#include "ic.h"
#include "AppData.h"
#include "OnlineProc.h"
#include "pboc_comm.h"
#include "pboc_pack.h"
#include "tools.h"
#include <string.h>

void _PreTransProc(TTransProc * pTransProc, char * pPIN)
{
	char * pData ;
	pTransProc->iNeedCDA = 0 ;

	pTransProc->iTransType = 1 ;
	if (pData = GetAppData("\x9F\x35", NULL))
		pTransProc->iCanOnline = (((pData[0] & 0x0F) == 0x03 ||
				(pData[0] & 0x0F) == 0x06) ? 0 : 1) ; 
	pTransProc->E_PIN = pPIN ;
}

int AnaRespData(char * pData, int len)
{
	int i  = 0 ;
	char buff[2];
	int tag72 = 0 ;
	int tag71 = 0 ;

	while (i < len) 
	{
		switch(*(pData + i))
		{
		case 0x8A:
			SetAppData("\x8A", pData + i + 2, 2) ;
			break ;
		case 0x9c:
			SetAppData("\x9C", pData + i + 2, 1) ;
			break ;
		case 0x91:
			SetAppData("\x91", pData + i + 2, *(pData+i + 1)) ;
			break ;
		case 0x71:
		case 0x72:
			memset(buff, 0x00, 2) ;
			if (*(pData + i) == 0x72)
			{
				buff[0] = 0x7F ;
				buff[1] = ++tag72 ;
			}
			else
			{
				buff[0] = 0x3F ;
				buff[1] = ++tag71 ;
			}
			SetAppData(buff, pData + i + 2, *(pData + i + 1)) ;
			break ;
		default:
			break ;
		}
		i += 2 + *(pData + i + 1) ;
	}
	if (i != len )
	{
		return FAIL ;
	}
	return SUCC ;
}

int Online(int iTransType, char * pPIN)
{
	char * pData ;
	char pOutData[256] ;
	PBOC_TRANS_DATA pInData ;
	int len ;

	memset(&pInData, 0x00, sizeof(PBOC_TRANS_DATA)) ;
	memset(&pOutData, 0x00, sizeof(pOutData)) ;
	
	pInData.TransType = iTransType ;

	if (pData = GetAppData("\x9f\x36", NULL) )
		memcpy(pInData.ATC, pData, 2) ;

	if (pData = GetAppData("\x9f\x26", NULL) )
		memcpy(pInData.ARQC, pData, 8) ;

	if (pData = GetAppData("\x9f\x27", NULL) )
		pInData.CID = * pData  ;

	if (pData = GetAppData("\x8A", NULL) )
		memcpy(pInData.Auth_Res_Code, pData, 2) ;

	if (pData = GetAppData("\x9f\x39", NULL) )
		pInData.POS_Enter_Mode = *pData ;

	if (pData = GetAppData("\x5A", &len) )
	{
		memcpy(pInData.Pan, pData, len) ;
		pInData.PanLen = len ;
	}

	if (pData = GetAppData("\x9f\x02", NULL) )
		memcpy(pInData.gAmount, pData, 6) ;
	
	if (pData = GetAppData("\x9f\x03", NULL) )
		memcpy(pInData.gAmount_O, pData, 6) ;

	if (pData = GetAppData("\x9f\x21", NULL) )
		memcpy(pInData.POS_time, pData, 3) ;
	
	if (pData = GetAppData("\x9A", NULL) )
		memcpy(pInData.POS_date, pData, 3) ;

	if (pData = GetAppData("\x9F\x10", &len) )
	{
		memcpy(pInData.ISS_APP_DATA, pData, len) ;
		pInData.IssAppDataLen = len ;
	}
	
	GetTVR(pInData.TVR) ;
	GetTSI(pInData.TSI) ;
	pInData.ISRLen = ScriptNum * 5 ;
	memcpy(pInData.ISR, _ISResult, pInData.ISRLen) ;

	pInData.iAcceptForced = iAcceptForced ;
	if (pPIN)
		memcpy(pInData.E_PIN, pPIN, 12) ;
	if ((len = CommWithHost(iTransType, &pInData,pOutData)) == FAIL) 
		return FAIL ;

	if (iTransType == 0x01)
	{
		if (AnaRespData(pOutData, len) == FAIL)
		{
			clrscr() ;
			printf("后台响应数据解析错误\n") ;
			getkeycode(0) ;
			return -2000 ;
		}
	}
	return SUCC ;
}

int TransProc(TTransProc * pTransProc)
{
	char * p ;
	int len ;
	int i ;
	
	if (pTransProc->iNeedCDA) //如果脱机认证要用到CDA, 则需先作CDA处理
	{
		//详细的CDA处理，暂时不支持CDA
	}
	if ( (p = GetAppData("\x9f\x27",NULL)) == NULL)
		return FAIL ;

	if(*p & 0x01) //不允许的服务
	{
		clrscr() ;
		printf("\n不允许的服务") ;
		getkeycode(0) ;
		return FAIL ;
	}
	if( (*p & 0xC0) == 0x00)
	{
		return ACTYPE_AAC;
	}
	if( (*p & 0xC0) == 0x40)
	{
		return ACTYPE_TC;
	}
	if( (*p & 0xC0) == 0xC0)
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
			printf("1.强制联机\n") ;
			printf("2.脱机批准\n") ;
			printf("3.脱机拒绝\n") ;
			len = getkeycode(0) - 0x30 ;
			if (len <=3 && len >=1)
				break ;
		}
		if (len == 2)
			return AAR_OFFLINE_APPROVE ;
		if (len == 3)
			return AAR_OFFLINE_REFUSE ;
	}
	if (!(pTransProc->iCanOnline))
		return ONLINE_FAIL ;
	

	//联机请求
	clrscr() ;
	printf("\n联机处理......") ;
	if (Online(pTransProc->iTransType, pTransProc->E_PIN) == FAIL)
	{
		clrscr();
		printf("\n联机处理失败\n") ;
		getkeycode(0);
		return ONLINE_FAIL ;
	}
	
	//以下情况结束交易
	p = GetAppData("\x82",NULL) ;
	if ( !(*p & 0x04)) //AIP不支持发卡行鉴定
	{
		 return ONLINE_SUCC;
	}
	
	if ((p = GetAppData("\x91", &len)) == NULL) //发卡行鉴定数据
		return ONLINE_SUCC ;

	SUCC_MSG("发卡行认证") ;
	
	//发卡行认证
	if ( (i = IC_ExternalAuthenticate(p, len)) < 0)
	{
		clrscr() ;
		printf("认证失败\n") ;
		getkeycode(30) ;

/* 补充CASE CI032.00不允许退出
		if (i != ICERR_AUTHFAIL)
			return FAIL ;
*/
		SetTVR(ISSUER_AUTH_FAIL, 1) ;
	}
	SetTSI(ISSUER_AUTH_COMPLETION, 1) ;
	
	return ONLINE_SUCC ;
}
