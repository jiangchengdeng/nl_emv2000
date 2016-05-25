/**
* @file tvrtsi.c
* @brief 设置TVR,TSI

  该模块在EMV2000中使用，完成如下两个功能
       @li 根据参数设置TVR字节（终端验证结果）
	   @li 根据参数设置TSI字节（交易状态信息）
* @version Version 1.0
* @author 叶明统
* @date 2005-08-02
*/
#include "AppData.h"
#include "tvrtsi.h"
#include <string.h>

///全局TVR
static char *  _tvrvalue ; 
///全局TSI
static char *  _tsivalue ;

/**
* @fn InitTVR
* @brief 初始化TVR
*/
void InitTVR(void)
{
	char tmp[5] ;
	memset(tmp, 0x00, 5) ;
	SetAppData("\x95", tmp, 5) ;
}

/**
* @fn SetTVR
* @brief 根据iSetMask设置TVR相应的位

* @param (in) int iSetMask 要在TVR字节中设置的位的掩码
* @param (in) OnOff  打开(1)或关闭(0)某位，
* @return void
*/
void SetTVR( int iSetMask, int OnOff )
{
	_tvrvalue = GetAppData("\x95", NULL) ;
	if( OnOff )
	{
		_tvrvalue[iSetMask >> 8]  |= (unsigned char)iSetMask ;
	}
	else
	{
		_tvrvalue[iSetMask >> 8]  &= ~((unsigned char)iSetMask) ;
	}

}

/**
* @fn InitTSI
* @brief 初始化TSI
*/
void InitTSI(void)
{
	char tmp[2] ;
	memset(tmp, 0x00, 2) ;
	SetAppData("\x9B", tmp, 2) ;
}

/**
* @fn SetTSI
* @brief 根据iSetMask设置TSI相应的位

* @param (in) int iSetMask 要在TSI字节中设置的位的掩码
* @param (in) int OnOff  打开(1)或关闭(0)某位
* @return void
*/
void SetTSI( int iSetMask, int OnOff )
{
	_tsivalue = GetAppData("\x9B", NULL) ;
	if ( OnOff )
	{
		_tsivalue[iSetMask >> 8] |= (unsigned char)iSetMask ;
	}
	else
	{
		_tsivalue[iSetMask >> 8] &= ~((unsigned char)iSetMask) ;
	}
}

/**
* @fn GetTVR
* @brief 返回TVR字节

* @param (out) char * tvrtmp 返回TVR字节
* @return void
*/
void GetTVR( char * tvrtmp )
{

	memcpy(tvrtmp, _tvrvalue, 5) ;

}

/**
* @fn GetTSI
* @brief 返回TVR字节

* @param (out) char * tsitmp 返回TVR字节
* @return void
*/
void GetTSI( char * tsitmp )
{
	memcpy(tsitmp, _tsivalue, 2) ;
}
