#ifndef _CAPK_H_
#define _CAPK_H_

#define	FILE_CAPK		"capkfile.dat"

typedef struct
{
	char RID[5];				//应用提供者标识
	char Index;					//认证中心公钥索引
	char HashAlgorithm;			//认证中心HASH算法标识(目前固定为0x01)
	char PkAlgorithm;			//公钥算法标识(目前固定为0x01)
	char Value[248];			//公钥模			
	char Len;					//公钥模长度
	unsigned char Exponent[3];	//认证中心公钥指数
	char Hash[20];				//认证中心公钥验证值
}TCAPK;


#define		CAPKERR_BASE		(0)
#define		CAPKERR_NONE		(CAPKERR_BASE)
#define		CAPKERR_INIT		(CAPKERR_BASE-1)
#define		CAPKERR_LOAD		(CAPKERR_BASE-2)
#define		CAPKERR_SAVE		(CAPKERR_BASE-3)

extern int CAPK_Init(void);
extern int CAPK_Load(char RID[5], char Index, TCAPK *CAPK);
extern int CAPK_Save(const TCAPK *CAPK);

#endif
