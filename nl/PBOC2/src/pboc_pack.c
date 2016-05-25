#include	<string.h>
#include	<stdlib.h>

#include "../8200API/posapi.h"
#include "../8200API/define.h"

#include	"../inc/pboc_pack.h"
#include "../inc/pboc_comm.h"

static PBOC_TLV xTLVList[TOTAL_TAG_NUMBER];///<用于打包解包的TLV 列表 

//static char xDataBuf[PACK_BUF_LEN];///<用于打包解包的数据缓冲区

//BITMAP与TAG的关系映射,以下描述TAG在BITMAP中的位置
static const char TAG_MAP[TOTAL_TAG_NUMBER] = 
{
	TAG_TRANS_TYPE			,	//Transaction Type
	TAG_TRANS_COUNTER		,	//Application Transaction Counter
	TAG_ARQC				,	//ARQC
	TAG_CRYPT_INFO_DATA	,	//Cryption Information Data
	TAG_AMOUNT_AUTH		,	//Amount Authorized
	TAG_AMOUNT_OTHER		,	//Amount Other
	TAG_ENCIPHERED_PIN		,	//Enciphered PIN
	TAG_AUTH_RESP_CODE	,	//Authorization Response Code
	TAG_ISS_AUTH_DATA		,	//Issuer Authorized Data
	TAG_ISS_SCRIPT			,	//Issuer Script
	TAG_ISS_RESULT			,	//Issuer Script Results
	TAG_TERM_VERIFI_RESULT	,	//Terminal Verification Result
	TAG_TRANS_STATUS_INFO	,	//Transaction Status Information
	TAG_APP_PAN				,	//Application PAN
	TAG_TRANS_DATE			,	//Transaction Date
	TAG_TRANS_TIME			,	//Transaction Time
	TAG_CAPK_UPDATE		,	//CAPK Update Data
	TAG_TRANS_RESULT		,	//transaction result
	TAG_POS_ENTRY_MODE	,	//pos entry mode
	TAG_ISS_DATA			,// 9f10 
	TAG_TRANS_FORCEACCEPT	,//交易强迫接受标志 4FAA
};

//交易类型与BITMAP的关系映射，以下描述特定交易的BITMAP
static const PBOC_BITMAP xBitmapList[TOTAL_TRANS_NUMBER] =
{
	{TRANS_AUTH,		"111111100000010000011"},	//Authorization
	{TRANS_BATCH,		"110111010011111101011"},	//Batch Data Capture
	{TRANS_REVERSAL,	"110010010011111100011"},	//Reversal
	{TRANS_UPDATE_CAK,	"100000000000000000000"},	//Update CA key
	{TRANS_ENTRY_MODE,	"100010100000011100100"},	//Pos Entry Mode
};

/**
 *	@sn	int AssociateTLVList(PBOC_TRANS_DATA * pxData)
 *	@brief 根据TAG_MAP中定义的TAG位置把PBOC_TRANS_DATA结构中的数据关联到xTLVList
 *	@param	pxData :交易数据
 *	@return @li < 0 出错
 *		@li = 0 成功
 */
static int AssociateTLVList(PBOC_TRANS_DATA * pxData)
{
	int i = 0;
	
	memset((char *)&xTLVList[0], 0, sizeof(xTLVList));
	for(i = 0; i < TOTAL_TAG_NUMBER; i++)
	{
		switch(TAG_MAP[i])
		{
			case TAG_TRANS_TYPE			:	//Transaction Type	
				memcpy(xTLVList[i].cLabel, "\x9C", 1);
				xTLVList[i].pxValue = &(pxData->TransType);
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 1;
				break;
				
			case TAG_TRANS_COUNTER		:	//Application Transaction Counter
				memcpy(xTLVList[i].cLabel, "\x9f\x36", 2);
				xTLVList[i].pxValue = pxData->ATC;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 2;
				break;
				
			case TAG_ARQC				:	//ARQC
				memcpy(xTLVList[i].cLabel, "\x9f\x26", 2);
				xTLVList[i].pxValue = pxData->ARQC;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 8;
				break;
				
			case TAG_CRYPT_INFO_DATA	:	//Cryption Information Data
				memcpy(xTLVList[i].cLabel, "\x9f\x27", 2);
				xTLVList[i].pxValue = &(pxData->CID);
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 1;
				break;
				
			case TAG_AMOUNT_AUTH		:	//Amount Authorized
				memcpy(xTLVList[i].cLabel, "\x9f\x02", 2);
				xTLVList[i].pxValue = pxData->gAmount;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 6;
				break;
				
			case TAG_AMOUNT_OTHER		:	//Amount Other
				memcpy(xTLVList[i].cLabel, "\x9f\x03", 2);
				xTLVList[i].pxValue = pxData->gAmount_O;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 6;
				 break;
				 
			case TAG_ENCIPHERED_PIN		:	//Enciphered PIN
				memcpy(xTLVList[i].cLabel, "\x9f\x50", 2);
				xTLVList[i].pxValue = pxData->E_PIN;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 12;
				break;
				
			case TAG_AUTH_RESP_CODE	:	//Authorization Response Code						
				memcpy(xTLVList[i].cLabel, "\x8a", 1);
				xTLVList[i].pxValue = pxData->Auth_Res_Code;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 2;
				break;
				
			case TAG_ISS_AUTH_DATA		:	//Issuer Authorized Data
				memcpy(xTLVList[i].cLabel, "\x91", 1);
				xTLVList[i].pxValue = pxData->IAD;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = pxData->IADLen;
				break;
				
			case TAG_ISS_SCRIPT			:	//Issuer Script	
				memcpy(xTLVList[i].cLabel, "\x72", 1);
				xTLVList[i].pxValue = pxData->IS;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = pxData->ISLen;
				break;

			case TAG_ISS_RESULT			:	//Issuer Script Results
				memcpy(xTLVList[i].cLabel, "\x9f\x51", 2);
				xTLVList[i].pxValue = pxData->ISR;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = pxData->ISRLen;
				break;
				
			case TAG_TERM_VERIFI_RESULT	:	//Terminal Verification Result
				memcpy(xTLVList[i].cLabel, "\x95", 1);
				xTLVList[i].pxValue = pxData->TVR;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 5;
				break;
				
			case TAG_TRANS_STATUS_INFO	:	//Transaction Status Information
				memcpy(xTLVList[i].cLabel, "\x9b", 1);
				xTLVList[i].pxValue = pxData->TSI;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 2;
				break;

			case TAG_APP_PAN			:	//Application PAN
				memcpy(xTLVList[i].cLabel, "\x5A", 1);
				xTLVList[i].pxValue = pxData->Pan;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = pxData->PanLen;
				break;
				
			case TAG_TRANS_DATE		:	//Transaction Date
				memcpy(xTLVList[i].cLabel, "\x9A", 1);
				xTLVList[i].pxValue = pxData->POS_date;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 3;
				break;
				
			case TAG_TRANS_TIME			:	//Transaction Time
				memcpy(xTLVList[i].cLabel, "\x9F\x21", 2);
				xTLVList[i].pxValue = pxData->POS_time;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 3;
				break;

			case TAG_CAPK_UPDATE		:	//CAPK Update Data
				memcpy(xTLVList[i].cLabel, "\xDC", 1);
				xTLVList[i].pxValue = pxData->UpdateKey;
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = pxData->UpdateKeyLen;
				break;
				
			case TAG_TRANS_RESULT		:	//transaction result
				memcpy(xTLVList[i].cLabel, "\xDE", 1);
				xTLVList[i].pxValue = &(pxData->TransResult);
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 1;
				break;
				
			case TAG_POS_ENTRY_MODE	:	//pos entry mode
				memcpy(xTLVList[i].cLabel, "\xEE", 1);
				xTLVList[i].pxValue = &(pxData->POS_Enter_Mode);
				xTLVList[i].iLabelLen = 1;
				xTLVList[i].iValueLen = 1;
				break;
			case TAG_ISS_DATA:
				memcpy(xTLVList[i].cLabel, "\x9F\x10", 2);
				xTLVList[i].pxValue = pxData->ISS_APP_DATA;
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = pxData->IssAppDataLen ;
				break;
			case TAG_TRANS_FORCEACCEPT:
				memcpy(xTLVList[i].cLabel, "\x4F\xAA", 2);
				xTLVList[i].pxValue = &(pxData->iAcceptForced);
				xTLVList[i].iLabelLen = 2;
				xTLVList[i].iValueLen = 1 ;
				break;			
			default:
				return PACKTLV_ERR_TAGMAP;
		}
	}

	return 0;
}

/**
 *	@sn	int PackTLV(PBOC_TLV *pxInTLV, char * pxOutTLV)
 *	@brief	把PBOC_TLV结构数据打包成TLV
 *	@param	pxInTLV :PBOC_TLV结构数据指针
 *	@param	pxOutTLV:输出TLV字节串
 *	@return	@li <= 0出错
 *		@li  > 0 TLV串长度
 */
static int PackTLV(PBOC_TLV *pxInTLV, char * pxOutTLV)
{
	int iTmp = 0, iTLVLength = 0, i =0;
	char cTmp[10];
	
	iTLVLength = 0;

	//get Tag label
	memcpy(pxOutTLV, pxInTLV->cLabel, pxInTLV->iLabelLen);
	iTLVLength += pxInTLV->iLabelLen;

	//add value length
	iTmp = pxInTLV->iValueLen;
	memset(cTmp, 0, sizeof(cTmp));
	i = 0;///<计算长度值所占字节数
	while((iTmp & 0xFF) > 0)
	{
		cTmp[i] = iTmp & 0xFF;
		iTmp = iTmp >> 8;
		i++;
	}
	iTmp = pxInTLV->iValueLen;
	if((i > 3) || (iTmp < 0))
	{
		return PACKTLV_ERR_LENGTH;
	}
	if(iTmp <= 127)
	{
		//1-127，长度域的格式是：最高字节的b8位为0，b7到b1位表示值域长度
		pxOutTLV[iTLVLength] = iTmp & 0xFF;
		iTLVLength += 1;
	}
	else
	{
		//超过127，长度域的格式是：最高字节的b8位为1，b7到b1位表示长度域最高字节的后续字节的字节数，
		//最高字节的后续字节为值域的长度
		pxOutTLV[iTLVLength] =	i | 0x80;
		iTLVLength += 1;
		while(i > 0)
		{
			pxOutTLV[iTLVLength] = cTmp[i - 1];
			iTLVLength += 1;
			i--;
		}
	}

	//add value
	memcpy(&pxOutTLV[iTLVLength], pxInTLV->pxValue, pxInTLV->iValueLen);
	iTLVLength += pxInTLV->iValueLen;
	
	return iTLVLength;
}

/**
 *	@sn	int PBOC_PACK(char cTransType, PBOC_TRANS_DATA * pxData, char * pxOutData)
 *	@brief	根据xBitmapList的BITMAP定义，把交易数据打包
 *	@param	cTransType : 交易类型
 *	@param	pxData : 交易数据
 *	@param	pxOutData : 打包后的结果
 *	@return	@li	<= 0 出错
 *		@li > 0 成功
 */
static int PBOC_PACK(char cTransType, PBOC_TRANS_DATA * pxData, char * pxOutData)
{
	int i = 0, iDataLen = 0, iRet = 0;
	char bitmap[TOTAL_TAG_NUMBER+1] ;

	//关联交易数据到xTLVList，方便打包
	if((iRet = AssociateTLVList(pxData)) != 0)
	{
		return iRet;
	}
	
	memset(bitmap, 0, sizeof(bitmap));
	//获取交易的BitMap
	iRet = -1;
	for(i = 0; i < TOTAL_TRANS_NUMBER; i++)
	{
		if(xBitmapList[i].TransType == cTransType)
		{	
			memcpy(bitmap, xBitmapList[i].bitmap, TOTAL_TAG_NUMBER);
			iRet = 0;
			break;
		}
	}		

	if(iRet != 0)
	{
		return PACKTLV_ERR_LENGTH;
	}
	//根据bitmap打包
	for( i = 0; i < TOTAL_TAG_NUMBER; i ++)
	{
		if(bitmap[i] == '1')
		{
			if((iRet = PackTLV(&xTLVList[i], &pxOutData[iDataLen])) <= 0)
			{
				return iRet;
			}
			else
			{
				iDataLen += iRet;
			}
		}
	}

	return iDataLen;
}
#if 0
/**
 *	@sn	static int GetTLVTagInfo(int iOff, PBOC_TLV *pxTag)	
 *	@brief	从iOff位置开始获取1个TAG的信息
 *	@param iOff :tag在xDataBuf中的开始位置
 *	@param pxTag:获取的该TAG的信息
 *	@return @li <= 0出错
 *	@li > 0当前指针偏移位置,供解下一个TAG使用
 */
static int GetTLVTagInfo(int iOff, PBOC_TLV *pxTag)	
{
	int i = 0, iLenLen = 0, iDataLen = 0, j = 0;
	char tmp[5];

	i = iOff;
//取Tag
	if((xDataBuf[iOff] & 0x1f) == 0x1f)
	{	
		i += 1;

		while(i < MAX_LABEL_LEN)
		{
			i += 1; 
			if((xDataBuf[i] & 0x80) == 0)
			{
				break;
			}
		}
	}
	else
	{
		i += 1;
	}
	pxTag->iLabelLen = i - iOff;
	if(pxTag->iLabelLen > MAX_LABEL_LEN)
	{
		return -1;
	}
	memcpy(pxTag->cLabel, &xDataBuf[iOff], pxTag->iLabelLen);
	
	//取长度
	if(xDataBuf[i] & 0x80)
	{
		iLenLen = xDataBuf[i] & 0x7f;
		i += 1;
		memcpy(tmp, &xDataBuf[i], iLenLen);
		for(j = 0; j < iLenLen; j++)
		{
			iDataLen = (iDataLen << 8) + tmp[j];
		}
		i += iLenLen;
	}
	else
	{
		iDataLen = xDataBuf[i] & 0x7f;
		i += 1;
	}
	pxTag->iValueLen = iDataLen;
	
	//取数据
	pxTag->pxValue = &xDataBuf[i]; 
	i += iDataLen;
	
	return i;
}
/**
 *	@sn	static int PBOC_UNPACK(int iDataLen, char * pxInData, PBOC_TRANS_DATA * pxOutData)
 *	@brief	将一个ＴＬＶ串数据解到PBOC_TRANS_DATA结构中
 *	@param	iDataLen	:输入ＴＬＶ串的长度
 *	@param	pxInData	:输入ＴＬＶ串
 *	@param	pxOutData:保存解包的结果
 *	@return	@li <0 出错
 *	@li = 0 成功
 */
static int PBOC_UNPACK(int iDataLen, char * pxInData, PBOC_TRANS_DATA * pxOutData)
{
	int  iRet = 0;
	PBOC_TLV tmp;
	char iOff = 0;

	//关联交易数据到xTLVList，方便打包
	if((iRet = AssociateTLVList(pxOutData)) != 0)
	{
		return iRet;
	}
	if(iDataLen > PACK_BUF_LEN)
	{
		return -1;
	}
	memcpy(xDataBuf, pxInData, iDataLen);
	iOff = 0;
	while(iOff < iDataLen)///<如果Off == iDataLen则解包结束
	{			
		memset((char *)&tmp, 0, sizeof(PBOC_TLV));
		iRet = GetTLVTagInfo(iOff, &tmp);
		
		if(iRet > 0)
		{
			iOff = iRet;//为解下个TAG准备
		}
		else
		{
			return -1;
		}
		switch(tmp.pxValue[0])
		{
			case '\x9C':	//transtype
				memcpy(&pxOutData->TransType, tmp.pxValue, 1);
				break;
			case '\x8a':	//ARC
				memcpy(pxOutData->Auth_Res_Code, tmp.pxValue, 2);
				break;
			case '\x91':	//Issuer Auth Data
				pxOutData->IADLen = tmp.iValueLen;
				memcpy(pxOutData->IAD, tmp.pxValue, pxOutData->IADLen);
				break;
			case '\x72':	//Issuer Script, Var length
				pxOutData->ISLen = tmp.iValueLen;
				pxOutData->ISFlag = 0x72;
				memcpy(pxOutData->IS, tmp.pxValue, pxOutData->ISLen);
				break;
			case '\x71':	//Issuer Script, Var length
				pxOutData->ISLen = tmp.iValueLen;
				pxOutData->ISFlag = 0x71;
				memcpy(pxOutData->IS, tmp.pxValue, pxOutData->ISLen);
				break;
			case '\xdc':
				pxOutData->UpdateKeyLen = tmp.iValueLen;
				memcpy(pxOutData->UpdateKey, tmp.pxValue, pxOutData->UpdateKeyLen);
				break;
			default:		//Unknown Tag
				break;
		}
	
	}
	if(iOff != iDataLen)///<包长度不一致
	{
		return -1;
	}
	
	return 0;
}
#endif
/**
 *	@sn	int CommWithHost(char cTransType, PBOC_TRANS_DATA * pxInData,  char * pxOutData)
 *	@brief	与主机通讯
 *	@param	cTransType :交易类型
 *	@param	pxInData:终端得到的交易数据
 *	@param	pxOutData:主机返回交易响应数据
 *	@return < 0出错
 *	@li == 0成功
 */
int CommWithHost(char cTransType, PBOC_TRANS_DATA * pxInData, char * pxOutData)
{
	int iRet = 0, iDataLen = 0;
	char xDataBuf[PACK_BUF_LEN];
	
	InitComm();
	memset(xDataBuf, 0, sizeof(xDataBuf));
	if((iRet = PBOC_PACK(cTransType, pxInData, xDataBuf)) <= 0)
	{
		return iRet;
	}
	if((iRet = SendData(xDataBuf, iRet)) != SUCC)
	{
		return iRet;
	}
	if((iRet = ReceiveData(pxOutData, &iDataLen)) != SUCC)
	{
		return iRet;
	}	
	
	return iDataLen;
}

