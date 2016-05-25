/**
*       @file       tlv.c  
*       @brief      TLV解码模块

*
*       @version    1.0
*       @autor      兰天 
*       @date       2005/07/25

*/

#include <string.h>
#include "posapi.h"
#include "tlv.h"

#define		MAX_TAG_NUM			128		//Tag数
#define		VALUE_BUFFER_SIZE	2048	//Value缓冲区大小

//TLV元
typedef struct
{
	char	Tag[LEN_TAG];
	char	Restriction;
	int		Length;
	int		Offset;
}TTLVTuple;



//值缓冲
typedef struct
{
	char	Buffer[VALUE_BUFFER_SIZE];	//值
	int		Pos;						//在缓冲区中的偏移
}TValueBuffer;

static TTagRestriction	mTagRestriction[MAX_TAG_NUM];	//Tag约束
static TTLVTuple	mTLV[MAX_TAG_NUM];
static int			mTLVNum;			//TLV结构数(即标签数)
static TValueBuffer	mValueBuffer;		//值缓冲



/*
@brief		判断字节某位是否为1

@param		char	Byte	传入字节
@param		char	Bit		位(1-8)
@return		TRUE/FALSE
*/
int IsBitOn(char Byte, char Bit)
{
	if (((unsigned char)(Byte) >> (Bit-1)) % 2 == 0)
	{
		return(FALSE);
	} else
	{
		return(TRUE);
	}
}

/**
@brief		获得标签(Tag)

@param		(in)	char	*TLVStr		TLV串
@param		(in)	int		TLVStrLen	TLV串长
@param		(out)	TTag	pTag		Tag(输出)
@param		(out)	int		*TagLen		Tag串长
@return		TLVERR_xxx
*/
int TLV_GetTag(const char *TLVStr, int TLVStrLen, TTag pTag, int *TagLen)
{
	int Pos, i;
	unsigned char	FirstTagByte;

	FirstTagByte = (unsigned char)TLVStr[0];
	if ((FirstTagByte & 31) == 31 && FirstTagByte != 0xFF)	//有跟随字节
	{
		Pos = 1;
		while (1)
		{
			if (IsBitOn(TLVStr[Pos], 8) == TRUE)	//第8位为1则有后续字节
			{
				Pos++;
			} else
			{
				break;
			}
			if (Pos > LEN_TAG || Pos == TLVStrLen)	//已超出传入TLV串长
			{
				*TagLen = 0;
				return(TLVERR_INVALIDTAG);
			}
		}
		*TagLen = Pos+1;
		memcpy((char *)pTag, TLVStr, *TagLen);
		for (i=*TagLen; i<LEN_TAG; i++)
		{
			pTag[i] = 0;
		}
	} else
	{
		pTag[0] = TLVStr[0];
		*TagLen = 1;
		for (i=*TagLen; i<LEN_TAG; i++)
		{
			pTag[i] = 0;
		}
	}
	return(TLVERR_NONE);
}

/**
@brief		获得长度(Length)

@param		char	*TLVStr		TLV串
@param		int		TLVStrLen	TLV串长
@param		int		*Len		长度串(输出)
@param		int		*LenLen		长度串长度(输出)
@return		TLVERR_xxx
*/
int TLV_GetLength(const char *TLVStr, int TLVStrLen, int *Len, int *LenLen)
{
	int i, ByteNum;
	
	*Len = ((unsigned char)(TLVStr[0])) & 0x7F;
	if (IsBitOn(TLVStr[0], 8) == TRUE)	//第8位为1,则1-7位表示跟随字节数
	{
		ByteNum = *Len;
		if (ByteNum > 3)
		{
			*Len = 0;
			*LenLen = 0;
			return(TLVERR_INVALIDLENGTH);
		}
		*Len = 0;
		for (i=1; i<=ByteNum; i++)
		{
			*Len = *Len + (unsigned int)TLVStr[i] << ((ByteNum-i)*8);
		}
		*LenLen = ByteNum+1;
	} else
	{
		*LenLen = 1;
	}
	return(TLVERR_NONE);
}

/*
@brief		判断是否为嵌套TLV

Tag第6位为1则为嵌套TLV
@param		TTag	Tag			Tag
@return		TRUE/FALSE
*/
int NestingTLV(TTag Tag)
{
	return(IsBitOn(Tag[0], 6));
}

/*
@brief		检查是否满足约束

mTagRestrict约束了Tag的最小长度和最大长度, 
当mTagRestrict[0].Tag == NULL时表示对所有Tag都不做约束,
当mTagRestrict[x].MaxLen == 0时表示对该Tag不做约束
@param		TTag	Tag			Tag(T)
@param		int		Len			Length(L)
@return		TRUE/FALSE
*/
int Restrict(TTag Tag, int Len)
{
	int i;

	if (mTagRestriction[0].Tag[0] == NULL)
	{
		return(TRUE);
	}
	i = 0;
	while(1)
	{
		if (mTagRestriction[i].Tag[0] == NULL)
		{
			break;
		}
		if (memcmp(Tag, mTagRestriction[i].Tag, LEN_TAG) == 0)
		{
			if (mTagRestriction[i].MaxLen == 0)
			{
				return(TRUE);
			}
			if ((Len >= mTagRestriction[i].MinLen) && (Len <= mTagRestriction[i].MaxLen))
			{
				return(TRUE);
			} else
			{
				return(FALSE);
			}
		}
		i++;
	}
	return(TRUE);
}

/**
@brief		TLV模块初始化

TR.Tag为全0时为约束集的结束标志
@param		TTagRestriction		TR[]	Tag约束集
@return		TLVERR_xxx
*/
int TLV_Init(const TTagRestriction TR[])
{
	int i, j;
	
	memset((char *)mTLV, 0, sizeof(mTLV));
	mTLVNum = 0;
	memset(mValueBuffer.Buffer, 0, sizeof(mValueBuffer.Buffer));
	mValueBuffer.Pos = 0;
	i = 0;
	
	if(TR != NULL)
	{
		while(1)
		{
			if (TR[i++].Tag[0] == NULL)
			{
				break;
			}
		}
	}
	
	if (i > MAX_TAG_NUM)
	{
		return(TLVERR_INVALIDTAGRESTRICTION);
	}
	memset((char *)mTagRestriction, 0, sizeof(mTagRestriction));
	for (j=0; j<i; j++)
	{
		memcpy(mTagRestriction[j].Tag, TR[j].Tag, LEN_TAG);
		mTagRestriction[j].MinLen = TR[j].MinLen;
		mTagRestriction[j].MaxLen = TR[j].MaxLen;
		//memcpy((char *)(mTagRestriction[j]), (char *)(TR[j]), sizeof(TTagRestriction));
	}
	return(TLVERR_NONE);
}

/**
@brief		TLV解码

@param		char	*TLVStr		TLV串
@param		int		TLVStrLen	TLV串长
@return		TLVERR_xxx
*/
int TLV_Decode(char *TLVStr, int TLVStrLen)
{
	TTag	Tag;
	int		TagLen, ret, Len, LenLen, TLVRemainLen, DecodeByte;
	char	*TLVStr0;
	char    *TLVStrTemplate;

//	memset(mValueBuffer.Buffer, 0, sizeof(mValueBuffer.Buffer));
//	mValueBuffer.Pos = 0;
//	mTLVNum = 0;

	//TLV串长度为0时，不进行解码
	if(TLVStrLen <=0 )
	{
		return(TLVERR_NONE);
	}
	
	TLVStr0 = TLVStr;
	TLVRemainLen = TLVStrLen;
	DecodeByte = 0;
/*
clrscr();
printf("StrLen = %d", TLVStrLen);
getkeycode(0);
*/
	while (1)
	{
//Disp_deg_buf(TLVStr, TLVRemainLen);
		ret = TLV_GetTag(TLVStr, TLVRemainLen, Tag, &TagLen);	//取得Tag
		if (ret != TLVERR_NONE)
		{
			TLVStr = TLVStr0;
			return(ret);
		}
		TLVRemainLen -= TagLen;
		DecodeByte += TagLen;
		if (Tag[0] == 0x00 || Tag[0] == 0xFF)
		{
			if (TLVRemainLen == 0) 
			{
				break;
			}
			TLVStr++;
			continue;	
		}
/*
clrscr();
printf("RemainLen=%d\n", TLVRemainLen);
printf("DecodeByte=%d\n", DecodeByte);
printf("Tag = %02x%02x", Tag[0], Tag[1]); 
getkeycode(0);
*/
		ret = TLV_GetLength(TLVStr+TagLen, TLVRemainLen, &Len, &LenLen);	//取得Len
		if (ret != TLVERR_NONE)
		{
			TLVStr = TLVStr0;
			return(ret);
		}
		TLVRemainLen -= LenLen;
		DecodeByte += LenLen;
/*
clrscr();
printf("len = %d\n", Len); 
printf("RemainLen=%d\n", TLVRemainLen);
printf("DecodeByte=%d\n", DecodeByte);
getkeycode(0);
*/
		mTLV[mTLVNum].Restriction = Restrict(Tag, Len);

		memcpy(mTLV[mTLVNum].Tag, Tag, LEN_TAG);
		mTLV[mTLVNum].Length = Len;
		if (NestingTLV(Tag) == TRUE)	//嵌套TLV
		{
			/*
			对于模板, Offset, Len的含义为输入串的偏移和长度
			该值可通过TLV_GetTemplate取值
			*/
			TLVRemainLen -= Len;
			DecodeByte += Len;
			mTLV[mTLVNum].Offset = TLVStr-TLVStr0+TagLen+LenLen;
			mTLV[mTLVNum++].Length = Len;
			TLVStr += TagLen+LenLen;
			if (TLVStr+Len-TLVStr0 > TLVStrLen)//越界
			{
				TLVStr = TLVStr0;
				return(TLVERR_DECODEFAIL);
			}
			TLVStrTemplate = TLVStr;
			ret = TLV_Decode(TLVStrTemplate, Len);
			if(ret != TLVERR_NONE)
			{
				return TLVERR_DECODEFAIL;
			}
			TLVStr += Len;
		}
		else
		{
				TLVRemainLen -= Len;
				DecodeByte += Len;
/*
clrscr();
printf("Value\n"); 
printf("TLVRemainLen=%d\n", TLVRemainLen);
printf("DecodeByte=%d\n", DecodeByte);
getkeycode(0);
*/
				if (TLVRemainLen < 0 || DecodeByte > TLVStrLen)	//越界
				{
					TLVStr = TLVStr0;
					return(TLVERR_DECODEFAIL);
				}
				//取得Value
				memcpy(mValueBuffer.Buffer+mValueBuffer.Pos, TLVStr+TagLen+LenLen, Len);
				mTLV[mTLVNum].Offset = mValueBuffer.Pos;
				mValueBuffer.Pos += Len;
				mTLVNum++;
				TLVStr += TagLen+LenLen+Len;
		}
		if (TLVRemainLen == 0)	//已解完
		{
			break;
		}
	}
	TLVStr = TLVStr0;
	
	if (DecodeByte != TLVStrLen)
	{
		return(TLVERR_DECODEFAIL);
	}
	
	return(TLVERR_NONE);
}

/**
@brief		取得值(Value)

@param		TTag	Tag			标签(Tag)
@param		char	*Value		值(Value)(输出)
@param		int		*Len		值长度(输出)
@param		int		Offset		匹配第Offset个相同标签
@return		TLVERR_xxx
*/
int TLV_GetValue(TTag Tag, char *Value, int *Len, int Offset)
{
	int i, n;
	TTag Tag0;

	n = 0;
	if (Offset <=0)
	{
		Offset = 1;
	}
	memset((char *)Tag0, 0, sizeof(Tag0));
	memcpy(Tag0, Tag, strlen(Tag));
	
	for (i=0; i<mTLVNum; i++)
	{
		if (memcmp((char *)Tag0, (char *)mTLV[i].Tag, LEN_TAG) == 0)
		{
			n++;
			if (n != Offset)
			{
				continue;
			}
			memcpy(Value, mValueBuffer.Buffer+mTLV[i].Offset, mTLV[i].Length);
			*Len = mTLV[i].Length;

			if (mTLV[i].Restriction == TRUE)
			{
				return(TLVERR_NONE);
			} else
			{
				return(TLVERR_INVALIDTAG);
			}
			
		}
	}
	return(TLVERR_TAGNOTFOUND);
}


/**
@brief		取得模板的值(Value)

@param		TTag	Tag			标签(模板Tag)
@param		int		*Offset		该模板的值在原TLV串的偏移
@param		int		*Len		值长度(输出)
@return		TLVERR_xxx
*/
int TLV_GetTemplate(TTag Tag, int *Offset, int *Len)
{
	int i;
	TTag Tag0;

	memset((char *)Tag0, 0, sizeof(Tag0));
	memcpy(Tag0, Tag, strlen(Tag));
	
	if (NestingTLV(Tag0) == FALSE)
	{
		return(TLVERR_TAGNOTFOUND);
	}
	for (i=0; i<mTLVNum; i++)
	{
		if ((mTLV[i].Restriction == TRUE) && (memcmp((char *)Tag0, (char *)mTLV[i].Tag, LEN_TAG) == 0))
		{
			*Offset = mTLV[i].Offset;
			*Len = mTLV[i].Length;
			return(TLVERR_NONE);
		}
	}
	return(TLVERR_TAGNOTFOUND);
}



/**
@brief		取得标签列表

@param		TTagList	TagList		标签列表(输出)
@param		int			*TagNum		标签数
@return		无
*/
void TLV_GetTagList(TTagList TagList[], int *TagNum)
{
	int i;
	
	for (i=0; i<mTLVNum; i++)
	{
		memcpy((char *)TagList[i].Tag, mTLV[i].Tag, LEN_TAG);
		TagList[i].Restriction = mTLV[i].Restriction;
		TagList[i].len = mTLV[i].Length;
		TagList[i].offset = mTLV[i].Offset;
	}
	*TagNum = mTLVNum;
}


