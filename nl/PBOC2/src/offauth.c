#include <string.h>
#include <stdlib.h>
#include "posapi.h"
#include "define.h"
#include "posdef.h"
#include "ic.h"
#include "capk.h"
#include "rsa.h"
#include "tools.h"
#include "tvrtsi.h"
#include "appdata.h"
#include "dolprocess.h"
#include "tlv.h"


//哈什算法标识
#define HASH_ALGORITHM	0x01

char m_isspk_modules[MAX_RSA_MODULUS_LEN];	//发卡行公钥模
char m_isspk_exp[3];						//发卡行公钥指数
int  m_isspk_modules_len;					//发卡行公钥模长

char m_icpk_modules[MAX_RSA_MODULUS_LEN];	//IC卡公钥模
char m_icpk_exp[3];							//IC卡公钥模指数
int  m_icpk_modules_len;					//IC卡公钥模长

char m_pinpk_modules[MAX_RSA_MODULUS_LEN];	//PIN公钥模
char m_pinpk_exp[3];						//PIN公钥模指数
int  m_pinpk_modules_len;					//PIN公钥模长

char m_RID[5];
char m_staticdata[MAX_RSA_MODULUS_LEN]; 

//SDA相关数据
typedef struct
{
	char 	index; 				//注册中心公钥索引
 	char 	*isscert;			//发卡行公钥证书
 	int	 	isscert_len;		//发卡行公钥证书长度
 	char 	*isspk_remainder;	//发卡行公钥余项
 	int		isspk_remainder_len;//发卡行公钥余项长度
 	char	*isspk_exp;			//发卡行公钥指数
 	int		isspk_exp_len;		//发卡行公钥指数长度
 	char	*signeddata;		//签名后的静态应用数据
 	int		signeddata_len;		//签名后的静态应用数据长度
 	char	*pan;				//IC卡主帐号
 	int		pan_len;			//主帐号长度
}TSDAData;


//DDA相关数据
typedef struct
{
	TSDAData sdadata;
 	char 	*iccert;			//IC卡公钥证书
 	int	 	iccert_len;			//IC卡公钥证书长度
 	char 	*icpk_remainder;	//IC卡公钥余项
 	int		icpk_remainder_len; //IC卡公钥余项长度
 	char	*icpk_exp;			//IC卡公钥指数
 	int		icpk_exp_len;		//IC卡公钥指数长度
}TDDAData;


/**
* @fn _JudgeExpdate
* @brief 判断RID, 公钥索引，证书序列号是否有效

* @param (in)	char RID[5]	 
* @param (in)	char index 公钥索引	 
* @param (in)	char serialnum[3]	证书序列号	 
* @return  成功: SUCC(有效)
* @return  失败: FAIL(无效)
*/
int _JudgeBlackCert(char RID[5], char index, char serialnum[3])
{
	int fp;
	int len;
	char curvalue[16];
	char buf[16];

	memset(curvalue, 0, sizeof(curvalue));
	memcpy(curvalue, RID, 5);
	memcpy(curvalue+5, &index, 1);
	memcpy(curvalue+6, serialnum, 3);
	
	//打开发卡行证书黑名单文件
	fp = fopen(BlackCertFile, "r");
	if(fp < 0)
	{
		return SUCC;
	}

	while(1)
	{
		len = fread(buf, sizeof(TBlackCert), fp);
		if(len != sizeof(TBlackCert))
		{
			break;			
		}

		if(memcmp(curvalue, buf, sizeof(TBlackCert))==0)
		{
			fclose(fp);
			return FAIL;
		}
	}
	
	fclose(fp);
	return SUCC;
}


/**
* @fn _JudgeExpdate
* @brief 判断是否超过有效期

* @param (in)	char *expdate	有效期
* @return  成功: SUCC(在有效期内)
* @return  失败: FAIL(不在有效期内)
*/
int _JudgeExpdate(char *expdate)
{
	char date[9];
	char time[7];
	char buf[9];
	int curday;
	int expday;
	char expiredate[5];

	memset(date, 0, sizeof(date));
	memset(time, 0, sizeof(time));
	GetTime(date, time);

	curday = 0;
	expday = 0;

	memset(buf, 0, sizeof(buf));
	memcpy(buf, date, 6);
	curday = atoi(buf);

	
	memset(expiredate, 0, sizeof(expiredate));
	BcdToAscii((unsigned char *)expiredate, (unsigned char *)expdate, 4, 0);
	memset(buf, 0, sizeof(buf));
	if(memcmp(expiredate+2, "50", 2)>0)	//如果年份大于50则认为是20世纪
	{
		memcpy(buf, "19", 2);
	}
	else
	{
		memcpy(buf, "20", 2);
	}
	
	memcpy(buf+2, expiredate+2, 2);
	memcpy(buf+4, expiredate, 2);
	expday = atoi(buf);

	if(curday <= expday)
	{
		return SUCC;
	}
	else
	{
		return FAIL;
	}
}


/**
* @fn _PreSDA
* @brief SDA(静态数据认证)预处理，获得进行SDA所需的数据

* @param (out)	TSDAData sdadata	SDA所需的相关数据
* @return  成功: SUCC
* @return  失败: FAIL
*/
int _PreSDA(TSDAData *sdadata)
{
	char *pdata;
	int len;

	len = 0;
	pdata = GetAppData("\x8f", &len);
	if(len !=1)
	{
		return FAIL;
	}
	sdadata->index = pdata[0];
	
	sdadata->isscert = GetAppData("\x90", &sdadata->isscert_len);
	if(sdadata->isscert == NULL)
	{
		return FAIL;
	}

	sdadata->isspk_remainder = GetAppData("\x92", &sdadata->isspk_remainder_len);

	sdadata->isspk_exp = GetAppData("\x9f\x32", &sdadata->isspk_exp_len);
	if(sdadata->isspk_exp == NULL)
	{
		return FAIL;
	}

	sdadata->pan = GetAppData("\x5a", &sdadata->pan_len);
	if(sdadata->pan == NULL)
	{
		return FAIL;
	}

	return SUCC;
}


/**
* @fn GetIssuerPK
* @brief 获得发卡行公钥

* @param (in)	char RID[5]			//应用提供者标识  		
* @param (in)	TSDAData *sdadata	//静态数据认证所需数据
* @param (out)	unsigned char *isspk_modules	//发卡行公钥模
* @param (out)	unsigned char *isspk_exp		//发卡行公钥指数
* @param (out)	int *isspk_len		//发卡行公钥指数模长
* @return  成功: SUCC
* @return  失败: FAIL
*/
int GetIssuerPK(char RID[5], TSDAData *sdadata, char *isspk_modules, char *isspk_exp, int *isspk_len)
{
	int ret;
	int i;
	TCAPK capk;
	char res[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	char digest[20];

	//以下定义为从发卡行公钥证书恢复出的数据元素
	char DataHeader;	//数据头
	char DataTrailer;	//数据尾
	char CertFormat;	//证书格式
	char IssIdNum[4];	//发卡行标识
	char ExpDate[2];	//证书有效期
	char SerialNum[3];	//证书序列号
	char HashAlg;		//哈什算法
	char IssPkAlg;		//发卡行公钥哈什算法
	char IssPkLen;		//发卡行公钥长度
	char IssPkExpLen;	//发卡行公钥指数长度
	char IssPkModules[MAX_RSA_MODULUS_LEN];	//发卡行公钥部分模
	char HashValue[20];		//哈什值
	
	//根据认证中心索引和RID读取CA公钥
	ret = CAPK_Load(RID, sdadata->index, &capk);

	//读取CA公钥失败，退出
	if(ret != CAPKERR_NONE)
	{
//		ERR_MSG("  指定的CA公钥\n    不存在");
		return FAIL;
	}

	//判断发卡行证书长度是否等于CA公钥长度
	if(sdadata->isscert_len != capk.Len)
	{
//		ERR_MSG("   发卡行证书\n    长度错误");
		return FAIL;
	}

	//恢复发卡行证书
	memset(res, 0, sizeof(res));
	ret = Recover((unsigned char *)sdadata->isscert, (unsigned char *)capk.Value, 
		(unsigned char *)capk.Exponent, capk.Len, (unsigned char *)res);
	if(ret != SUCC)
	{
//		ERR_MSG("   发卡行证书\n    恢复失败");
		return FAIL;
	}

	//分解发卡行证书数据
	DataHeader = res[0];
	CertFormat = res[1];
	memcpy(IssIdNum, res+2, 4);
	memcpy(ExpDate, res+6, 2);
	memcpy(SerialNum, res+8, 3);
	HashAlg = res[11];
	IssPkAlg = res[12];
	IssPkLen = res[13];
	IssPkExpLen = res[14];
	memcpy(IssPkModules, res+15, capk.Len-36);
	memcpy(HashValue, res + capk.Len-21, 20);
	DataTrailer = res[capk.Len-1];
	

	//检查证书格头是否合法
	if(DataHeader != 0x6a)	
	{
//		ERR_MSG("   发卡行证书\n   数据头错误");
		return FAIL;
	}

	//检查证书格式是否合法
	if(CertFormat != 0x02)	
	{
//		ERR_MSG("   发卡行证书\n    格式错误");
		return FAIL;
	}

	//检查证书尾是否合法
	if(DataTrailer != 0xbc)
	{
//		ERR_MSG("   发卡行证书\n   数据尾错误");
		return FAIL;
	}

	//检查发卡行公钥模余项是否存在
	if(IssPkLen > capk.Len - 36 && sdadata->isspk_remainder == NULL)
	{
		SetTVR(ICC_DATA_LOST, 1);
		return FAIL;
	}	

	//生成参与哈什计算的数据
	memset(buf, 0, sizeof(buf));
	memcpy(buf, res+1, capk.Len-22);
	memcpy(buf + capk.Len-22, sdadata->isspk_remainder,
		sdadata->isspk_remainder_len);
	memcpy(buf + capk.Len-22+sdadata->isspk_remainder_len,
		sdadata->isspk_exp, IssPkExpLen);

	//计算哈什结果
	memset(digest, 0, sizeof(digest));
	CalcMsgDigest(buf, capk.Len-22+sdadata->isspk_remainder_len+
		IssPkExpLen, digest);
	
	//比较哈什结果是否正确
	if(memcmp(digest, HashValue, 20) != 0)
	{
//		ERR_MSG(" 发卡行证书\n 哈什结果不正确");
		return FAIL;
	}

	//判断发卡行标识是否正确
	memset(buf, 0, sizeof(buf));
	BcdToAscii((unsigned char *)buf, (unsigned char *)IssIdNum, 8, 0);
	BcdToAscii((unsigned char *)buf+8, (unsigned char *)(sdadata->pan), 8, 0);
	for(i=0; i<8; i++)
	{
		if(buf[i]=='F')
		{
			break;
		}

		if(buf[i] != buf[8+i])
		{
//			ERR_MSG(" 发卡行标识错误");
			return FAIL;
		}
	}

	//判断证书是否失效
	ret = _JudgeExpdate(ExpDate);
	if(ret != SUCC)
	{
//		ERR_MSG("  证书已经过期");
		return FAIL;
	}
	

	//验证 rid, 公钥索引，证书序列号是否有效
	if(_JudgeBlackCert(RID, sdadata->index, SerialNum)!=SUCC)
	{
//		ERR_MSG("发卡行证书已撤销");
		return FAIL;
	}
		
	//判断公钥算法标识是否正确
	if(IssPkAlg != HASH_ALGORITHM)
	{
//		ERR_MSG("   发卡行公钥\n    算法错误");
		return FAIL;
	}	

	//生成发卡行公钥
	*isspk_len = IssPkLen;
	if(*isspk_len <= capk.Len-36)
	{
		memcpy(isspk_modules, IssPkModules, IssPkLen);
	}
	else
	{
		memcpy(isspk_modules, IssPkModules, capk.Len-36);
		memcpy(isspk_modules + capk.Len - 36, sdadata->isspk_remainder, sdadata->isspk_remainder_len);
	}
	
	if(IssPkExpLen == 3)
	{
		memcpy(isspk_exp, sdadata->isspk_exp, 3);
	}
	else
	{
		memset(isspk_exp, 0x00, 3);
		isspk_exp[2] = sdadata->isspk_exp[0];
	}

	return SUCC;
}


/**
* @fn GetIcPK
* @brief 获得IC卡公钥

* @param (in)	char RID[5]			//应用提供者标识  		
* @param (in)	TDDAData *ddadata	//动态数据认证所需数据
* @param (out)	unsigned char *icpk_modules	//IC卡公钥模
* @param (out)	unsigned char *icpk_exp		//IC卡公钥指数
* @param (out)	int *icpk_len		//IC卡公钥指数模长
* @return  成功: SUCC
* @return  失败: FAIL
*/
int GetIcPK(TDDAData *ddadata, char *staticdata, 
	char *isspk_modules, char *isspk_exp, int isspk_len,
	char *icpk_modules, char *icpk_exp, int *icpk_len)
{
	int ret;
	int i;
	int len;
	char res[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	char digest[20];
	char *pAip;
	char *pdata;

	//以下定义为从IC卡公钥证书恢复出的数据元素
	char DataHeader;	//数据头
	char DataTrailer;	//数据尾
	char CertFormat;	//证书格式
	char pan[10];		//发卡行标识
	char ExpDate[2];	//证书有效期
	char SerialNum[3];	//证书序列号
	char HashAlg;		//哈什算法
	char IcPkAlg;		//IC卡公钥哈什算法
	char IcPkLen;		//IC卡公钥长度
	char IcPkExpLen;	//IC卡公钥指数长度
	char IcPkModules[MAX_RSA_MODULUS_LEN];	//IC卡公钥部分模
	char HashValue[20];		//哈什值
	

	//判断IC卡公钥证书长度是否等于发卡行公钥模长度
	if(ddadata->iccert_len != isspk_len)
	{
//		ERR_MSG("   IC卡证书\n    长度错误");
		return FAIL;
	}

	//恢复IC卡证书
	memset(res, 0, sizeof(res));
	ret = Recover((unsigned char *)ddadata->iccert, (unsigned char *)isspk_modules, 
		(unsigned char *)isspk_exp, isspk_len, (unsigned char *)res);
	if(ret != SUCC)
	{
//		ERR_MSG("   IC卡证书\n    恢复失败");
		return FAIL;
	}

	//分解IC卡证书数据
	DataHeader = res[0];
	CertFormat = res[1];
	memcpy(pan, res+2, 10);
	memcpy(ExpDate, res+12, 2);
	memcpy(SerialNum, res+14, 3);
	HashAlg = res[17];
	IcPkAlg = res[18];
	IcPkLen = res[19];
	IcPkExpLen = res[20];
	memcpy(IcPkModules, res+21, isspk_len - 42);
	memcpy(HashValue, res + isspk_len-21, 20);
	DataTrailer = res[isspk_len-1];
	

	//检查证书格头是否合法
	if(DataHeader != 0x6a)	
	{
//		ERR_MSG("   IC卡证书\n   数据头错误");
		return FAIL;
	}

	//检查证书格式是否合法
	if(CertFormat != 0x04)	
	{
//		ERR_MSG("   IC卡证书\n    格式错误");
		return FAIL;
	}

	//检查证书尾是否合法
	if(DataTrailer != 0xbc)
	{
//		ERR_MSG("   发卡行证书\n   数据尾错误");
		return FAIL;
	}

	//检查IC卡公钥模余项是否存在
	if(IcPkLen > isspk_len - 42 && ddadata->icpk_remainder == NULL)
	{
		SetTVR(ICC_DATA_LOST, 1);
		return FAIL;
	}	

	//生成参与哈什计算的数据
	memset(buf, 0, sizeof(buf));
	memcpy(buf, res+1, isspk_len-22);
	len = isspk_len - 22;
	memcpy(buf + len, ddadata->icpk_remainder, ddadata->icpk_remainder_len);
	len = len + ddadata->icpk_remainder_len;
	memcpy(buf + len, ddadata->icpk_exp, IcPkExpLen);
	len = len + IcPkExpLen;
	memcpy(buf + len, staticdata+1, staticdata[0]);
	len = len + staticdata[0];

	pdata = GetAppData("\x9f\x4a", &i);
	if(pdata != NULL) 	//包含静态数据认证标签列表
	{
		if( i == 1 && pdata[0] == 0x82) //标签列表只包含82标签
		{
			pAip = GetAppData("\x82", &ret);
			memcpy(buf + len, pAip, 2);
			len = len + 2;
		}
		else
		{
//			ERR_MSG("  静态数据认证\n  标签列表错误");
			return FAIL;
		}
	}

	
	//计算哈什结果
	memset(digest, 0, sizeof(digest));
	CalcMsgDigest(buf, len, digest);
	
	//比较哈什结果是否正确
	if(memcmp(digest, HashValue, 20) != 0)
	{
//		ERR_MSG(" IC卡证书\n 哈什结果不正确");
		return FAIL;
	}

	//判断主帐号是否正确
	memset(buf, 0, sizeof(buf));
	BcdToAscii((unsigned char *)buf, (unsigned char *)pan, 20, 0);
	BcdToAscii((unsigned char *)buf+20, (unsigned char *)ddadata->sdadata.pan, 20, 0);
	for(i=0; i<20; i++)
	{
		if(buf[i]=='F')
		{
			break;
		}

		if(buf[i] != buf[20+i])
		{
//			ERR_MSG("   主帐号错误");
			return FAIL;
		}
	}

	//判断证书是否失效
	ret = _JudgeExpdate(ExpDate);
	if(ret != SUCC)
	{
//		ERR_MSG("  证书已经过期");
		return FAIL;
	}
	
	
	//判断公钥算法标识是否正确
	if(IcPkAlg != HASH_ALGORITHM)
	{
//		ERR_MSG("   发卡行公钥\n    算法错误");
		return FAIL;
	}	

	//生成发卡行公钥
	*icpk_len = IcPkLen;
	if(*icpk_len <= isspk_len - 42)
	{
		memcpy(icpk_modules, IcPkModules, IcPkLen);
	}
	else
	{
		memcpy(icpk_modules, IcPkModules, isspk_len-42);
		memcpy(icpk_modules + isspk_len-42, ddadata->icpk_remainder, ddadata->icpk_remainder_len);
	}
	
	if(IcPkExpLen == 3)
	{
		memcpy(icpk_exp, ddadata->icpk_exp, 3);
	}
	else
	{
		memset(icpk_exp, 0x00, 3);
		icpk_exp[2] = ddadata->icpk_exp[0];
	}

	return SUCC;
}


int VerifyStaticData(TSDAData *sdadata, char *staticdata, char *isspk_modules, char *isspk_exp, int isspk_len)
{
	int ret;
	int i;
	char res[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	char digest[20];
	char *pdata;
	char *pAip;
	int len;

	//以下定义为从签名的静态应用数据恢复出的数据元素
	char DataHeader;	//数据头
	char DataTrailer;	//数据尾
	char CertFormat;	//签名数据格式
	char HashAlg;		//哈什算法
	char DataAuthCode[2];	//数据认证代码
	char HashValue[20];	//哈什值
	
	//判断签名静态应用数据长度是否等于发卡行公钥模长度
	if(sdadata->signeddata_len != isspk_len)
	{
//		ERR_MSG("  签名静态应用\n  数据长度错误");
		return FAIL;
	}

	//恢复静态应用数据
	memset(res, 0, sizeof(res));
	ret = Recover((unsigned char *)sdadata->signeddata, (unsigned char *)isspk_modules, 
		(unsigned char *)isspk_exp, isspk_len, (unsigned char *)res);
	if(ret != SUCC)
	{
//		ERR_MSG("  静态应用数据\n    恢复失败");
		return FAIL;
	}

	//分解静态应用数据
	DataHeader = res[0];
	CertFormat = res[1];
	HashAlg = res[2];
	memcpy(DataAuthCode, res+3, 2);
	memcpy(HashValue, res + isspk_len-21, 20);
	DataTrailer = res[isspk_len-1];
	

	//检查数据头是否合法
	if(DataHeader != 0x6a)	
	{
//		ERR_MSG("  静态应用数据\n 恢复数据头错误");
		return FAIL;
	}

	//检查证书格式是否合法
	if(CertFormat != 0x03)	
	{
//		ERR_MSG("  静态应用数据\n  数据格式错误");
		return FAIL;
	}

	//检查证书尾是否合法
	if(DataTrailer != 0xbc)
	{
//		ERR_MSG("  静态应用数据\n  恢复数据尾错误");
		return FAIL;
	}

	//生成参与哈什计算的数据
	memset(buf, 0, sizeof(buf));
	memcpy(buf, res+1, isspk_len-22);
	memcpy(buf + isspk_len-22, staticdata+1, staticdata[0]); 
	
	len = isspk_len - 22 + staticdata[0];

	
	pdata = GetAppData("\x9f\x4a", &i);
	if(pdata != NULL) 	//包含静态数据认证标签列表
	{
		if( i == 1 && pdata[0] == 0x82) //标签列表只包含82标签
		{
			pAip = GetAppData("\x82", &ret);
			memcpy(buf + len, pAip, 2);
			len = len + 2;
		}
		else
		{
//			ERR_MSG("  静态数据认证\n  标签列表错误");
			return FAIL;
		}
	}
	
	//计算哈什结果
	memset(digest, 0, sizeof(digest));
	CalcMsgDigest(buf, len, digest);
	
	//比较哈什结果是否正确
	if(memcmp(digest, HashValue, 20) != 0)
	{
//		ERR_MSG(" 静态数据认证\n 哈什结果不正确");
		return FAIL;
	}

	ret = SetAppData("\x9f\x45", DataAuthCode, 2);
	if(ret < 0)
	{
//		ERR_MSG("  保存数据认证\n  代码失败!!");
		return FAIL;
	}
	else
	{
		return SUCC;
	}

}


/**
* @fn _PreDDA
* @brief DDA(动态数据认证)预处理，获得进行DDA所需的数据

* @param (out)	TSDAData ddadata	DDA所需的相关数据
* @return  成功: SUCC
* @return  失败: FAIL
*/
int _PreDDA(TDDAData *ddadata)
{
	int ret;
	
	ret = _PreSDA(&ddadata->sdadata);
	if(ret < 0)
	{
		return FAIL;
	}
	
	ddadata->iccert = GetAppData("\x9f\x46", &ddadata->iccert_len);
	if(ddadata->iccert == NULL)
	{
		return FAIL;
	}

	ddadata->icpk_remainder = GetAppData("\x9f\x48", &ddadata->icpk_remainder_len);

	ddadata->icpk_exp = GetAppData("\x9f\x47", &ddadata->icpk_exp_len);
	if(ddadata->icpk_exp == NULL)
	{
		return FAIL;
	}

	return SUCC;	
}


int VerifyDynamicData(char *icpk_modules, char *icpk_exp, int icpk_len)
{
	int ret;
	char res[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	char ddol[MAX_RSA_MODULUS_LEN];
	char digest[20];
	char *pdata;
	int len;
	char dynadata[MAX_RSA_MODULUS_LEN];
	int length;

	//以下定义为从签名的静态应用数据恢复出的数据元素
	char DataHeader;	//数据头
	char DataTrailer;	//数据尾
	char CertFormat;	//签名数据格式
	char HashAlg;		//HASH算法
	char DynaDataLen;	//动态数据长度
	char DynamicData[256]; //IC卡动态数据
	char HashValue[20];	//哈什值
	
	//如果IC卡上没有DDOL,使用终端缺省的DDOL
	pdata = GetAppData(DDOL, NULL);
	if(pdata == NULL)
	{
		SetAppData(DDOL, g_termparam.DefaultDDOL, g_termparam.DefaultDDOLen);
	}
	
	//取DDOL
	memset(ddol, 0, sizeof(ddol));
	ret = DOLProcess(DDOL, ddol);
	if(ret <0)
	{
//		ERR_MSG("DDOL不包含随机数");
		return FAIL;
	}

	//内部认证
	memset(buf, 0, sizeof(buf));
	length = IC_InternalAuthenticate(ddol+1, ddol[0], buf);
	if(length < 0)
	{
//		ERR_MSG("  内部认证失败");
		return QUIT;
	}

	TLV_Init(NULL) ;
	if (TLV_Decode(buf, length) != TLVERR_NONE)
	{
		return  QUIT;
	}	

	//取动态签名数据
	memset(dynadata, 0x00, sizeof(dynadata)) ;
	if (TLV_GetValue("\x80", dynadata, &len, 1) == TLVERR_TAGNOTFOUND)
	{
		if(TLV_GetValue("\x9f\x4b", dynadata, &len, 1) == TLVERR_TAGNOTFOUND)
		{
//			ERR_MSG(" 无动态应用数据\n");
			return FAIL;
		}
	}
	else
	{
		//判断80模板是否正确
		if(len + 2 != length)
		{
			return QUIT;
		}
	}
	
	//判断签名静态应用数据长度是否等于发卡行公钥模长度
	if(len != icpk_len)
	{
//		ERR_MSG("  签名动态应用\n  数据长度错误");
		return FAIL;
	}

	//恢复动态应用数据
	memset(res, 0, sizeof(res));
	ret = Recover((unsigned char *)dynadata, (unsigned char *)icpk_modules, 
		(unsigned char *)icpk_exp, icpk_len, (unsigned char *)res);
	if(ret != SUCC)
	{
//		ERR_MSG("  静态应用数据\n    恢复失败");
		return FAIL;
	}

	//分解动态应用数据
	DataHeader = res[0];
	CertFormat = res[1];
	HashAlg = res[2];
	DynaDataLen = res[3];
	memcpy(DynamicData, res+4, DynaDataLen);
	memcpy(HashValue, res + icpk_len-21, 20);
	DataTrailer = res[icpk_len-1];
	

	//检查数据头是否合法
	if(DataHeader != 0x6a)	
	{
//		ERR_MSG("  动态应用数据\n 恢复数据头错误");
		return FAIL;
	}

	//检查证书格式是否合法
	if(CertFormat != 0x05)	
	{
//		ERR_MSG("  动态应用数据\n  数据格式错误");
		return FAIL;
	}

	//检查证书尾是否合法
	if(DataTrailer != 0xbc)
	{
//		ERR_MSG("  动态应用数据\n  恢复数据尾错误");
		return FAIL;
	}

	//生成参与哈什计算的数据
	memset(buf, 0, sizeof(buf));
	memcpy(buf, res+1, icpk_len-22);
	memcpy(buf + icpk_len-22, ddol+1, ddol[0]); 
	
	len = icpk_len - 22 + ddol[0];

	//计算哈什结果
	memset(digest, 0, sizeof(digest));
	CalcMsgDigest(buf, len, digest);
	
	//比较哈什结果是否正确
	if(memcmp(digest, HashValue, 20) != 0)
	{
//		ERR_MSG(" 动态数据认证\n 哈什结果不正确");
		return FAIL;
	}

	ret = SetAppData("\x9f\x4C", DynamicData+1, DynamicData[0]);
	if(ret < 0)
	{
//		ERR_MSG("  保存IC动态\n  数字失败!!");
		return FAIL;
	}
	else
	{
		return SUCC;
	}

}



/**
* @fn GetPinPK
* @brief 获得PIN公钥

* @param (out)	unsigned char *pinpk_modules	//PIN公钥模
* @param (out)	unsigned char *pinpk_exp		//PIN公钥指数
* @param (out)	int *pinpk_len		//PIN公钥指数模长
* @return  成功: SUCC
* @return  失败: FAIL
* @return  证书不存在: QUIT
*/
int GetPinPK(char *isspk_modules, char *isspk_exp, int isspk_len,
	char *pinpk_modules, char *pinpk_exp, int *pinpk_len)
{
	int ret;
	int len;
	char res[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	char digest[20];

	//以下定义为从Pin卡公钥证书恢复出的数据元素
	char DataHeader;	//数据头
	char DataTrailer;	//数据尾
	char CertFormat;	//证书格式
	char pan[10];		//PIN标识
	char ExpDate[2];	//证书有效期
	char SerialNum[3];	//证书序列号
	char HashAlg;		//哈什算法
	char PinPkAlg;		//Pin卡公钥哈什算法
	char PinPkLen;		//Pin卡公钥长度
	char PinPkExpLen;	//Pin卡公钥指数长度
	char PinPkModules[MAX_RSA_MODULUS_LEN];	//Pin卡公钥部分模
	char HashValue[20];		//哈什值

	char *pPincert; 			//PIN证书指针
	int pincert_len;			//PIN证书长度
 	char *pPinpk_remainder;		//PIN公钥余项
 	int	 pinpk_remainder_len;	//PIN公钥余项长度
 	char *pPinpk_exp;			//PIN公钥指数
 	int	 pinpk_exp_len;			//PIN公钥指数长度

	pincert_len = 0;
	pinpk_exp_len = 0;
	pPincert = GetAppData("\x9f\x2d", &pincert_len);
	if(pPincert == NULL)
	{
		return QUIT;
	}

	pPinpk_remainder = GetAppData("\x9f\x2f", &pinpk_remainder_len);
	pPinpk_exp = GetAppData("\x9f\x2e", &pinpk_exp_len);
	if(pPinpk_exp == NULL)
	{
		return QUIT;
	}

	//判断Pin卡公钥证书长度是否等于PIN公钥模长度
	if(pincert_len != isspk_len)
	{
//		ERR_MSG("   Pin卡证书\n    长度错误");
		return QUIT;
	}

	//恢复Pin卡证书
	memset(res, 0, sizeof(res));
	ret = Recover((unsigned char *)pPincert, (unsigned char *)isspk_modules, 
		(unsigned char *)isspk_exp, isspk_len, (unsigned char *)res);
	if(ret != SUCC)
	{
//		ERR_MSG("   Pin卡证书\n    恢复失败");
		return FAIL;
	}

	//分解Pin卡证书数据
	DataHeader = res[0];
	CertFormat = res[1];
	memcpy(pan, res+2, 10);
	memcpy(ExpDate, res+12, 2);
	memcpy(SerialNum, res+14, 3);
	HashAlg = res[17];
	PinPkAlg = res[18];
	PinPkLen = res[19];
	PinPkExpLen = res[20];
	memcpy(PinPkModules, res+21, isspk_len - 42);
	memcpy(HashValue, res + isspk_len-21, 20);
	DataTrailer = res[isspk_len-1];
	

	//检查证书格头是否合法
	if(DataHeader != 0x6a)	
	{
//		ERR_MSG("   Pin卡证书\n   数据头错误");
		return FAIL;
	}

	//检查证书格式是否合法
	if(CertFormat != 0x04)	
	{
//		ERR_MSG("   Pin卡证书\n    格式错误");
		return FAIL;
	}

	//检查证书尾是否合法
	if(DataTrailer != 0xbc)
	{
//		ERR_MSG("   PIN证书\n   数据尾错误");
		return FAIL;
	}

	//生成参与哈什计算的数据
	memset(buf, 0, sizeof(buf));
	memcpy(buf, res+1, isspk_len-22);
	len = isspk_len - 22;
	memcpy(buf + len, pPinpk_remainder, pinpk_remainder_len);
	len = len + pinpk_remainder_len;
	memcpy(buf + len, pPinpk_exp, PinPkExpLen);
	len = len + PinPkExpLen;

	//计算哈什结果
	memset(digest, 0, sizeof(digest));
	CalcMsgDigest(buf, len, digest);

	//比较哈什结果是否正确
	if(memcmp(digest, HashValue, 20) != 0)
	{
//		ERR_MSG(" Pin证书\n 哈什结果不正确");
		return FAIL;
	}

	//判断证书是否失效
	ret = _JudgeExpdate(ExpDate);
	if(ret != SUCC)
	{
//		ERR_MSG("  证书已经过期");
		return FAIL;
	}
	
	
	//判断公钥算法标识是否正确
	if(PinPkAlg != HASH_ALGORITHM)
	{
//		ERR_MSG("   PIN公钥\n    算法错误");
		return FAIL;
	}	

	//生成PIN公钥
	*pinpk_len = PinPkLen;
	if(*pinpk_len <= isspk_len - 42)
	{
		memcpy(pinpk_modules, PinPkModules, PinPkLen);
	}
	else
	{
		memcpy(pinpk_modules, PinPkModules, isspk_len-42);
		memcpy(pinpk_modules + isspk_len-42, pPinpk_remainder, pinpk_remainder_len);
	}
	
	if(PinPkExpLen == 3)
	{
		memcpy(pinpk_exp, pPinpk_exp, 3);
	}
	else
	{
		memset(pinpk_exp, 0x00, 3);
		pinpk_exp[2] = pPinpk_exp[0];
	}

	return SUCC;
}



int EncryptPin(char *pin, char *encpin)
{
	int ret;
	int i;
	char ic_rand[8];
	char randnum[MAX_RSA_MODULUS_LEN];
	char buf[MAX_RSA_MODULUS_LEN];
	TDDAData ddadata;
	
	//获得PIN公钥
	ret = GetPinPK(m_isspk_modules, m_isspk_exp, m_isspk_modules_len, 
		m_pinpk_modules, m_pinpk_exp, &m_pinpk_modules_len);
	if( ret == FAIL )
	{
//ERR_MSG("pin 证书错误");
		return FAIL;
	}
	else if(ret == QUIT)
	{
//ERR_MSG("None PIN证书");
		if(m_icpk_modules_len == 0) //没有IC卡公钥，解出它
		{
			memset(&ddadata, 0, sizeof(ddadata));
			_PreDDA(&ddadata);
	
			//获得发卡行公钥
			memset(m_isspk_modules, 0x00, sizeof(m_isspk_modules));
			memset(m_isspk_exp, 0x00, sizeof(m_isspk_exp));
			m_isspk_modules_len = 0;
	
			ret = GetIssuerPK(m_RID, &(ddadata.sdadata), m_isspk_modules, m_isspk_exp, &m_isspk_modules_len);
			if(ret != SUCC)
			{
				return FAIL;
			}

			//获得IC卡行公钥
			memset(m_icpk_modules, 0x00, sizeof(m_icpk_modules));
			memset(m_icpk_exp, 0x00, sizeof(m_icpk_exp));
			m_icpk_modules_len = 0;
	
			ret = GetIcPK(&ddadata, m_staticdata, m_isspk_modules,
				m_isspk_exp, m_isspk_modules_len, m_icpk_modules, m_icpk_exp, &m_icpk_modules_len);
			if(ret != SUCC)
			{
				return FAIL;
			}
		}
		memcpy(m_pinpk_modules, m_icpk_modules, m_icpk_modules_len);
		memcpy(m_pinpk_exp, m_icpk_exp, 3);
		m_pinpk_modules_len = m_icpk_modules_len;
	}

	//生成IC卡随机数
	memset(ic_rand, 0, sizeof(ic_rand));
	ret = IC_GetChallenge(ic_rand);
	if(ret <0 )
	{
//ERR_MSG("取随机数错误");
//		return QUIT;  //原CASE要求退出交易
		return FAIL ;  //补充CASE CI032.00 不允许退出
	}

	//生成终端随机数
	memset(randnum, 0, sizeof(randnum));
	for (i=0 ; i<= (m_pinpk_modules_len-17) / 4 ; i++)
	{
		ret = rand();
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", ret);
		AsciiToBcd((unsigned char *)randnum+i*4, (unsigned char *)buf, 8, 0);
	}	

	memset(buf, 0, sizeof(buf));
	buf[0] = 0x7f;
	memcpy(buf + 1, pin, 8);
	memcpy(buf + 9, ic_rand, 8);
	memcpy(buf + 17, randnum, m_pinpk_modules_len-17);

	Recover((unsigned char * )buf, (unsigned char *)m_pinpk_modules,
		(unsigned char *)m_pinpk_exp, m_pinpk_modules_len, (unsigned char *)encpin);

	return m_pinpk_modules_len;
}


int OffAuth_SDA(char RID[5], char *staticdata)
{
	TSDAData sdadata;
//	char isspk_modules[MAX_RSA_MODULUS_LEN];	//发卡行公钥模
//	char isspk_exp[3];							//发卡行公钥指数
//	int isspk_len;								//发卡行公钥模长度
	int ret;

	//初始化进行静态数据认证所需的数据
	memset(&sdadata, 0, sizeof(sdadata));
	ret = _PreSDA(&sdadata);
	sdadata.signeddata = GetAppData("\x93", &sdadata.signeddata_len);
	if(ret != SUCC || sdadata.signeddata == NULL )
	{
		SetTVR(ICC_DATA_LOST, 1);
		SetTVR(OFFLINE_SDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
//		ERR_MSG("静态认证数据不全");
		return FAIL;
	}

	//获得发卡行公钥
	memset(m_isspk_modules, 0x00, sizeof(m_isspk_modules));
	memset(m_isspk_exp, 0x00, sizeof(m_isspk_exp));
	m_isspk_modules_len = 0;
	ret = GetIssuerPK(RID, &sdadata, m_isspk_modules, m_isspk_exp, &m_isspk_modules_len);
	if(ret != SUCC)
	{
		SetTVR(OFFLINE_SDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
		return FAIL;
	}

	//验证签名的静态应用数据
	ret = VerifyStaticData(&sdadata, staticdata, m_isspk_modules, m_isspk_exp, m_isspk_modules_len);
	if(ret != SUCC)
	{
		SetTVR(OFFLINE_SDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
		return FAIL;
	}

	SetTSI(OFFLINE_DA_COMPLETION, 1);
	return SUCC;
}


int OffAuth_DDA(char RID[5], char *staticdata)
{
	TDDAData ddadata;
//	char isspk_modules[MAX_RSA_MODULUS_LEN];	//发卡行公钥模
//	char isspk_exp[3];							//发卡行公钥指数
//	int isspk_len;								//发卡行公钥模长度
//	char icpk_modules[MAX_RSA_MODULUS_LEN];		//IC卡公钥模
//	char icpk_exp[3];							//IC卡公钥指数
//	int icpk_len;								//IC卡公钥模长度
	int ret;

	//初始化进行静态数据认证所需的数据
	memset(&ddadata, 0, sizeof(ddadata));
	ret = _PreDDA(&ddadata);
	if(ret != SUCC)
	{
		SetTVR(ICC_DATA_LOST, 1);
		SetTVR(OFFLINE_DDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
//		ERR_MSG("动态认证数据不全");
		return FAIL;
	}
	
	//获得发卡行公钥
	memset(m_isspk_modules, 0x00, sizeof(m_isspk_modules));
	memset(m_isspk_exp, 0x00, sizeof(m_isspk_exp));
	m_isspk_modules_len = 0;
	
	ret = GetIssuerPK(RID, &(ddadata.sdadata), m_isspk_modules, m_isspk_exp, &m_isspk_modules_len);
	if(ret != SUCC)
	{
		SetTVR(OFFLINE_DDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
		return FAIL;
	}

	//获得IC卡行公钥
	memset(m_icpk_modules, 0x00, sizeof(m_icpk_modules));
	memset(m_icpk_exp, 0x00, sizeof(m_icpk_exp));
	m_icpk_modules_len = 0;
	
	ret = GetIcPK(&ddadata, staticdata, m_isspk_modules,
		m_isspk_exp, m_isspk_modules_len, m_icpk_modules, m_icpk_exp, &m_icpk_modules_len);
	if(ret != SUCC)
	{
		SetTVR(OFFLINE_DDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
		return FAIL;
	}

	//验证动态签名数据
	ret = VerifyDynamicData(m_icpk_modules, m_icpk_exp, m_icpk_modules_len);
	if(ret != SUCC)
	{
		SetTVR(OFFLINE_DDA_FAIL, 1);
		SetTSI(OFFLINE_DA_COMPLETION, 1);
		return ret;
	}
		
	SetTSI(OFFLINE_DA_COMPLETION, 1);
	
	return SUCC;
}


int OffAuth(char RID[5], char *staticdata)
{
	char *pAIP;
	int len;

	memset(m_RID, 0, sizeof(m_RID));
	memset(m_staticdata, 0, sizeof(m_staticdata));
	memcpy(m_RID, RID, 5);
	memcpy(m_staticdata, staticdata, staticdata[0]+1);
	
	//从缓冲区中取AIP
	pAIP = GetAppData("\x82", &len);
	if(pAIP == NULL)
	{
//		ERR_MSG("   AIP不存在!");
		return FAIL;
	}

	if(pAIP[0]&0x02 && g_termconfig.TermCap[2]&0x10)	//CDA
	{
//		ERR_MSG("暂不支持CDA");
		return FAIL;
	}
	else if(pAIP[0]&0x20 && g_termconfig.TermCap[2]&0x40)		//DDA
	{
//		SUCC_MSG("DDA");
		if(OffAuth_DDA(RID, staticdata)==QUIT)
		{
			return QUIT;
		}
		else
		{
			return SUCC;
		}
	}
	else if(pAIP[0]&0x40 && (g_termconfig.TermCap[2]&0x80 || g_termconfig.TermCap[2]&0x40)) //SDA
	{
//		SUCC_MSG("SDA");	
		OffAuth_SDA(RID, staticdata);
		return SUCC;
		
	}
	else 			//无需脱机认证
	{
//		SUCC_MSG("NO OFFAUTH");	
		SetTVR(OFFLINE_AUTH_UNDO, 1);
		return SUCC;
	}
	
}

int CAPKLoad(char RID[5], char Index, TCAPK *CAPK)
{
	int fp;
	TCAPK TmpCAPK;
	unsigned char keycount = 0 ;
	int i = 0;

	if ((fp = fopen(FILE_CAPK, "r")) < 0)
		return FAIL ;
	fread((char *)&keycount, 1, fp) ;

	for (i = 0; i < keycount ; i++)
	{
		if (fread((char *)&TmpCAPK, sizeof(TCAPK), fp) != sizeof(TCAPK))
		{
			break ;
		}
		if (TmpCAPK.Index == Index)
		{
			memcpy((char *)CAPK, (char *)&TmpCAPK, sizeof(TCAPK));
			fclose(fp) ;
			return SUCC ;
		}
	}
	fclose(fp);
	return FAIL ;
}


