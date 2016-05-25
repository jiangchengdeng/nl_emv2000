/**
 * @file ProcessLimit.c
 * @brief 处理限制

   测试IC卡中的应用与终端中的应用的兼容程度
   并做必须的调整，包括拒绝交易
   检查的内容包括应用生效日期，应用失效日期
   应用版本号以及其他发卡行定义的限制条件(应用用途控制AUC)
 * @version Version 1.0
 * @author 叶明统
 * @date 2005-8-25 
 */
#include <string.h>
#include "posapi.h"
#include "AppData.h"
#include "tvrtsi.h"

#define DATE(yearh, yearl, month, day) \
	(int)(( (unsigned int) yearh << 24) | ( (unsigned int) yearl << 16) | \
	( (unsigned int) month << 8) | ( (unsigned int) day ))
/**
 * @fn ProcessLimit

 * @param int nIsATM 当前是否是ATM设备
 * @return FAIL
 			SUCC
 */
int ProcessLimit(int nIsATM)
{
	char * ptrData1, *ptrData2, *ptrData3, *ptrData4 ;
	struct postime pt ;
	char yearh ;
	int invalid = 1 ;
	//应用版本号检查
	if ( (ptrData1 = GetAppData("\x9F\x08", NULL)) &&  	//IC 卡应用版本号
		(ptrData2 = GetAppData("\x9F\x09", NULL) ) )	//终端应用版本号
	{
		if (memcmp(ptrData1, ptrData2, 2) !=0)
		{
			SetTVR(VERSION_NOT_MATCHED, 1) ;
		}
	}

	//应用用途控制检查
	if ( ptrData1 = GetAppData("\x9F\x07", NULL))		//应用用途控制存在
	{
		if (ptrData2 = GetAppData("\x5F\x28", NULL))	//发卡行国家代码存在
		{
			ptrData3 = GetAppData("\x9F\x1A",NULL) ; 	//终端国家代码,一定存在
			if (!ptrData3)
				return FAIL ;
			ptrData4 = GetAppData("\x9C", NULL) ; 		//交易类型，一定存在
			if (!ptrData4)
				return FAIL ;	
			if (memcmp(ptrData2, ptrData3, 2) == 0) 	// 国内交易
			{
				switch(*ptrData4)
				{
				case 0x00:		//商品和服务
					if ( *ptrData1 & 0x20 ) //有效
						invalid = 0 ;
					break ;
				case 0x01:		//现金交易
					if ( *ptrData1 & 0x80 ) //有效
						invalid = 0 ;
					break ;
				default :
					break ;
				}
			}
			else //国际交易
			{
				switch(*ptrData4)
				{
				case 0x00:		//商品和服务
					if ( *ptrData1 & 0x10) //有效
						invalid = 0 ;
					break ;
				case 0x01:		//现金交易
					if ( *ptrData1 & 0x40 ) //有效
						invalid = 0 ;
					break ;
				default :
					break ;
				}
			}
		}
		else
		{
			invalid = 0;
		}
		
		if (nIsATM) //是ATM设备
		{
			if ( !(*ptrData1 & 0x02) && !invalid ) //ATM 有效
				invalid = 1 ;
		}
		else //非ATM设备
		{
			if ( !(*ptrData1 & 0x01) && !invalid) //除ATM 外的终端有效
				invalid = 1 ;
		}
	}	
	else
	{
		invalid = 0;
	}
	
	if (invalid)
		SetTVR(SERV_REFUSE, 1) ;

	//应用有效期/失效期检查
	getpostime(&pt) ;
	if (ptrData1 = GetAppData("\x5F\x25", NULL) ) //如果卡中有应用生效日期
	{
		yearh = *ptrData1 > 0x50 ? 0x19 : 0x20 ;		
		if (DATE(pt.yearh, pt.yearl, pt.month, pt.day) <=
			DATE(yearh, *ptrData1, *(ptrData1 + 1), *(ptrData1 + 2)) )
		{
			SetTVR(APP_NOT_EFFECT, 1) ;
		}
	}

	ptrData1 = GetAppData("\x5F\x24", NULL) ;//读取应用失效日期
	yearh = *ptrData1 > 0x50 ? 0x19 : 0x20 ;
	if (DATE(pt.yearh, pt.yearl, pt.month, pt.day) >
			DATE(yearh, *ptrData1, *(ptrData1 + 1), *(ptrData1 + 2)) )
		SetTVR(APP_EXPIRE, 1) ;

	return SUCC ;	
}
