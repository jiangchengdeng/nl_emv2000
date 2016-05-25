/**
* @file TermRisk.c
* @brief ÖÕ¶Ë·çÏÕ¹ÜÀí

* @version Version 1.0
* @author Ò¶Ã÷Í³
* @date 2005-08-05
*/

#include <string.h>
#include <stdlib.h>
#include "posapi.h"
#include "posdef.h"
#include "define.h"
#include "tvrtsi.h"
#include "ic.h"
#include "AppData.h"
#include "tools.h"
#include "TermRisk.h"

extern TTermParam g_termparam ;

void _PreTermRiskManage(TTermRisk * pTermRisk)
{
	pTermRisk->pPAN = GetAppData("\x5a", NULL);
	pTermRisk->pAIP = GetAppData("\x82", NULL);
	pTermRisk->Lower_COL= GetAppData("\x9f\x14", NULL);
	pTermRisk->Upper_COL= GetAppData("\x9f\x23", NULL);
	pTermRisk->MaxTargetPercent = g_termparam.MaxTargetPercent ;
	pTermRisk->TargetPercent = g_termparam.TargetPercent ;
	pTermRisk->FloorLimt = g_termparam.FloorLimit ;
	pTermRisk->ThresholdValue = g_termparam.ThresholdValue ;

}

//²éÑ¯¿¨ºÅÊÇ·ñÔÚºÚÃûµ¥ÖĞ
// SUCC ²»ÔÚºÚÃûµ¥ÖĞ  FAIL ÔÚºÚÃûµ¥ÖĞ
int	BlackListExcepFile(char *pPAN)
{
	int fp;
	int len;
	TBlackCard blackcard;
	
	//´ò¿ªºÚÃûµ¥ÎÄ¼ş
	fp = fopen(BlackListFile, "r");
	if(fp < 0)
	{
		return SUCC;
	}

	while(1)
	{
		len = fread((char *)&blackcard, sizeof(TBlackCard), fp);
		if(len != sizeof(TBlackCard))
		{
			break;			
		}

		if(memcmp(pPAN, blackcard.cardno, blackcard.len)==0)
		{
			fclose(fp);
			SetTVR(CARD_IN_EXCEP_FILE, 1);
			return FAIL;
		}
	}
	
	fclose(fp);
	return SUCC;
}

/**
* @fn Floor_Limit
* @brief ×îµÍÏŞ¶î
  
  ¼ÆËã½»Ò×½ğ¶î£¨°üÀ¨µ±Ç°µÄÓë×î½üÊ¹ÓÃµÄ£©£¬ÅĞ¶ÏÊÇ·ñ³¬¹ı
  ½»Ò×ÏŞ¶î£¬²¢ÖÃÏàÓ¦µÄTVRÎ»
* @param (in) const char * FL_TERM ÖÕ¶Ë½»Ò×ÏŞ¶î
* @param (in) int iAmount µ±Ç°½»Ò×½ğ¶î£¨ÊÚÈ¨½ğ¶î£©
* @return SUCC <×îµÍÏŞ¶î
          FAIL >=×îµÍÏŞ¶î
*/
int FloorLimit(const char * FL_TERM, int iAmount)
{
	int amount = 0;
	int termlimit ;
	
	C4ToInt( (unsigned int *)&termlimit, (unsigned char *)FL_TERM ) ;
//	amount = getlatestamount() ;
	if ( amount + iAmount >= termlimit )
	{
		SetTVR(TRADE_EXCEED_FLOOR_LIMIT, 1) ;
		return FAIL ;
	}
	return SUCC ;
}

/**
* @fn RandTransSelect
* @brief Ëæ»ú½»Ò×Ñ¡Ôñ
  
* @param (in) const TTermRisk *  pTermRisk Ö¸ÏòÖÕ¶Ë·çÏÕ¹ÜÀíÓÃµ½µÄÊı¾İ¬°üº¬±¾´Î²Ù×÷ÒªÊ¹ÓÃµÄÓò
* @param (in) int iAmount µ±Ç°½»Ò×½ğ¶î
* @return void
*/
void RandTransSelect( const TTermRisk * pTermRisk, int iAmount )
{
	int termfl ;				// ÖÕ¶Ë×îµÍÏŞ¶î 
	int ThresValue ;			// Æ«ÖÃËæ¼´ãĞÖµ
	float TransTargPercent ;	// ½»Ò×Ä¿±ê°Ù·Ö±È 
	float InterpolationFactor ; // ²åÖµÒò×Ó 
	int RandomPercent ;			// Ëæ»ú°Ù·ÖÊı 
	struct postime pt ;

	C4ToInt((unsigned int *)&termfl, (unsigned char *)pTermRisk->FloorLimt) ;
	C4ToInt((unsigned int *)&ThresValue, (unsigned char *)pTermRisk->ThresholdValue) ;

	getpostime(&pt) ;
	srand(pt.second + pt.minute * 60 + pt.hour * 60 * 60) ;
	RandomPercent = rand() % 99 + 1 ;

	if (iAmount < ThresValue) //½»Ò×½ğ¶îĞ¡ÓÚËæ»úÆ«ÖÃãĞÖµ
	{
		if (RandomPercent <= pTermRisk->TargetPercent) //Ëæ»ú°Ù·ÖÊıĞ¡ÓÚÄ¿±ê°Ù·ÖÊı
		{
			SetTVR(SELECT_ONLILNE_RANDOM, 1) ;
		}
	}
	else if (iAmount < termfl) // ½»Ò×½ğ¶î´óÓÚËæ»úÆ«ÖÃãĞÖµ£¬Ğ¡ÓÚ×îµÍÏŞ¶î
	{
		InterpolationFactor = (float)(iAmount - ThresValue) /    //²åÖµÒò×Ó
			(float)(termfl - ThresValue) ;
		TransTargPercent = (float)(pTermRisk->MaxTargetPercent -  //½»Ò×Ä¿±ê°×·ÖÊı
			pTermRisk->TargetPercent) * InterpolationFactor + 
			pTermRisk->TargetPercent ;
		if ( (float)RandomPercent <= TransTargPercent )
		{
			SetTVR(SELECT_ONLILNE_RANDOM, 1) ;
		}
	}
}

/**
* @fn GetATC
* @brief »ñÈ¡Ó¦ÓÃ½»Ò×ĞòºÅ£¬°üÀ¨µ±Ç°µÄ£¬ºÍ×î½üÁª»úµÄ
 
* @param (out) char * atc ·µ»Øµ±Ç°ATC 
* @param (out) char * onlineatc ·µ»Ø×î½üÁª»úATC
* @return 0 ¶şÕß¶¼Ã»ÓĞÈ¡µ½
          1 È¡µ½atc
		  2 È¡µ½onlineatc
		  3 Í¬Ê±È¡µ½¶şÕß
*/
int GetATC( char * atc, char * onlineatc )
{
	int ret ;
	char oData[256] ;
	int iExist = 0 ;
    
	/* È¡ATC */
	memset(oData, 0, 10) ;
	ret = IC_GetData( GETDATAMODE_ATC, oData ) ;
	if ( ret > 0)
	{
		iExist |= 0x01 ;
		memcpy( atc, oData + 3, 2 ) ;
	}
    /* È¡Latest Online ATC */
	memset(oData, 0, 10) ;
	ret = IC_GetData( GETDATAMODE_ONLINEATC, oData ) ;
	if ( ret > 0)
	{
		iExist |= 0x02 ;
		memcpy( onlineatc, oData + 3, 2 ) ;
	}
	return iExist ;
}
/**
* @fn CheckVelocity
* @brief Æµ¶È¼ì²é
 
* @param (in) const TTermRisk *  pTermRisk Ö¸ÏòÖÕ¶Ë·çÏÕ¹ÜÀíÓÃµ½µÄÊı¾İ
* @return SUCC ÒÑ¼ì²é
          FAIL Ã»ÓĞ¼ì²é
*/
int CheckVelocity( const TTermRisk *  pTermRisk ) 
{
	int ret ;
	char atc[2] ; // Ó¦ÓÃ½»Ò×ĞòºÅ 
	char lastonlineatc[2] ; // ×î½üÁª»úÓ¦ÓÃ½»Ò×ĞòºÅ 
	int atctmp, loatctmp ;

	// ²»´æÔÚÁ¬ĞøÍÑ»ú½»Ò×ÏÂÏŞ»òÉÏÏŞ
	if ((pTermRisk->Lower_COL == NULL)||
		(pTermRisk->Upper_COL == NULL))
	{
		return FAIL ; //Ìø¹ı´Ë½Ú
	}
	
	ret = GetATC(atc, lastonlineatc) ;
	if ( ret & 0x02 )
	{
		loatctmp = (((int)lastonlineatc[0]) << 8) + lastonlineatc[1] ;
		if ( loatctmp == 0 )		//ÉÏ´ÎÁª»ú½»Ò×ĞòºÅ¼Ä´æÆ÷Îª0
		{
			SetTVR(NEW_CARD, 1) ;	// ÉèÖÃĞÂ¿¨±êÖ¾
		}
	}
	if ( ret != 3) // ÎŞ·¨´ÓICCÍ¬Ê±È¡µ½atc ºÍ lastonlineatc 
	{
		SetTVR(EXCEED_CON_OFFLINE_FLOOR_LIMIT, 1) ; //³¬¹ıÁ¬ĞøÍÑ»úÏÂÏŞ
		SetTVR(EXCEED_CON_OFFLINE_UPPER_LIMIT, 1) ; //³¬¹ıÁ¬ĞøÍÑ»úÉÏÏŞ
		SetTVR(ICC_DATA_LOST, 1) ; //  IC¿¨Êı¾İÈ±Ê§ 
	}
	else
	{
		atctmp = (((int)atc[0]) << 8) + atc[1] ;
		if (atctmp <= loatctmp)
		{
			SetTVR(EXCEED_CON_OFFLINE_FLOOR_LIMIT, 1) ;
			SetTVR(EXCEED_CON_OFFLINE_UPPER_LIMIT, 1) ;
		}
		if ( atctmp - loatctmp > *(pTermRisk->Lower_COL) ) //Èç¹û´óÓÚÁ¬ĞøÍÑ»úÏÂÏŞ
		{
			SetTVR(EXCEED_CON_OFFLINE_FLOOR_LIMIT, 1) ;
		}
		if ( atctmp - loatctmp > *(pTermRisk->Upper_COL) ) //Èç¹û´óÓÚÁ¬ĞøÍÑ»úÉÏÏŞ
		{
			SetTVR(EXCEED_CON_OFFLINE_UPPER_LIMIT, 1) ;
		}
	}
	return SUCC ;
}

/**
* @fn TermRisk_Manage
* @brief Æµ¶È¼ì²é
 
* @param (in) TTermRisk * pTermRisk  ÖÕ¶Ë·çÏÕ¹ÜÀíÓÃµ½µÄÊı¾İ
* @param (in) int iAmount µ±Ç°½»Ò×½ğ¶î(ÊÚÈ¨½ğ¶î)
* @return SUCC ÒÑ¼ì²é
          FAIL Ã»ÓĞ¼ì²é
*/
int TermRiskManage(const TTermRisk * pTermRisk , int iAmount )
{
	int ret ;
	if ( *(pTermRisk->pAIP) & 0x08 )
	{
		BlackListExcepFile(pTermRisk->pPAN) ; //ÖÕ¶ËºÚÃûµ¥ºÍÒì³£ÎÄ¼ş´¦Àí
		if (g_termparam.cAllowForceOnline)
		{
			clrscr() ;
			printf("ÊÇ·ñÇ¿ÆÈÁª»ú?\n") ;
			printf("ÊÇ(È·ÈÏ). \n·ñ(È¡Ïû).") ;
			while(1) 
			{
				ret = getkeycode(0) ;
				if (ret == ENTER || ret == ESC)
					break ;
			}
			if (ret == ENTER)
				SetTVR(MERCHANT_REQ_ONLINE, 1) ;
		}
		FloorLimit(pTermRisk->FloorLimt, iAmount) ; //×îµÍÏŞ¶î¼ì²é
		RandTransSelect(pTermRisk, iAmount) ; //Ëæ¼´½»Ò×Ñ¡Ôñ
		CheckVelocity(pTermRisk) ; //Æµ¶È¼ì²é
		SetTSI(TERM_RISK_MANA_COMPLETION, 1) ;
	}
	return SUCC ;
}
