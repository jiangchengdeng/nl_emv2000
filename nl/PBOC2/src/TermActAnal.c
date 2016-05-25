/**
* @file TermActAnal.c
* @brief 终端行为分析

* @version Version 1.0
* @author 叶明统
* @date 2005-08-05
*/
#include "posapi.h"
#include "posdef.h"
#include "tvrtsi.h"
#include "ic.h"
#include "tlv.h"
#include "dolprocess.h"
#include "AppData.h"
#include <string.h>

extern TTermParam g_termparam ;
extern void CalcMsgDigest(char *message, int len, char *digest) ;

void _PreTermActAna(TTermActAna * pTermActAna)
{
	char * pData ;
	char buff[256] ;
	char hash[256] ;
	
	pData = GetAppData("\x9f\x0d", NULL) ;
	if (pData == NULL)
	{
		SetAppData("\x9f\x0d", "\xFF\xFF\xFF\xFF\xFF", 5) ;
		pData = GetAppData("\x9f\x0d", NULL) ;
	}
	pTermActAna->IAC_Default = pData ;

	pData = GetAppData("\x9f\x0e", NULL) ;
	if (pData == NULL)
	{
		SetAppData("\x9f\x0e", "\x00\x00\x00\x00\x00", 5) ;
		pData = GetAppData("\x9f\x0e", NULL) ;
	}
	pTermActAna->IAC_Decline = pData ;

	pData = GetAppData("\x9f\x0f", NULL) ;
	if (pData == NULL)
	{
		SetAppData("\x9f\x0f", "\xFF\xFF\xFF\xFF\xFF", 5) ;
		pData = GetAppData("\x9f\x0f", NULL) ;
	}
	pTermActAna->IAC_Online = pData ;

	pTermActAna->TAC_Decline = g_termparam.TAC_Decline ;
	pTermActAna->TAC_Default = g_termparam.TAC_Default;
	pTermActAna->TAC_Online = g_termparam.TAC_Online ;

	if (pData = GetAppData("\x9F\x35", NULL))
		pTermActAna->nCanOnline = (((pData[0] & 0x0F) == 0x03 ||
				(pData[0] & 0x0F) == 0x06) ? 0 : 1) ; 
	else
		pTermActAna->nCanOnline = 1 ;
	pTermActAna->nNeedCDA = 0 ;

	memset(buff, 0, sizeof(buff));
	memset(hash, 0, sizeof(hash));
	pData = GetAppData("\x97", NULL) ;
	if (pData == NULL)
	{
		SetAppData("\x97", g_termparam.DefaultTDOL, g_termparam.DefaultTDOLen) ;
	}
	DOLProcess(TDOL, buff) ;
	if (!pData)
	{
		DelAppData("\x97") ;
	}
	CalcMsgDigest(buff + 1, buff[0], hash ) ;
	SetAppData("\x98", hash, 20) ;

}
/**
* @fn CheckTVR
* @brief 检查TVR
  
  将TVR与发卡行行为代码及终端行为代码进行比较，以确定后续交易行为
* @param (in) const TTermActAna * pTermActAna
* @param (out) char * cAuthResCode 授权响应代码
* @return @li ACTYPE_ACC	终端将向卡片请求ACC
		  @li ACTYPE_ARQC	终端将向卡片请求ARQC
		  @li ACTYPE_TC		终端将向卡片请求TC
*/
int CheckTVR( const TTermActAna * pTermActAna, char *cAuthResCode )
{
	char cTVR[5] ;
	int i ;

	GetTVR(cTVR) ; //获得TVR，用于以下比较
	for (i = 0; i < 5; i++) //拒绝
	{
		if( (pTermActAna->TAC_Decline[i] & cTVR[i]) ||
			(pTermActAna->IAC_Decline[i] & cTVR[i]) )
		{
			memcpy(cAuthResCode, "\x5A\x31", 2) ; //脱机拒绝
			return ACTYPE_AAC ; //返回AAC
		}
	}

	memcpy(cAuthResCode, "\x59\x31",2) ; //脱机批准
	if (pTermActAna->nCanOnline) //终端可以联机
	{
		for (i = 0; i < 5; i++)
		{
			if( (pTermActAna->TAC_Online[i] & cTVR[i]) || 
				(pTermActAna->IAC_Online[i] & cTVR[i]) )
			{
				memcpy(cAuthResCode, "\x00\x00", 2) ; //联机
				return ACTYPE_ARQC ; //返回AAC
			}
		}
	}
	else //缺省
	{
		for (i = 0; i < 5; i++)
		{
			if( (pTermActAna->TAC_Default[i] & cTVR[i]) ||
				(pTermActAna->IAC_Default[i] & cTVR[i]) )
			{
				memcpy(cAuthResCode, "\x5A\x33", 2) ; //无法联机，脱机被拒绝
				return ACTYPE_AAC ; //返回AAC
			}
		}
		memcpy(cAuthResCode, "\x59\x33",2) ; //无法联机，脱机批准
	}
	return ACTYPE_TC ;
}

/**
* @fn AnalyzeACData
* @brief 分析Generate AC命令返回的代码
  
  必须确保两种情况下的必备域必须存在，否则失败
* @param (in) char * response 存放Generate AC的响应代码，不包括SW1SW2 
* @param (in) int len response的长度
* @param (in) int NeedCDA 是否是CDA，是则需要9f4b域存在
* @return SUCC 解析成功
		  FAIL 失败，数据有问题
*/
int AnalyzeACData(char * response, int len, int NeedCDA)
{
	char buff[255] ;
	int len0, ret ;
	TTagRestriction tr[7] = {   //想要获取的TLV
		{"\x9f\x27", 1, 1}, {"\x9f\x36", 2, 2}, {"\x9f\x26", 8, 8},
		{"\x9f\x4b", 1,255},{"\x9f\x10",1, 32}, {"\x80",11,43} }; 
	
	memset(buff, 0, 255) ;
	TLV_Init(tr); //初始化TLV解析模块 	
	if( TLV_Decode(response, len) != TLVERR_NONE )//解析TLV，失败则返回
		return FAIL ;	
	if (*response == 0x80 )//如果是80标签
	{
		ret = TLV_GetValue("\x80", buff, &len0, 1) ;
		if(len0 == 0) //TLV格式错误，必备域，长度不能为0 
			return FAIL ;
		if( ret == TLVERR_NONE )
		{
			SetAppData("\x9f\x27", buff, 1) ;
			SetAppData("\x9f\x36", buff + 1, 2) ;
			SetAppData("\x9f\x26", buff + 3, 8) ;
			if (len0 > 11) 
				SetAppData("\x9f\x10", buff + 11, len0 - 11) ;
			return SUCC ;
		}
		else
			return FAIL ;
	}
	else if (*response == 0x77 )//77标签
	{	
		// 必备的数据域
		if (TLV_GetValue("\x9f\x27", buff, &len0,1)  != TLVERR_NONE)
			return FAIL ;
		SetAppData("\x9f\x27", buff, len0) ;
		
		if (TLV_GetValue("\x9f\x36", buff, &len0,1)  != TLVERR_NONE)
			return FAIL ;
		SetAppData("\x9f\x36", buff, len0) ;	
		if (NeedCDA)
		{
			if (TLV_GetValue("\x9f\x4B", buff, &len0,1)  != TLVERR_NONE)
				return FAIL ;
			SetAppData("\x9f\x48", buff, len0) ;
		}
		else
		{
			if (TLV_GetValue("\x9f\x26", buff, &len0,1)  != TLVERR_NONE)
				return FAIL ;
			SetAppData("\x9f\x26", buff, len0) ;
		}
		//可选数据域
		if (TLV_GetValue("\x9f\x10", buff, &len0,1)  == TLVERR_NONE)
			SetAppData("\x9f\x10", buff, len0) ;
		return SUCC ;
	}
	else //标签错误
		return FAIL ;
}

/**
* @fn TermAct_Analyze
* @brief 执行终端行为分析
  
  在执行过程，同时又有卡片内的风险管理/行为分析，并将结果返回给终端
* @param (in) const TTermActAna * pTermActAna 终端行为分析用到的数据
* @return AAC 	终端行为分析和卡片行为分析后的交易结果
		  TC	
		  ARQC		
		  FAIL 失败
*/
int TermActAnalyze( const TTermActAna * pTermActAna)
{
	int ret ;
	TACType iACType ;
	char response[256] ;
	char cdolvalue[256] ;
	char cAuthResCode[2] = { 0x00, 0x00 } ;
	char *cid ;
	
	iACType = (TACType)CheckTVR(pTermActAna, cAuthResCode) ;
	SetAppData("\x8A",cAuthResCode, 2) ;


	memset(cdolvalue, 0x00, sizeof(cdolvalue)) ;
	if (DOLProcess(CDOL1, cdolvalue) == FAIL)
		return FAIL ;

	memset(response, 0, sizeof(response)) ;
	ret = IC_GenerateAC(iACType, pTermActAna->nNeedCDA, 
		cdolvalue + 1,  cdolvalue[0], response) ;

	if( ret < 0 ) //读IC出错
		return FAIL ;

	if( AnalyzeACData(response, ret, pTermActAna->nNeedCDA) == FAIL )
		return FAIL ;

	SetTSI(CARD_RISK_MANA_COMPLETION, 1) ;
	
	cid = GetAppData("\x9f\x27", NULL) ;
	if(cid == NULL)
		return FAIL ;

	if (((*cid & 0xC0) == 0x40) && (iACType < ACTYPE_TC))
		return FAIL ;
	if ((*cid & 0xC0) && (iACType == ACTYPE_AAC))
		return FAIL ;
	
	return SUCC;
}
