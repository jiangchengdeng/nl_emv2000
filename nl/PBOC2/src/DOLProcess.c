/**
 * @file DOLProcess.c
 * @brief 处理 DOL

 * @version Version 1.0
 * @author 叶明统
 * @date 2005-8-25
 */
#include <string.h>
#include "posapi.h"
#include "posdef.h"
#include "AppData.h"
#include "tvrtsi.h"
#include "DolProcess.h"
#include "rsa.h"


#define N 	0x01
#define CN 	0x02
#define O 	0x04
/**
 * @fn DOLProcess
 * @brief 对DOL(标签和长度列表)进行处理

 * @param (in) char * DOLStyle 要处理的DOL类型
 * @param (out) char * DOL_Value 返回DOL对应的数据元值
 * @return SUCC  成功
 		  FAIL    失败
 */
int DOLProcess(char * DOLStyle, char * DOL_Value)
{
	int len , nFormat, dollen ;
	char * ptr , *pData ;
	int i = 0 , offset = 0 ;
	char tmp[256] ;
	int flag9f37 = FALSE ;
	int iTVRPos = -1 ;
	int iTVRLenReq = 0 ;

	ptr = GetAppData(DOLStyle,  &len) ;
	if (!ptr)
	{
		if (*DOLStyle == 0x9F && *(DOLStyle + 1) == 0x38) //PDOL
		{
			DOL_Value[0] = 0x02 ;
			DOL_Value[1] = 0x83 ;
			DOL_Value[2] = 0x00 ;
			return SUCC ;
		}
		if (*DOLStyle == 0x8C) //CDOL1
			return FAIL ;			
	}

	memset(tmp, 0x00, 256) ;
	while (i < len)
	{
		//从全局缓冲区读数据
		pData = GetAppData (ptr + i, &dollen ) ; 
		if (*(ptr + i) & 0x20)
			pData =NULL ;

		//决定数据的格式是否为n，cn，others,及标签长度
		switch (*(ptr + i))
		{
		case 0x9F:
			i += 1 ; // 2 字节的标签
			switch (*(ptr + i))
			{
			case 0x02:
			case 0x03:
			case 0x15:
			case 0x11:
			case 0x1A:
			case 0x42:
			case 0x44:
			case 0x51:
			case 0x54:
			case 0x5C:
			case 0x73:
			case 0x75:
			case 0x57:
			case 0x76:
			case 0x39:
			case 0x35:
			case 0x3C:
			case 0x3D:
			case 0x41:
			case 0x21:
				nFormat = N ;
				break ;
			case 0x62:
			case 0x20:
				nFormat = CN ;
				break ;
			case 0x37:
				nFormat = O ;
				flag9f37 = TRUE ;
				break ;
			default:
				nFormat = O ;
				break ;
			}
			break ;
		case 0x5F:
			i += 1 ;
			switch (*(ptr + i))
			{
			case 0x25:
			case 0x24:
			case 0x34:
			case 0x28:
			case 0x2A:
			case 0x36:
				nFormat = N ;
				break ;
			default:
				nFormat = O ;
				break ;
			}
			break ;
		case 0x9A:
		case 0x9C:
			nFormat = N;
			break;
		case 0x5A:
			nFormat = CN ;
			break ;
		case 0x98:
			if (!HasAppData("\x97"))
			{
				if (g_termparam.DefaultTDOLen != 0)
				{
					SetTVR(USE_DEFAULT_TDOL, 1) ;
					if (offset >= 0)
						memcpy(tmp + iTVRPos, GetAppData("\x95", NULL), iTVRLenReq) ;
				}
			}
			nFormat = O ;
			break ;
		case 0x95:
			iTVRPos = offset ;
			iTVRLenReq = *(ptr + i + 1) > 5 ? 5 : *(ptr + i + 1) ;
		default:
			nFormat = O ;
			break ;
		}

		//数据不存在,OK, 补0x00
		if (pData == NULL ) 
		{ 
			memset(tmp + offset, 0x00, *(ptr + i +1)) ;
		}
		else 
		{
			//根据数据格式作相应的填充
			if (dollen < *(ptr + i + 1)) //如果数据元的实际长度小于要求的长度
			{
				if (nFormat & N)	//右对齐,左补0
					memcpy(tmp + offset + *(ptr + i + 1) - dollen, pData, dollen) ;
				else
					memcpy(tmp + offset, pData, dollen) ;
				if (nFormat & CN)  //左对齐,右补F
					memset(tmp + offset + dollen, 0xFF, *(ptr + i + 1) - dollen) ;
			}
			else //数据元的实际长度大于或等于要求的长度,需要截取
			{
				if (nFormat & N) //从最左端开始削减
					memcpy(tmp + offset, pData + dollen - *(ptr + i + 1), *(ptr + i + 1) ) ;
				else
					memcpy(tmp + offset, pData, *(ptr + i + 1)) ;
			}
		}
		offset += *(ptr + i + 1) ;
		i += 2 ;
	}

	//组装DOL的值
	if (*DOLStyle == 0x9F && *(DOLStyle + 1) == 0x38) //PDOL
	{
			DOL_Value[0] = offset + 2 ;
			DOL_Value[1] = 0x83 ;
			DOL_Value[2] = offset  ;
			memcpy(DOL_Value + 3, tmp, offset) ;
			return SUCC ;
	}
	if (*DOLStyle == 0x9F  && *(DOLStyle + 1) == 0x49) //DDOL
	{
		if (!flag9f37) //随机数必须有
			return FAIL ;
	}
	DOL_Value[0] = offset ;
	memcpy(DOL_Value + 1, tmp, offset) ;
	return SUCC ;
}
