#include <stdlib.h>
#include <string.h>
#include "posapi.h"
#include "define.h"
#include "posdef.h"
#include "ic.h"
#include "tlv.h"

extern TTermConfig g_termconfig;		//终端配置
extern TTermParam  g_termparam;		//终端参数

#define			LEN_MAXAPDUSTR		384
#define			LEN_MAXICRECEIVE	512
typedef char	TAPDUStr[LEN_MAXAPDUSTR];
typedef char	TICReceive[LEN_MAXICRECEIVE];
typedef enum
{
	ICCMD_SELECT,
	ICCMD_READRECORD,
	ICCMD_EXTERNALAUTH,
	ICCMD_INTERNALAUTH,
	ICCMD_GETPROCESSOPTIONS,
	ICCMD_OTHERS
}TICCommand;
static char	ICPort;
static char	mSW1, mSW2;

static char	*SWMsg[]=
{
"6200 No information given",
"6281 Part of returned data may be corrupted",
"6283 Selected file invalidated",
"6284 FCI not formatted according to P2",
"6300 Authentication failed",
"63Cx Counter provided by 'x' (valued from 0 to 15)",
"6400 State of nonvolatile memory unchanged",
"6500 No information given",
"6581 Memory failure",
"6700 Lc inconsistent with P1 P2",
"6800 No information given",
"6881 Logical channel not supported",
"6882 Secure messaging not supported",
"6900 No information given",
"6981 Command incompatible with file organisation",
"6982 Security status not satisfied",
"6983 Authentication method blocked",
"6984 Reference data invalidated",
"6985 Conditions of use not satisfied",
"6986 Command not allowed",
"6987 Secure messaging data object missing",
"6988 Secure messaging data object incorrect",
"6A00 No information given",
"6A80 Incorrect parameters in the data field",
"6A81 Function not supported",
"6A82 File not found",
"6A83 Record not found",
"6A84 Not enough memory space in file",
"6A85 Lc inconsistent with TLV structure",
"6A86 Incorrect parameters P1 P2",
"6A87 Lc inconsistent with P1 P2",
"6A88 Referenced data not found",
"6D00 Instruction code not supported or invalid",
"6E00 Class not supported",
"6F00 No precise diagnostics",
""
};

void ShowSWMessage(char SW1, char SW2)
{
	char SW[5];
	int i;

	sprintf(SW, "%02X%02X", SW1, SW2);
	if ((SW1 == 63) && (SW2 >> 4 == 0x0C))
	{
		memcpy(SW, "63Cx", 4);
		SW[4] = 0;
	}

	i = 0;
	while(1)
	{
		if (memcmp(SW, SWMsg[i], 4) == 0)
		{
			clrscr();
			printf("%02X%02X:\n", SW1, SW2);
			printf(SWMsg[i]+5);
			getkeycode(0);
		}
		i++;
		if (strlen(SWMsg[i]) == 0)
		{
			break;
		}
	}
}


/**
@brief		设定IC卡操作端口

@param		(in)	TICPort	Port		端口
@return		无
*/
void IC_SetPort(TICPort Port)
{
	ICPort = Port;
}

/*
@brief		将IC指令打包成APDU串

@param		(in)	char	CLA
@param		(in)	char	INS
@param		(in)	char	P1
@param		(in)	char	P2
@param		(in)	char	Lc
@param		(in)	char	iData
@param		(in)	char	Le
@param		(out)	char	oData
@return		APDU串长度
*/
int PackAPDU(char CLA, char INS, char P1, char P2, char Lc, const char *iData, char Le, char *oData)
{
    int Len;

    Len = 0;
    oData[0] = CLA;
    oData[1] = INS;
    oData[2] = P1;
    oData[3] = P2;
    Len += 4;

    if (Lc > 0)
    {
        oData[Len++] = Lc;
        memcpy( oData + Len, iData, Lc );
        Len += Lc;
    }

    if (Le != 0xFF)		//Le==0xFF时表示Le不存在
    {
        oData[Len++] = Le;
    }

    return(Len);
}

/**
@brief		判断IC卡是否插入

@param		无
@return		TRUE/FALSE
*/
int IC_IsCardExist(void)
{
	int 	ret, State;
	char	IsCardExist;
	
    State = getDS5002state();
    if ( ret < 0 )
    {
        return(ICERR_UNKNOW);
    }
    switch (ICPort)
    {
    	case ICPORT1:
    		IsCardExist = State & IC1_EXIST;
    		break;
    	case ICPORT2:
	    	IsCardExist = State & IC2_EXIST;
    		break;
    }
    if (IsCardExist == 1)
    {
    	return(TRUE);
    } else
    {
    	return(FALSE);
    }
}


/**
@brief		判断IC卡是否已上电

@param		(in)	无
@return		TRUE/FALSE
*/
int IC_IsCardPowerUped(void)
{
	int 	ret, State;
	char	IsCardPowerUped;
	
    State = getDS5002state();
    if ( ret < 0 )
    {
        return(ICERR_UNKNOW);
    }
    switch (ICPort)
    {
    	case ICPORT1:
    		IsCardPowerUped = State & IC1_POWER;
    		break;
    	case ICPORT2:
	    	IsCardPowerUped = State & IC2_POWER;
    		break;
    }
    if (IsCardPowerUped == 1)
    {
    	return(TRUE);
    } else
    {
    	return(FALSE);
    }	
}


/*
@brief		取得IC读写出错码

将在iccrw返回值转为IC模块错误码
@param		(in)	int		ErrCode		iccrw返回值
@return		ICERR_xxx
*/
int GetIccrwErr(int ErrCode)
{
	switch (ErrCode)
	{
		case -2:
		case -4:
			return(ICERR_ISDRAW);
		case -3:
			return(ICERR_NOICCARD);
		case -7:
			return(ICERR_POWERUP);
	}
	return(ICERR_UNKNOW);
}

/*
@brief		解释SW为IC模块错误码

@param		(in)	char		SW1
@param		(in)	char		SW2
@param		(in)	TICCOmmand	Cmd		IC指令
@return		ICERR_xxx
*/
int ParseSW(char SW1, char SW2, TICCommand Cmd)
{
	mSW1 = SW1;
	mSW2 = SW2;

/*	if (!((SW1 == 0x90) && (SW2 == 0x00)))
	{
		ShowSWMessage(SW1, SW2);
	}
*/
	switch (SW1)
	{
		case 0x61:
			return(ICERR_NORMALPROCESS);
		case 0x62:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_NOINFORMATION);
				case 0x81:
					return(ICERR_ERRORRESPONSEDATA);
				case 0x82:
					return(ICERR_INVALIDFILELEN);
				case 0x83:
					return(ICERR_INVALIDSELECTFILE);
				case 0x84:
					return(ICERR_INVALIDFCI);
			}
			break;
		case 0x63:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_AUTHFAIL);
				default:
					if (SW2 >> 4 == 0xC0)
					{
						return(ICERR_VERIFYFAIL);
					}
			}
			break;
		case 0x64:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_STATEFLAGNOTCHANGED);
			}
			break;
		case 0x65:
			switch (SW2)
			{
				case 0x81:
					return(ICERR_MEMORYERROR);
			}
			break;
		case 0x67:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_LENERROR);
			}
			break;
		case 0x68:
			switch (SW2)
			{
				case 0x82:
					return(ICERR_UNSUPPORTSAFEMESSAGE);
			}
			break;
		case 0x69:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_CANTPROCESS);
				case 0x01:
					return(ICERR_CMDNOTACCEPT);
				case 0x81:
					return(ICERR_UNCOMPATIBLEFILESTRU);
				case 0x82:
					return(ICERR_SAFESTATEDISSATISFY);
				case 0x83:
					return(ICERR_VERIFYBLOCKED);
				case 0x84:
					return(ICERR_INVALIDREFERENCE);
				case 0x85:
					return(ICERR_DISSATISFYCONDITION);
				case 0x86:
					return(ICERR_NOTCURRENTEF);
				case 0x87:
					return(ICERR_MISSSAFEMESSAGE);
				case 0x88:
					return(ICERR_INVALIDSAFEMESSAGE);
			}
		case 0x6A:
			switch (SW2)
			{
				case 0x80:
					return(ICERR_INVALIDPARA);
				case 0x81:
					return(ICERR_FUNCTIONNOTSUPPORT);
				case 0x82:
					return(ICERR_FILENOTFOUND);
				case 0x83:
					return(ICERR_RECORDNOTFOUND);
				case 0x84:
					return(ICERR_NOTENOUGHSPACE);
				case 0x86:
					return(ICERR_INVALIDP1P2);
				case 0x88:
					return(ICERR_REFERDATANOTFOUND);
			}
			break;
		case 0x6B:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_INVALIDPARA);
			}
			break;
		case 0x6C:
			return(ICERR_INVALIDLEN);
		case 0x6F:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_INVALIDDATA);
			}
			break;
		case 0x90:
			switch (SW2)
			{
				case 00:
					return(ICERR_NONE);
			}
			break;
		case 0x93:
			switch (SW2)
			{
				case 0x01:
					return(ICERR_NOTENOUGHMONEY);
				case 0x02:
					return(ICERR_INVALIDMAC);
				case 0x03:
					return(ICERR_APPBLOCKEDPERMANENTLY);
			}
			break;
		case 0x94:
			switch (SW2)
			{
				case 0x01:
					return(ICERR_NOTENOUGHMONEY);
				case 0x02:
					return(ICERR_TRANDCOUNTERMAX);
				case 0x03:
					return(ICERR_KEYINDEXNOTSUPPORT);
				case 0x06:
					return(ICERR_MACCANTUSE);
			}
			break;
		case 0x6E:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_INVALIDCLA);
			}
			break;
		case 0x6D:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_CODENOTSUPPORT);
			}
			break;
		case 0x66:
			switch (SW2)
			{
				case 0x00:
					return(ICERR_RECEIVETIMEOUT);
				case 0x01:
					return(ICERR_PARITYCHECKERROR);
				case 0x02:
					return(ICERR_CHECKSUMERROR);
				case 0x03:
					return(ICERR_NOFCI);
				case 0x04:
					return(ICERR_NOSFORKF);
			}
			break;
	}
	return(ICERR_UNKNOW);
/*
	if (SW1 == 0x90 && SW2 == 0x00)
	{
		return(ICERR_NONE);
	} else
	{
		switch (Cmd)
		{
			case ICCMD_SELECT:
				if (SW1==0x62 && SW2==0x83)
				{
					return(ICERR_SELECTINVALID);
				}
				break;
		}
		clrscr();printf("SW=%02x%02x", SW1, SW2);getkeycode(0);
		if (SW1==0x6A && SW2==0x83)
		{
			return(ICERR_NORECORD);
		}
		if (SW1==0x6A && SW2==0x81)
		{
			return(ICERR_CARDBLOCKED);
		}
	}
	return(ICERR_NONE);
*/	
}

/**
@brief		IC卡上电

@param		(in)	无
@return		ICERR_xxx
*/
int IC_PowerUp(void)
{
    int     	ret;
    TICReceive	ReceiveBuf;

    if (IC_IsCardExist() == FALSE)
    {
    	return(ICERR_NOICCARD);
    }
    
    ret = icpowerup(ICPort, ReceiveBuf);
    if (ret != SUCC)
    {
        return(ICERR_POWERUP);
    }
	return(ICERR_NONE);
}

/*
@brief		IC卡下电

@param		无
@return		ICERR_xxx
*/
int IC_PowerDown(void)
{
    int     ret;

    ret = icpowerdown(ICPort);
    if (ret != SUCC)
    {
        return(ICERR_POWERDOWN);
    }
	return(ICERR_NONE);
}


/**
@brief		ReadRecord

@param		(in)	char		SFI			短文件名
@param		(in)	char		RecordNo	记录号
@param		(out)	char		*oTLVStr	响应报文
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_ReadRecord(char SFI, char RecordNo, char *oTLVStr)
{
    int     	ret, APDUStrLen, ReceiveLen;
    char		P2;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	P2 = ((SFI & 0x1F) << 3) | 0x04;
	APDUStrLen = PackAPDU(0x00, 0xB2, RecordNo, P2, NULL, NULL, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return (GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
    	ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_READRECORD);
    	if (ret != ICERR_NONE)
    	{
    		return(ret);
    	}
        memcpy(oTLVStr, ReceiveBuf, ReceiveLen-2);		//-2:去除SW1,SW2
		return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);
}


/**
@brief		Select指令

@param		(in)	char		*AID		AID名
@param		(in)	int			AIDLen		AID长度
@param		(in)	TSelectMode	SM			选择模式
@param		(out)	char		*oTLVStr	响应报文
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_SelectByAID(const char *AID, int AIDLen, TSelectMode SM, char *oTLVStr)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	if (SM == SELECTMODE_FIRST)
	{
		APDUStrLen = PackAPDU(0x00, 0xA4, 0x04, 0x00, AIDLen, AID, 0x00, (char *)APDUStr);
	} else
	{
		APDUStrLen = PackAPDU(0x00, 0xA4, 0x04, 0x02, AIDLen, AID, 0x00, (char *)APDUStr);
	}

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return (GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
    	
    	ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_SELECT);
        memcpy(oTLVStr, ReceiveBuf, ReceiveLen-2);		//-2:去除SW1,SW2
    	if (ret != ICERR_NONE)	
    	{
			oTLVStr[0] = ReceiveLen-2;
			memcpy(oTLVStr+1, ReceiveBuf, ReceiveLen-2);		//-2:去除SW1,SW2
   			return(ret);
    	}
		else
		{
        	memcpy(oTLVStr, ReceiveBuf, ReceiveLen-2);		//-2:去除SW1,SW2
			return(ReceiveLen-2);
		}
    }
    
    return(ICERR_UNKNOW);
}

/**
@brief		SelectPSE

@param		(out)	char		*oSFI		标签88返回SFI
@return		ICERR_xxx
*/
int IC_SelectPSE(char *oSFI)
{
	int 	ret, TLVStrLen, Len;
	char	TLVStr[512], Value[512], AID[32];
	char 	IssCodeIndex;
	TTagList taglist[20];
	int		tagnum;
	int 	i;
	int     TagA5_offset;
	int		TagA5_len;
	int		Tag84_offset;
	int		Tag84_len;
	
	strcpy(AID, "1PAY.SYS.DDF01");
	
	//选择PSE
	TLVStrLen = IC_SelectByAID(AID, strlen(AID), SELECTMODE_FIRST, TLVStr);
	if (TLVStrLen < 0)
	{
		return(TLVStrLen);
	}

	//判断返回的数据是否正确
	
	//解码IC卡返回的TLV串
	TLV_Init(NULL);
	ret = TLV_Decode(TLVStr, TLVStrLen);
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

	//对选择PSE, 以下标签存在性为M, 故须做检查
	ret = TLV_GetValue("\x6F", Value, &Len, 1);	//检查FCI模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

	//判断6F模板是否正确
	if(Len + 2 != TLVStrLen)
	{
		return FAIL;
	}

	memset(taglist, 0, sizeof(taglist));
	tagnum = 0;
	TLV_GetTagList(taglist, &tagnum);

	for(i=0; i<tagnum; i++)
	{
		if(taglist[i].Tag[0] == 0x84)
		{
			Tag84_offset = i;
		}
		if(taglist[i].Tag[0] == 0xA5)
		{
			TagA5_offset = i;
		}
	}

	if(Tag84_offset > TagA5_offset) 
	{
		return USEAIDLIST;
	}
	
	ret = TLV_GetValue("\x84", Value, &Len, 1);	//检查DF
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

	ret = TLV_GetValue("\xA5", Value, &Len, 1);	//检查FCI数据专用模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

		
/*	
	ret = TLV_GetValue("\x9f\x11", Value, &Len, 1); //取发卡行代码索引
	if (ret == TLVERR_NONE)
	{
		IssCodeIndex = Value[0];
		//判断是否支持该发卡行索引代码	
		switch(IssCodeIndex)
		{
		case 1:
			if(!(g_termconfig.TermCapAppend[4] & 0x01))
			{
				return FAIL;
			}
			break;
		case 2:
			if(!(g_termconfig.TermCapAppend[4] & 0x02))
			{
				return FAIL;
			}
			break;
		case 3:
			if(!(g_termconfig.TermCapAppend[4] & 0x04))
			{
				return FAIL;
			}
			break;
		case 4:
			if(!(g_termconfig.TermCapAppend[4] & 0x08))
			{
				return FAIL;
			}
			break;
		case 5:
			if(!(g_termconfig.TermCapAppend[4] & 0x10))
			{
				return FAIL;
			}
			break;
		case 6:
			if(!(g_termconfig.TermCapAppend[4] & 0x20))
			{
				return FAIL;
			}
			break;
		case 7:
			if(!(g_termconfig.TermCapAppend[4] & 0x40))
			{
				return FAIL;
			}
			break;
		case 8:
			if(!(g_termconfig.TermCapAppend[4] & 0x80))
			{
				return FAIL;
			}
			break;
		case 9:
			if(!(g_termconfig.TermCapAppend[3] & 0x01))
			{
				return FAIL;
			}
			break;
		case 10:
			if(!(g_termconfig.TermCapAppend[3] & 0x02))
			{
				return FAIL;
			}
			break;
		default:
			return FAIL;
		}
	}
*/	
	ret = TLV_GetValue("\x88", Value, &Len, 1);	//检查SFI
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	*oSFI = Value[0] & 0x1F;	//低5位为SFI

	if(*oSFI<=10 && *oSFI>=1)	// SFI必须在01-10之间
	{
		return(ICERR_NONE);
	}
	else
	{
		return ICERR_LENERROR ;
	}
}


/**
@brief		选择DDF

@param		(in)	char		*AID		AID名
@param		(in)	int			AIDLen		AID长度
@param		(out)	char		*oSFI		SFI
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_SelectDDF(char *AID, int AIDLen, char *oSFI)
{
	int 	ret, TLVStrLen, Len;
	char	TLVStr[512], Value[512];
	TTagList taglist[20];
	int		tagnum;
	int 	i;
	int     TagA5_offset;
	int		TagA5_len;
	int		Tag84_offset;
	int		Tag84_len;

	//选择PSE
	TLVStrLen = IC_SelectByAID(AID, AIDLen, SELECTMODE_FIRST, TLVStr);
	if (TLVStrLen < 0)
	{
		return(TLVStrLen);
	}

	//解码IC卡返回的TLV串
	TLV_Init(NULL);
	ret = TLV_Decode(TLVStr, TLVStrLen);
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

	memset(taglist, 0, sizeof(taglist));
	tagnum = 0;
	TLV_GetTagList(taglist, &tagnum);

	for(i=0; i<tagnum; i++)
	{
		if(taglist[i].Tag[0] == 0x84)
		{
			Tag84_offset = i;
		}
		if(taglist[i].Tag[0] == 0xA5)
		{
			TagA5_offset = i;
		}
	}

	if(Tag84_offset > TagA5_offset) 
	{
		return USEAIDLIST;
	}

	//对选择PSE, 以下标签存在性为M, 故须做检查
	ret = TLV_GetValue("\x6F", Value, &Len, 1);	//检查FCI模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	ret = TLV_GetValue("\x84", Value, &Len, 1);	//检查DF
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	ret = TLV_GetValue("\xA5", Value, &Len, 1);	//检查FCI数据专用模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	ret = TLV_GetValue("\x88", Value, &Len, 1);	//检查SFI
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	*oSFI = Value[0] & 0x1F;	//低5位为SFI

	if(*oSFI<=10 && *oSFI>=1)	// SFI必须在01-10之间
	{
		return(ICERR_NONE);
	}
	else
	{
		return FAIL;
	}
}


/**
@brief		选择ADF

@param		(in)	char		*AID		AID名
@param		(in)	int			AIDLen		AID长度
@param		(in)	TSelectMode	SM			选择模块
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_SelectADF(char *AID, int AIDLen, TSelectMode SM, char *PDOL, int *PDOLLen)//, char *oSFI)
{
	int 	ret, TLVStrLen, Len;
	char	TLVStr[512], Value[512];
	int     retcode;
		
	//选择ADF
	TLVStrLen = IC_SelectByAID(AID, AIDLen, SM, TLVStr);
	retcode = TLVStrLen;
	
	if(TLVStrLen == ICERR_INVALIDSELECTFILE)	//应用锁定
	{
		TLVStrLen = TLVStr[0];
		memcpy(TLVStr, TLVStr+1, TLVStrLen);
		if (TLVStrLen == 0)
		{
//			clrscr() ;
	//		printf("Length  gotten 0") ;
			Get_response(0x00, TLVStr) ;
			TLVStrLen = TLVStr[0] ;
			memcpy(TLVStr, TLVStr+1, TLVStrLen);
		}
	}
	
	if (TLVStrLen < 0)
	{
		return(TLVStrLen);
	}
	//Debug("ADF:", TLVStrLen, TLVStr);

	//解码IC卡返回的TLV串
	TLV_Init(NULL);
	ret = TLV_Decode(TLVStr, TLVStrLen);
	if (ret != TLVERR_NONE)
	{
ERR_MSG("1");
		return(ret);
	}
	//对选择PSE, 以下标签存在性为M, 故须做检查
	ret = TLV_GetValue("\x6F", Value, &Len, 1);	//检查FCI模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}

	//判断6F模板是否正确
	if(Len + 2 != TLVStrLen)
	{
		return FAIL;
	}

	ret = TLV_GetValue("\x84", Value, &Len, 1);	//检查DF
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	ret = TLV_GetValue("\xA5", Value, &Len, 1);	//检查FCI数据专用模板
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
	ret = TLV_GetValue("\x9F\x38", Value, &Len, 1);	//取PDOL值
	if (ret == TLVERR_NONE)
	{
		memcpy(PDOL, Value, Len);
		*PDOLLen = Len;
	} else
	{
		PDOL[0] = 0;
		*PDOLLen = 0;
	}

#if (0)
	ret = TLV_GetValue("\x50", Value, &Len, 1);	//检查SFI
	if (ret != TLVERR_NONE)
	{
		return(ret);
	}
#endif	
//	*oSFI = Value[0];
	if(retcode < 0)
	{
		return retcode;
	}
	else
	{
		return(ICERR_NONE);
	}
}

/**
@brief		GetChallenge指令

@param		(out)	char	*pOut		随机数
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_GetChallenge(char *pOut)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	APDUStrLen = PackAPDU(0x00, 0x84, 0x00, 0x00, NULL, NULL, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return (GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
        ret = ParseSW(ReceiveBuf[ReceiveLen - 2], ReceiveBuf[ReceiveLen -1], ICCMD_OTHERS) ;
	if (ret != ICERR_NONE)
		return ret ;
	memcpy(pOut,ReceiveBuf, ReceiveLen-2) ;
        return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);
}

/**
@brief		外部认证

@param		(in)	char		*iData		输入数据
@param		(in)	int			DataLen		数据长度
@return		ICERR_xxx
*/
int IC_ExternalAuthenticate(char *iData, char DataLen)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	APDUStrLen = PackAPDU(0x00, 0x82, 0x00, 0x00, DataLen, iData, 0xFF, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return (GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		return(ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_EXTERNALAUTH));
    }
    
    return(ICERR_UNKNOW);
}

/**
@brief		内部认证

@param		(in)	char		*iData		输入数据
@param		(in)	int			iDataLen	输入数据长度
@param		(in)	char		*oData		输出数据
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_InternalAuthenticate(char *iData, char iDataLen, char *oData)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	APDUStrLen = PackAPDU(0x00, 0x88, 0x00, 0x00, iDataLen, iData, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return (GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_EXTERNALAUTH);
		if (ret != ICERR_NONE)
		{
			return(ret);
		}
		memcpy(oData, ReceiveBuf, ReceiveLen-2);
		return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);
}


/**
@brief		验证

@param		(in)	char		*iData		输入数据
@param		(in)	int			iDataLen	输入数据长度
@param		(in)	TVerifyMode	VM			验证模式
@return		ICERR_xxx
*/
int IC_Verify(char *iData, char iDataLen, TVerifyMode VM)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};
    char		P2;

	switch (VM)
	{
		case VERIFYMODE_NORMAL:
			P2 = 0x80;
			break;
		case VERIFYMODE_ENCRYPT:
			P2 = 0x88;
			break;
	}
	APDUStrLen = PackAPDU(0x00, 0x20, 0x00, P2, iDataLen, iData, 0xff, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return(GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		return(ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_INTERNALAUTH));
    }
    
    return(ICERR_UNKNOW);
}


/**
@brief		GetProcessOptions指令

@param		(in)	char		*iData		输入数据
@param		(in)	int			iDataLen	输入数据长度
@param		(in)	char		*oData		输出数据
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_GetProcessOptions(char *iData, char iDataLen, char *oData)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};

	APDUStrLen = PackAPDU(0x80, 0xA8, 0x00, 0x00, iDataLen, iData, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return(GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_GETPROCESSOPTIONS);
		if (ret != ICERR_NONE)
		{
			return(ret);
		}
        memcpy(oData, ReceiveBuf, ReceiveLen-2);
        return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);
}


/**
@brief		GetData指令

@param		(in)	TGetDataMode	GDM		取数据模式
@param		(in)	char		*oData		输出数据
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_GetData(TGetDataMode GDM, char *oData)
{
    int     	ret, APDUStrLen, ReceiveLen, Len;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};
    char 		P1, P2;
    TTag		Tag;
    char		Value[512];
    

    switch (GDM)
    {
    	case GETDATAMODE_ATC:
    		P1 = 0x9F;
    		P2 = 0x36;
    		break;
    	case GETDATAMODE_ONLINEATC:
    		P1 = 0x9F;
    		P2 = 0x13;
    		break;
    	case GETDATAMODE_PASSWORDRETRY:
    		P1 = 0x9F;
    		P2 = 0x17;
    		break;
	case GETDATAMODE_LOGFORMAT:
		P1 = 0x9F ;
		P2 = 0x4F ;
		break ;
    }

	APDUStrLen = PackAPDU(0x80, 0xCA, P1, P2, NULL, NULL, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return(GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_GETPROCESSOPTIONS);
		if (ret != ICERR_NONE)
		{
			return(ret);
		}
    	memset((char *)Tag, 0, sizeof(Tag));
    	Tag[0] = P1;
    	Tag[1] = P2;
		TLV_Init(NULL);
    	ret = TLV_Decode(ReceiveBuf, ReceiveLen-2);
    	if (ret != TLVERR_NONE)
    	{
    		return(ret);
    	}
    	//检查IC卡返回的是P1, P2所指定的Tag
    	ret = TLV_GetValue(Tag, Value, &Len, 1);
    	if (ret != TLVERR_NONE)
    	{
    		return(ret);
    	}
		
        memcpy(oData, ReceiveBuf, ReceiveLen-2);
        return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);
}

/**
@brief		GenerateAC指令

@param		(in)	TACType		ACType
@param		(in)	char		*iData		输入数据
@param		(in)	int			iDataLen	输入数据长度
@param		(in)	char		*oData		输出数据
@return		>=0		响应报文长度
@return		<0		ICERR_xxx
*/
int IC_GenerateAC(TACType ACType, int iNeedCDA, char *iData, int iDateLen, char *oData)
{
    int     	ret, APDUStrLen, ReceiveLen;
    TAPDUStr	APDUStr = {0};
    TICReceive	ReceiveBuf = {0};
    char 		P1;

    switch (ACType)
    {
    	case ACTYPE_AAC:
    		P1 = 0x00;
    		break;
    	case ACTYPE_TC:
    		P1 = 0x40;
    		break;
    	case ACTYPE_ARQC:
    		P1 = 0x80;
    		break;
    	case ACTYPE_RFU:
    		P1 = 0xC0;
    		break;
    }
  	if (iNeedCDA)
		P1 |= 0x40 ;
	
	APDUStrLen = PackAPDU(0x80, 0xAE, P1, 0x00, iDateLen, iData, 0x00, (char *)APDUStr);

    ret = iccrw_new(ICPort, APDUStrLen, APDUStr, &ReceiveLen, ReceiveBuf);
    if (ret < 0)
    {
        return(GetIccrwErr(ret));
    } else
    if (ret == 0)
    {
		ret = ParseSW(ReceiveBuf[ReceiveLen-2], ReceiveBuf[ReceiveLen-1], ICCMD_GETPROCESSOPTIONS);
		if (ret != ICERR_NONE)
		{
			return(ret);
		}
		TLV_Init(NULL);
    	ret = TLV_Decode(ReceiveBuf, ReceiveLen-2);
    	if (ret != TLVERR_NONE)
    	{
    		return(ret);
    	}
        memcpy(oData, ReceiveBuf, ReceiveLen-2);
        return(ReceiveLen-2);
    }
    
    return(ICERR_UNKNOW);

}


/**
@brief		取最近一次的状态字

@param		(out)	char	*SW1
@param		(out)	char	*SW2
@return		无
*/
void IC_GetLastSW(char *SW1, char *SW2)
{
	memcpy(SW1, (char *)&mSW1, 1);
	memcpy(SW2, (char *)&mSW2, 1);
}

/**
@brief		返回当前使用的IC卡座

@return		无
*/
int IC_GetPort() 
{
	return ICPort ;
}

int ic_comm(char CLA,char INS,char P1,char P2,char Lc,char *d_in,char Le,char *d_out)
{
	char cmd[150];
	char cmd_len;
	char response[300];
	int ret;
//	char state;

	cmd[0]=CLA;
	cmd[1]=INS;
	cmd[2]=P1;
	cmd[3]=P2;
	cmd_len=4;
	if(Lc>0)
	{
		cmd[4]=Lc;
		memcpy(cmd+5,d_in,Lc);
		cmd_len=cmd_len+1+Lc;
	}
	if(Le!=0xff)
	{
		cmd[cmd_len]=Le;
		cmd_len=cmd_len+1;
	}
	memset(response,0,sizeof(response));//need check
//	if(CLA==0x00&&INS==0xa4)	Dis_deg_buf(cmd,cmd_len);
	ret=iccrw(ICPort ,cmd_len,cmd,response);//读卡,cmd为输出,response为接受缓冲区。
//	if(CLA==0x00&&INS==0xc0)	Dis_deg_buf(response+1,response[0]);
//	if(CLA==0x00&&INS==0xa4)		ic_err(ret);
//	if(ret==SUCC)
//	{
//		if(response[response[0]-1]==0x6c)
//		{
//			Le=response[response[0]];
//			cmd[cmd_len-1]=Le;
//			goto RESENDCODE;
//		}
//	}

	if((ret==-4)||(ret==-2))
	{
//		ERR_MSG("ICC is pulled out when reading! Please insert original card...\n");
		return FAIL ;
	}
	else if(ret==0)
	{
		memcpy(d_out,response,response[0]+1);
//		Dis_deg_buf(response,response[0]+1);
//		if(ZW)
//		{
//			ERR_CODE(response[response[0]-1],response[response[0]]);
//			Dis_deg_buf(response,response[0]+1);
//		}
	}
	else
	{
//		clrscr();
//		printf("!!![%02x][%02x]\nAn error when reading ICC!",(unsigned int)ret,GetErrorCode());
//		Beep(3);
//		getkeycode(0);
		return FAIL;
	}
	return SUCC;
}
int Get_response(char len , char *response)
{
	int ret;
	ret=ic_comm(0x00,0xC0,0x00,0x00,0x00,NULL,len,response);
//	Dis_deg_buf(response+1,response[0]);
	if(ret==SUCC)
	{
		if((response[response[0]-1]==0x90)&&(response[response[0]]==0x00))
			return SUCC;
		if(response[response[0]-1]==0x61)
			return Get_response(response[response[0]],response);
		else
		{
//			ERR_CODE(response[response[0]-1],response[response[0]]);
			return FAIL;
		}
	}
	else
		return FAIL;
}
