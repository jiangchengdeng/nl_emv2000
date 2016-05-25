/**
*       @file       capk.c  
*       @brief      CAPK存取模块

*
*       @version    1.0
*       @autor      兰天 
*       @date       2005/07/27

*/


#include "capk.h"
#include "posapi.h"
#include <string.h>
#include <stdlib.h>


/*
@brief		初始化CAPK

@param		无
@return		CAPKERR_xxx
*/
int CAPK_Init(void)
{
	int fp;

	fdel(FILE_CAPK);
	fp = fopen(FILE_CAPK, "w");
	fclose(fp);
	if (fp < 0)
	{
		return(CAPKERR_INIT);
	}
	return(CAPKERR_NONE);
}


/*
@brief		读取CAPK

@param		(in)	char	RID[5]		注册的应用提供者标识(暂时不使用)
@param		(in)	char	Index		CAPK索引
@param		(out)	TCAPK	*CAPK		CAPK
@return		CAPKERR_xxx
*/
int CAPK_Load(char RID[5], char Index, TCAPK *CAPK)
{
	int fp;
	TCAPK TmpCAPK;

	fp = fopen(FILE_CAPK, "r");
	if (fp < 0)
	{
		return(CAPKERR_LOAD);
	}
	while(1)
	{
		if (fread((char *)&TmpCAPK, sizeof(TCAPK), fp) != sizeof(TCAPK))
		{
			fclose(fp);
			return(CAPKERR_LOAD);
		}
//		if (TmpCAPK.Index == Index && memcmp(RID, TmpCAPK.RID, 5) == 0 )
		if (TmpCAPK.Index == Index)
		{
			memcpy((char *)CAPK, (char *)&TmpCAPK, sizeof(TCAPK));
			break;
		}
	}
	fclose(fp);
	return(CAPKERR_NONE);
}


/*
@brief		保存CAPK

保存CAPK, 对已存在的(索引一致), 进行覆盖
@param		TCAPK	*iCAPK		CAPK
@return		CAPKERR_xxx
*/
int CAPK_Save(const TCAPK *CAPK)
{
	int fp;
	TCAPK TmpCAPK;

	fp = fopen(FILE_CAPK, "w");
	if (fp < 0)
	{
		return(CAPKERR_SAVE);
	}
	
	while(1)
	{
		if (ftell(fp) == filelength(fp))	//文件尾
		{
			break;
		}
		if (fread((char *)&TmpCAPK, sizeof(TCAPK), fp) != sizeof(TCAPK))
		{
			fclose(fp);
			return(CAPKERR_SAVE);
		}
		if (TmpCAPK.Index == CAPK->Index && memcmp(CAPK->RID, TmpCAPK.RID, 5) == 0 )	//找到则覆盖
		{
			if (fseek(fp, -sizeof(TCAPK), SEEK_CUR) != SUCC)
			{
				fclose(fp);
				return(CAPKERR_SAVE);
			}
			if (fwrite((char *)CAPK, sizeof(TCAPK), fp) != sizeof(TCAPK))
			{
				fclose(fp);
				return(CAPKERR_SAVE);
			} 
			else
			{
				fclose(fp);
				return(CAPKERR_NONE);
			}
		}
	}

	//新增
	if (fwrite((char *)CAPK, sizeof(TCAPK), fp) != sizeof(TCAPK))
	{
		fclose(fp);
		return(CAPKERR_SAVE);
	}
	fclose(fp);
	return(CAPKERR_NONE);
}



