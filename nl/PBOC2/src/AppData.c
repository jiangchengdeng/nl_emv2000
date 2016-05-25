/**
 * @file AppData.c
 * @brief 定义数据存取方式

       为了方便模块之间的数据交换(大量), 有必要建立一个全局的缓冲区,
   并提供访问函数,以求最大的灵活性,并独立于各个模块的函数入口/出口,
   方便定制不同的数据访问模式----面向数据
       本模块提供数据的初始化,读取,写入,删除功能(针对全局)
 * @version Version 1.0
 * @author 叶明统
 * @date 2005-8-31
 */

#include <string.h>
#include "posapi.h"
#include "AppData.h"

#ifndef MAXLEN
#define MAXLEN 4028		//全局缓冲区的最大容量
#endif 

#ifndef MAXTAG
#define MAXTAG 255 		//最多可以存放的标签数
#endif

static char 		_AppData[MAXLEN] ;			//全局缓冲区

static int		_DataManage[MAXTAG + 2] ;	//缓冲区管理数组


/**
 * @fn void InitAppData (void)
 * @brief 初始化全局缓冲区为全0x00

 * @return 无
 */
void InitAppData (void)
{
	memset (_AppData, 0x00, MAXLEN) ;
	memset (_DataManage, 0x00, MAXTAG) ;
}

/**
 * @fn int HashAppData( char *)
 * @brief  判断是否存在一个标签tag

 * @param char * tag 要判断的标签
 * @return int	@li  TRUE 存在
 			    @li  FALSE 不存在
 */			     	
int HasAppData ( char * tag )
{
	int i ;
	int nID = 0 ;
	
	nID = * ( (unsigned char *)tag) ;
	if ( (*tag & 0x1F) == 0x1f ) //有跟随字节
	{
		nID = (nID << 8) | * ( (unsigned char *)(tag + 1)) ;
	}
	if (!nID)
		return FALSE;
	for (i = 1 ; i <= _DataManage[0] ; i++) 
	{
		if (nID == (_DataManage[i] & 0x0000ffff))
		{
			return i ;
		}
	}
	return FALSE ;	
}

/**
 * @fn char  * GetAppData(unsigned char  *, int *)
 * @brief 从全局缓冲区读数据
 
 * @param char  * tag 	要读取数据的标签,如"\x9F\x07"
 * @param int  * length 	读出的数据的长度
 * @return char  * 指向数据在缓冲区中的位置 
 			 NULL 没有找到要求的数据
 */
char * GetAppData (char * tag, int * length)
{
	int i ;
	
	if ( (i = HasAppData(tag)) == FALSE)
	{
		return NULL ;
	}

	if( length != NULL)
	{
		*length =  (_DataManage[i + 1] >> 16) - (_DataManage[i] >> 16) ;
	}
	return  _AppData + (_DataManage[i] >> 16) ;
	
}

/**
 */
void DeleteAppData (int nIndex)
{
	int offset , len1, len2 ;
	
	offset = _DataManage[nIndex] >> 16 ; //要删除的起始地址
	len1 =  (_DataManage[nIndex + 1] >> 16) - offset ;//要删除的长度
	len2 =  (_DataManage[_DataManage[0] + 1] >> 16) - offset - len1 ;//

	memcpy (_AppData + offset, _AppData + offset + len1, len2) ;
	len1 <<= 16 ;
	for (offset = nIndex + 1 ; offset <= _DataManage[0] +1 ; offset++)
	{
		_DataManage[offset] -= len1 ;
		_DataManage[offset - 1] = _DataManage[offset] ;
	}
	_DataManage[0]-- ;
}
/**
 * @fn int SetAppData(unsigned char  *, unsigned char  * content, int length)
 * @brief 向全局缓冲区写数据

 * @param char  * tag 要写入的数据的标签
 * @param char  * content 写入的数据
 * @param int	   length 写入数据的长度
 * @return	SUCC	成功
 			ADERR_INVALIDTAG 无效的标签
 			ADERR_DATADUP	 标签已经存在
 			ADERR_TAGOVER	 标签数已满
 			ADERR_LENOVER	 缓冲区满
 */
int SetAppData (char  * tag, char  * content, int length)
{
	int i = 0 ;
	int nID = 0 ;
	int len, offset ;
	
	nID = * ( (unsigned char *)tag) ;
	if ( (*tag & 0x1F) == 0x1f ) //有跟随字节
	{
		nID = (nID << 8) | * ( (unsigned char *)(tag+1)) ;
	}
	if (!nID)
		return ADERR_INVALIDTAG ;	
	for (i = 1 ; i <= _DataManage[0] ; i++) 
	{
		if (nID == (_DataManage[i] & 0x0000ffff))
		{
			offset = _DataManage[i ] >> 16 ;
			len =  (_DataManage[i + 1] >> 16) - offset ;
			if (len != length)
			{
				DeleteAppData(i) ;
				break ;
			}
			memcpy(_AppData + offset, content, length) ;
			return SUCC ;
		}
	}
	
	if (_DataManage[0] == MAXTAG) //标签缓冲区满
		return ADERR_TAGOVER ;
	offset = _DataManage[_DataManage[0] + 1] >> 16 ; //下一个可以存放的位置
	len =MAXLEN - offset ; //下一个可以存放的长度
	if (len <length) //长度不够
		return ADERR_LENOVER ;
	
	memcpy (_AppData +offset, content, length) ; //写入缓冲区
	_DataManage[0] ++ ; //增加标签数
	_DataManage[_DataManage[0]] = ((offset << 16) | nID) ; //保存当前标签的信息
	_DataManage[_DataManage[0] + 1] = ((offset + length) << 16) ;//记录再下一个可用的位置
	return SUCC ;
}

/**
 * @fn int DelAppData(unsigned char  * tag)
 * @从缓冲区中删除指定标签的值

 * @param char  * tag 要删除的标签
 * @return SUCC
 		   ADERR_NOTFOUND 不存在该标签
 */
int DelAppData (char  * tag)
{
	int i = 0 ;
	
	if ( (i = HasAppData(tag)) == FALSE)
	{
		return ADERR_NOTFOUND ;
	}
	
	DeleteAppData(i) ;
	return SUCC ;
}

void GetTags(int * tmp, int * num)
{
	memcpy( tmp, _DataManage + 1, _DataManage[0] * 4) ;
	*num = _DataManage[0] ;
}