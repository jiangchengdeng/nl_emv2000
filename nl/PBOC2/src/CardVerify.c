/**
* @file CardVerify.c
* @brief 持卡人验证
  
* @version Version 2.0
* @author 叶明统
* @date 2005-08-05
*/

#include "posapi.h"
#include "posdef.h"
#include "tvrtsi.h"
#include "tools.h"
#include "ic.h"
#include "AppData.h"
#include "CardVerify.h"
#include "offauth.h"
#include "des.h"
#include <string.h>

void _PreCardVerify(TCVML * pCVML, int iAmount)
{
	char * p ;
	int len ;
	
	pCVML->pAIP = GetAppData("\x82", NULL);
	pCVML->pAppCurrCode = GetAppData("\x9f\x42", NULL);
	pCVML->pTransCurrCode = GetAppData("\x5f\x2a", NULL);
		
	if (p = GetAppData("\x9f\x62", NULL))
		pCVML->CardIDType = *p ;
	pCVML->CardID = GetAppData("\x9f\x61", &len);
	pCVML->CardIDLen = len ;
	pCVML->iAmount = iAmount;
	p = GetAppData("\x9C", NULL) ;
	pCVML->IsCash = (p[0] == 0x01?1:0) ;
	
	p = GetAppData("\x9f\x33", NULL) ;
	pCVML->cvmcap = p[1] ;

	if (p = GetAppData("\x8e", &len) )
	{
		C4ToInt((unsigned int *)&(pCVML->CVMx), (unsigned char *)p);	
		C4ToInt((unsigned int *)&(pCVML->CVMy), (unsigned char *)p+4);
		
		pCVML->CVRList = p+8;
		pCVML->nCVRCount = len - 8;
	}
	else
		pCVML->nCVRCount = 0 ;
}

/**
* @fn cvrconditon
* @brief CVR条件代码判断，由Card_Verify调用

* @param (in) const TCVML * pCVML 持卡人验证要用到的数据
* @param (in) int i 指示当前的CVM
* @return 0 不执行该CVR
          1 执行CVR
*/
int cvrconditon(const TCVML * pCVML, int i)
{
	char con = pCVML->CVRList[2 * i + 1] ;

	if (con >= 0x06)
	{
		if (pCVML->pAppCurrCode == NULL) //应用货币不存在
			return 0 ;
		if (memcmp(pCVML->pTransCurrCode, pCVML->pAppCurrCode, 2) != 0)
			return 0 ;
	}
	switch( con ) //CVM条件代码
	{
	case 0x00:				//总是执行
		return 1 ;
	case 0x01:				//如果是ATM现金
		return pCVML->IsCash  ;
	case 0x02:				//如果不是现金或返现
		return !pCVML->IsCash ;
	case 0x03:				//如果终端支持CVM
		switch (0x3f & pCVML->CVRList[i * 2]) // 当前的CVM
		{
		case 0x01:	//IC 卡明文PIN核对
			return pCVML->cvmcap & 0x80 ;
		case 0x02:	//联机加密PIN
			return pCVML->cvmcap & 0x40 ;
		case 0x03:	//IC 卡明文PIN + 签名
			return (pCVML->cvmcap & 0x80) && (pCVML->cvmcap & 0x20) ;
		case 0x04:
			return (pCVML->cvmcap & 0x10) && (pCVML->cvmcap & 0x20) ;
		case 0x05:
			return (pCVML->cvmcap & 0x10) ;
		case 0x1E:
			return pCVML->cvmcap & 0x20 ;
		case 0x1F: 	//无需CVM
			return pCVML->cvmcap & 0x08 ;
		case 0x20: 	//持卡人证件出示
		 	return pCVML->cvmcap & 0x01 ;
		default:
			return 0 ;//所有保留值将跳过,不处理
		}
	case 0x04: 				//如果是有人值守现金
		return pCVML->IsCash & 0x02 ;
	case 0x05:				//如果是返现
		return pCVML->IsCash & 0x04 ;
	case 0x06:				//如果交易以应用货币进行并且金额小于x 
		if (pCVML->iAmount < pCVML->CVMx)
			return 1 ;
		break ;
	case 0x07:				// 如果交易以应用货币进行并且金额大于x 
		if (pCVML->iAmount > pCVML->CVMx)
			return 1 ;
		break ;
	case 0x08:				// 如果交易以应用货币进行并且金额小于y 
		if (pCVML->iAmount < pCVML->CVMy)
			return 1 ;
		break ;
	case 0x09:				// 如果交易以应用货币进行并且金额大于y 
		if (pCVML->iAmount > pCVML->CVMy)
			return 1 ;
		break;
	default:
		break ;
	}
	return 0 ;
}

/**
* @fn pinwordcheck
* @brief 检查输入密码的有效性

* @param (in) char * pPin 用户输入的明文PIN
* @param (in) int len pPin的长度
* @return FAIL 无效
		  SUCC 有效
*/
int pinwordcheck(char *pin, int len)
{
	int i;

	for(i = 0; i < len ; i++ )
	{
		if( *(pin + i) < 0x30 || *(pin + i) > 0x39)	//无效的密码字符
			return FAIL;
	}
	return SUCC;
}

/**
* @fn verifypin
* @brief 封装PIN验证指令数据，并调用指令

* @param (in) char * pPin 用户输入的明文PIN
* @param (in) int style 密文1/明文0
* @return ICERR_xxx IC卡返回错误指令，包括超限，失败（无法认证），PIN错误
		  FAIL 严重错误,必须退出交易
*/
int verifypin( char * pin, int style )
{
	int pinlen, ret ;
	char bcdpin[7] ;
	char buff[256] ;
	char pinblock[256];

	pinlen = strlen( pin ) ;//4-12字节
	AsciiToBcd((unsigned char*) bcdpin, (unsigned char*)pin, pinlen, 0 ) ; //左对齐
	if( pinlen & 0x01 )
		bcdpin[pinlen / 2] |= 0x0f ; //奇数个，需要填充为1111
	memset( buff, 0xff, 256 ) ;
	buff[0] = 0x20 | (pinlen & 0x0f) ;
	memcpy( buff + 1, bcdpin, (pinlen + 1) / 2 ) ;

	if( style ) // 密文PIN
	{
		pinlen = EncryptPin(buff, pinblock);
		if(pinlen < 0)
		{
			return pinlen;
		}
	}
	else	//明文PIN
	{
		memcpy(pinblock, buff, 8);
		pinlen = 8 ; //明文PIN数据块长度
	}

	ret = IC_Verify(pinblock, pinlen, (TVerifyMode)style) ;//Verify PIN指令
	return ret ;
}

/**
* @fn pinprocess
* @brief 执行PIN验证

* @param (in) int style 验证方式，VERIFYMODE_NORMAL明文，VERIFYMODE_ENCRYPT密文
* return OUTLIMIT 超限，验证失败
		 QUIT	  未输入密码
		 EXIT	  密码键盘失效
		 SUCC	  验证通过
		 FAIL     出错
*/
int pinprocess( int style )
{
	int ret ;
	int retrycount ; //重试计数
	char resp[8] ;
	char pin[16] ;
	int maxretrycount;
	
	memset(resp, 0, 8) ;
	ret = IC_GetData(GETDATAMODE_PASSWORDRETRY, resp) ;
	if( (ret < 0) || (resp[0] != 0x9F && resp[1] != 0x17) || (resp[2] != 1) )
		retrycount = 3 ;//缺省值
	else
		retrycount = resp[3] ; //TLV格式

	if ( !retrycount )
	{
		return OUTLIMIT ;
	}
	maxretrycount = retrycount;
	while( retrycount ) //验证密码
	{
		clrscr();
		memset(pin,0,16);
		printf("请输入密码:\n");
		printf("(还有%02d次机会)\n", retrycount) ; 
		ret = getnumstr(pin,12,PASSWD,60);
		if(ret == QUIT )  // 退出,未输入 
		{
			if(retrycount == maxretrycount) //如果未输入过密码退出
			{
				return QUIT;
			}
			else
			{
				return -1000;
			}
		}
		if(ret == TIMEOUT)
			return ret ; //键盘不可用
		if(ret < 4)
		{
			ERR_MSG("  密码长度错误!\n  请重新输入...");
			continue;
		}
		if( pinwordcheck(pin, ret) == FAIL ) //非法密码字符
		{
			ERR_MSG("密码中有无效字符\n请重新输入...");
			continue;
		}

		pin[12]=0;
		ret = verifypin( pin, style ) ; //验证
		if ( ret == ICERR_NONE )
		{
			return SUCC; //验证成功
		}
		else
		{
			if (ret == QUIT) //在动态数据验证中, 取随机数失败
				return FAIL ;
			if (ret == FAIL)	//verify失败
				return -1000;
			IC_GetLastSW(resp, resp + 1) ;
			if ( (*resp == 0x69) && 
				( ( *(resp + 1) == 0x83) ||( *(resp + 1) == 0x84) ) )
			{
				return OUTLIMIT ;	//上次交易，PIN被锁住
			}
			else if( *resp == 0x63 )  //PIN重试超限
			{
				if ( (*( resp +1)  & 0x0F) == 0)
				{
					return OUTLIMIT; //超限
				}
				retrycount = *( resp +1)  & 0x0F ; //重试计数
				continue ;
			}
			else 
				return FAIL ;
		}
		retrycount -- ;
	}//End of " while( retrycount ) "
	return OUTLIMIT ; 
}

/**
* @fn Offline_PIN
* @brief 脱机PIN处理

* @param (in) char cvrtype CVR第一字节指定的脱机PIN验证类型
* @return 	-1 出错
			1 失败
		  	2 成功
*/
int OffLinePIN( char cvrtype )
{
	int ret ;

	if( cvrtype == 0x01 || cvrtype == 0x03 )
		ret = pinprocess(VERIFYMODE_NORMAL) ;
	else
		ret = pinprocess(VERIFYMODE_ENCRYPT) ;

	switch( ret )
	{
	case OUTLIMIT:  // 超限，验证失败
		SetTVR(PIN_RETRY_EXCEED, 1) ;
		return 1 ;
	case QUIT:	   //未输入密码
		SetTVR(REQ_PIN_NOT_INPUT, 1) ;
		return 1 ;
	case TIMEOUT:	   //密码键盘失效
		SetTVR(REQ_PIN_PAD_FAIL, 1) ;
		return 1 ;
	case SUCC:
		SetTVR(UNKNOW_CVM, 0) ;
	   	SetTVR(PIN_RETRY_EXCEED, 0) ;
		SetTVR(REQ_PIN_PAD_FAIL, 0) ;
		SetTVR(REQ_PIN_NOT_INPUT, 0) ;
		SetTVR(INPUT_ONLINE_PIN, 0) ;
		return 2 ;
	case FAIL:
		return -1 ;
	default:
		return 1 ;
	}
	return 1 ;
}
/**
* @fn Online_PIN
* @brief 联机PIN处理

* @param (out) char * pPin 返回用户输入的明文PIN
* @return 1 失败
		  4 成功但需要联机
*/

int OnLinePIN( char * pPIN ) 
{
	int ret ;
	char pin[16] ;
	int pinlen;

	while(1) 
	{
		clrscr();
		memset(pin,0,16);
		printf("请输入密码(联机):\n");
		ret = getnumstr(pin,12,PASSWD,60);
		if(ret == QUIT)  // 退出,未输入 
		{
			SetTVR(REQ_PIN_NOT_INPUT, 1) ;
			return 1;
		}
		if(ret == TIMEOUT)
		{
			SetTVR(REQ_PIN_PAD_FAIL, 1) ;
			return 1 ; //键盘不可用
		}
		if(ret < 4)
		{
			ERR_MSG("  密码长度错误!\n  请重新输入...");
			continue;
		}
		if( pinwordcheck(pin, ret) == FAIL ) //非法密码字符
		{
			ERR_MSG("密码中有无效字符\n请重新输入...");
			continue;
		}
		else
			break ;
	}
	pin[12] = 0 ;
	AsciiToBcd((unsigned char*)pPIN, (unsigned char*)pin, ret, 0);
	pinlen = ret;
	if( pinlen & 0x01 )
		pPIN[pinlen / 2] |= 0x0f ; //奇数个，需要填充为1111
	Des((uchar *)pPIN, (uchar *)pPIN, DESKEY);
	SetTVR(INPUT_ONLINE_PIN, 1) ; /* 联机PIN处理 */
	return 4 ;	
}

/**
* @fn processcvr
* @brief 处理一条CVM

* @param (in) const TCVML * pCVML 持卡人验证要用到的数据
* @param (in) int i 指示当前的CVM
* @param (out) char * pPin 返回用户输入的明文PIN
* @return -1 出错
		  0 未执行
		  1 失败
		  2 成功
		  3 成功，但要签名
		  4 成功，但要联机
*/
int processcvr(const TCVML * pCVML, int i, char *pPIN )
{
	unsigned char cvrcode = 0x3f & pCVML->CVRList[2 * i] ;
	int ret ;
	switch ( cvrcode ) //CVM代码
	{
	case 0x00:			//CVM处理失败
		return 1 ;
	case 0x01:			//明文PIN验证 
	case 0x03:			//明文PIN验证和签名（纸张）
	case 0x04:			//密文PIN验证 
	case 0x05:			//密文PIN验证和签名（纸张）
		if ((cvrcode <= 0x03 && (pCVML->cvmcap & 0x80)) ||
			( cvrcode >= 0x04 && ( pCVML->cvmcap & 0x10) ))
		{
			ret = OffLinePIN( cvrcode ) ;
			if (ret == 2) //脱机PIN成功
			{
				if (cvrcode == 0x03 || cvrcode == 0x05) //需要签名
				{
					if (pCVML->cvmcap & 0x20) //终端支持签名
						return 3 ;
					else
						return 1 ;
				}
				return 2 ;
			}
			else if (ret == -1) 
				return -1 ;
		}
		else
			SetTVR(REQ_PIN_PAD_FAIL, 1) ; //终端不支持脱机pin
		return 1 ;
	case 0x02:			//联机加密PIN验证
		if (pCVML->cvmcap & 0x40) 
			return OnLinePIN( pPIN ) ;
		else
			SetTVR(REQ_PIN_PAD_FAIL, 1) ;//终端不支持联机pin
		return 1 ;
	case 0x1E:			//签名 
		if (pCVML->cvmcap & 0x20) 
			return 3 ; 
		return 1 ; //终端不支持签名
	case 0x1F:			//无需CVM
		if (pCVML->cvmcap & 0x08)
			return 2 ;
		return 1 ;
	case 0x20:			//持卡人证件出示
		clrscr() ;
		printf("类型: ") ;
		switch(pCVML->CardIDType)
		{
		case 0x00:
			printf("身份证\n") ;
			break ;
		case 0x01:
			printf("军官证\n") ;
			break ;
		case 0x02:
			printf("护照\n") ;
			break ;
		case 0x03:
			printf("入境证\n") ;
			break ;
		case 0x04:
			printf("临时身份证\n") ;
			break ;
		case 0x05:
			printf(" 其他\n") ;
			break ;
		default:
			break;
		}
		printf("证件号: ") ;
		for (ret = 0 ; ret < pCVML->CardIDLen ; ret++)
			printf("%c", pCVML->CardID[ret]) ;
		printf("\n正确请按\"确认\" ") ;
		if (getkeycode(0) == ENTER)
			return 2 ;
		else
			return 1 ;
	default:				//系统统保留或不使用,或EMV保留，跳过
		SetTVR(UNKNOW_CVM, 1) ; //未知的CVM
		return 1 ;
	}//"switch( cvrcode )" ends here
}

/**
* @fn Card_Verify
* @brief 持卡人验证

* @param (in) const TCVML * pcvml 持卡人验证方法列表以及所需的数据
* @param (out) char * pPin 返回用户输入的明文PIN
* @return FAIL 出错，需中止
		  SUCC 成功完成
		  SUCC_SIG 成功，但要签名
		  SUCC_ONLINE 成功，但要联机
*/
int CardVerify (const TCVML *pcvml,  char *pPIN)
{
	int iCVRTotal ; 
	int nextflag ; //如果当前CVR失败是否应用后续的CVR，1是，0否
	int ret = 0 ;
	int i ;
	char cvmr[3] = {0x00,0x00,0x00} ;
	
	if ( !(pcvml->pAIP[0] & 0x10) ) // 卡片不支持持卡人验证
	{
		cvmr[0] = 0x3F ; //未执行CVM,可能要进行磁条卡CVM处理
		SetAppData("\x9F\x34", cvmr, 3) ;
		return SUCC;
	}
	if( pcvml->nCVRCount == 0 ) //没有CVM列表
	{
		cvmr[0] = 0x3F ;  
		SetAppData("\x9F\x34", cvmr, 3) ;
		SetTVR(ICC_DATA_LOST, 1) ; 
		SetTVR(CV_NOT_SUCCESS,1) ;
		return SUCC ;
	}
	if( pcvml->nCVRCount % 2 ) // 格式出错，失败
		return FAIL ;
	iNeedSignature = 0 ;
	iCVRTotal = pcvml->nCVRCount / 2 ; 
	//CVM逐条验证
	for (i = 0; i < iCVRTotal; i++) 
	{		
		//持卡人条件代码
		if( cvrconditon (pcvml, i) )
		{
			//cvmr的前两个字节置为cvm的方法代码和条件代码
			cvmr[0] = pcvml->CVRList[i*2] ;
			cvmr[1] = pcvml->CVRList[2*i+1] ;
			nextflag = pcvml->CVRList[i*2] & 0x40 ; //是否执行后序的CVM

			//执行当前CVM
			ret = processcvr(pcvml, i, pPIN) ;
			if( ret == 1 )       //当前失败
			{
				cvmr[2] = 0x01 ; //失败
				if( nextflag ) //下一条CVM从这里进入
					continue ;
			}
			else if(ret == 2) //成功
				cvmr[2] = 0x02  ;
			else	 //未知,如签名,或出错
			{
				cvmr[2] = 0x00 ;
				if (ret == 3)
					iNeedSignature = 1 ;
			}
			if( ret != 0 ) // 无论成功失败都从这里结束
				break ;
		}
		cvmr[0] = 0x3F ; //未执行
	}
	SetTSI(CV_COMPLETION, 1) ;		//设置TSI
	if ( cvmr[0] == 0x3f )			// 最后一条CVR也未执行
	{
		memcpy(cvmr,"\x3f\x00\x00",3);
	}
	SetAppData("\x9F\x34", cvmr, 3) ;
	if( ret >= 2 )			/* 成功 */
	{
		SetTVR(CV_NOT_SUCCESS, 0) ;
		return ret - 2 ;		/* 0 SUCC,1 SIGNATURE, 2 ONLINEPIN */ 
	}
	else if (ret >= 0) 
	{
		SetTVR(CV_NOT_SUCCESS, 1) ; /* 未成功 */
		return SUCC;
	}
	else
		return FAIL ;
}

