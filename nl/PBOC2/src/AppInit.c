/**
 * @file AppInit.c
 * @brief 应用初始化

	对当前选定的应用进行初始化,通过组装PDOL指定的数据,
	向IC卡发送取处理响应命令,并根据返回的数据判断是否
	清楚当前应用而返回到应用选择,或则继续下面的读应用数据
 * @version Version 1.0
 * @author 叶明统
 * @date 2005-8-25
 */

#include <string.h>
#include "posapi.h"
#include "posdef.h"
#include "ic.h"
#include "tvrtsi.h"
#include "DOLProcess.h"
#include "AppData.h"
#include "AppInit.h" 
#include "tlv.h"



/**
 * @fn int AnalyzeProcOp(char *, int)
 * @brief 分析取处理选项的返回

 * @param char * resp 取处理选项的返回
 * @param int	   ret	 resp的长度
 * @return 
 */
int AnalyzeProcOp (char * resp, int ret)
{
	TTagRestriction tr[] = {
			{"\x82", 2, 2}, {"\x94", 0, 252}, {"\x77", 0, 0}} ;
	char value[256] ;
	int len ;
	
	//响应报文中的数据对象是一个标签为'80'的基本数据对象
	if (*resp == 0x80)
	{
		if (*(resp + 1) == 0 || *(resp + 1) == 2) //长度为0视为出错
			return FAIL ;
		if (*(resp+1) % 4 != 2) //应用交互特征2个字节，AFL 4字节边界
			return FAIL ;
		SetAppData("\x82", resp + 2, 2) ; //AIP的标签为'83'
		SetAppData("\x94", resp + 4, *(resp + 1) - 2) ; //AFL 标签为'94'
		return SUCC ;
	}
	else if (*resp = 0x77)
	{
		TLV_Init(tr) ;
		if (TLV_Decode(resp, ret) != TLVERR_NONE)
			return FAIL ;
		
		memset(value, 0x00, sizeof(value)) ;
		if (TLV_GetValue("\x82", value, &len, 1) == TLVERR_TAGNOTFOUND)
			return FAIL ;
		SetAppData("\x82", value, len) ;
		if (TLV_GetValue("\x94", value, &len, 1) == TLVERR_TAGNOTFOUND)
			return FAIL ;
		SetAppData("\x94", value, len) ;
		return SUCC ;
	}
	else 
		return FAIL ;
}
/**
 * @fn int AppInit(void)

 * @return SUCC 成功
 			 FAIL 失败
 */
int AppInit (void)
{
	char PDOL_Value[256] ;//APDU数据域最大不能超过255字节(LC决定)
	char resp[256] ;//APDU的响应,最大为256字节,由(LE)决定
	int ret ;
	char trace[4];

	InitTVR() ; //初始化TVR
	InitTSI() ; //初始化TSI

	memset (PDOL_Value, 0, 256) ;
	if (DOLProcess (PDOL, PDOL_Value) == FAIL)//建立数据元列表失败
		return FAIL ;

	memset(resp, 0x00, 256) ;
	//PDOL_Value第一字节为后续字节的长度
	if( (ret = IC_GetProcessOptions(PDOL_Value + 1, PDOL_Value[0], resp)) < 0 )
	{	
		IC_GetLastSW (PDOL_Value, PDOL_Value+1) ;
		if (*PDOL_Value == 0x69 && *(PDOL_Value + 1) ==0x85)
		{
			//需考虑清除当前应用，并回到应用选择
			return QUIT ;
		}
		return FAIL ; 	
	}
	
	//分析处理选项命令的响应
	if(AnalyzeProcOp( resp,  ret) == FAIL)
 		return FAIL ;

	getvar((char * )&ret, TSC_OFF, TSC_LEN) ;
	ret ++ ;
	if (ret > 99999999)
		ret = 0 ;

	memset(trace, 0, sizeof(trace));
	IntToC4((unsigned char *)trace, (unsigned int)ret) ;
	SetAppData("\x9F\x41", trace, 4) ;
	savevar((char * )&ret, TSC_OFF, TSC_LEN) ;
	return SUCC ;
}

