/**
* @file CardVerify.h
* @brief 持卡人验证头文件
  
* @version Version 2.0
* @author 叶明统
* @date 2005-08-05
*/

#ifndef _CARDHOLDER_VERIFY_
#define _CARDHOLDER_VERIFY_

#define SUCC_SIG 1      ///成功，但要签名
#define SUCC_ONLINE 2   ///成功，但要联机

#define OUTLIMIT -6 	//超限

void _PreCardVerify(TCVML * pCVML, int iAmount) ;
///持卡人验证函数
int CardVerify(const TCVML * pcvml, char * pPIN) ;

#endif