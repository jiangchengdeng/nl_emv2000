#include <stdlib.h>
#include <string.h>
#include "posapi.h"
#include "define.h"
#include "posdef.h"
#include "tlv.h"
#include "ic.h"
#include "appselect.h"

char m_PosAidNum;			//POS支持的应用数


static TAIDList	POSAIDList[MAXAIDLISTNUM];


typedef struct
{
	char	Tag_9F1A[2];
}TDOL;


int GetTermAidList(void)
{
	int fp;
	int len;
	int i;
	
	//打开终端AID列表文件
	fp = fopen(TermAIDFile, "r");
	if(fp < 0)
	{
		return FAIL;
	}

	if(fread((char *)&m_PosAidNum, 1, fp)!=1)
	{
		return FAIL;
	}

	if(m_PosAidNum > MAXAIDLISTNUM)
	{
		m_PosAidNum = MAXAIDLISTNUM;
	}
	
	//读取终端AID列表
	for(i=0; i<m_PosAidNum; i++)
	{
		
		len = fread((char *)&POSAIDList[i], AIDLISTLEN, fp);
		if(len != AIDLISTLEN)
		{
			break;
		}
	}

	m_PosAidNum = i;

	fclose(fp);
	return SUCC;
}
	
int GetAID(char SFI, TAIDList AIDList[], int *AIDNum)
{
	int ret, TLVStrLen, Len, Offset, Tag70Len, Tag61Len, Tag61LenLen;
	char	TLVStr[256], RecordNo, DDF[17], ADF[17], Tag70Value[512], Tag61Value[256],
			AppLabel[17], AppPreferedName[17], AppPriorityID, DDF_SFI;
	char tmp;
	int tag61_flag=0;

	RecordNo = 1;
	
	while(1)
	{
//		clrscr();printf("RecordNo=%d\n", RecordNo);getkeycode(0);
		TLVStrLen = IC_ReadRecord(SFI, RecordNo, TLVStr);
		
		if (TLVStrLen == ICERR_RECORDNOTFOUND)
		{
			break;
/*
			if (TLVStrLen == ICERR_RECORDNOTFOUND)
			{
				break;
			} else
			{
				RecordNo++;
				continue;
			}
*/			
		}

		if(TLVStrLen < 0)
		{
			return USEAIDLIST;
		}

		TLV_Init(NULL);
		if (TLV_Decode(TLVStr, TLVStrLen) != TLVERR_NONE)
		{
			return(USEAIDLIST);
		}
		ret = TLV_GetTemplate("\x70", &Offset, &Tag70Len);
		if (ret != TLVERR_NONE)
		{
			return(FAIL);
		}
		memcpy(Tag70Value, TLVStr+Offset, Tag70Len);

		//判断70模板是否正确
		if(Tag70Len + 2 != TLVStrLen)
		{
			return USEAIDLIST;
		}
		
		Offset = 0;		//Tag70中偏移
		tag61_flag = 0;
		while(1)
		{
			if (Tag70Value[Offset] == 0x61)
			{
				tag61_flag =1;
				ret = TLV_GetLength(Tag70Value+Offset+1, Tag70Len, &Tag61Len, &Tag61LenLen);
				if (ret != TLVERR_NONE)
				{
					return(FAIL);
				}
				memcpy(Tag61Value, Tag70Value+Offset+1+Tag61LenLen, Tag61Len);
//	Disp_deg_buf(Tag61Value, Tag61Len);
				TLV_Init(NULL);
				ret = TLV_Decode(Tag61Value, Tag61Len);
				if (ret != TLVERR_NONE)
				{
					return(USEAIDLIST);
				}


				ret = TLV_GetValue("\x9D", DDF, &Len, 1);	//判断记录中目录入口是否为DDF
				if (ret == TLVERR_NONE)	//记录为DDF,则须进入该目录再ReadRecord
				{
//					clrscr();printf("DDF");getkeycode(0);
					DDF[Len] = 0;
					ret = IC_SelectDDF(DDF, Len, &DDF_SFI);
					if (ret != ICERR_NONE)
					{
						if (ret != ICERR_RECORDNOTFOUND)//CB022要求
						{
							return USEAIDLIST ;
						}
						goto NEXT61TAG;
					}
					if (GetAID(DDF_SFI, AIDList, AIDNum) == USEAIDLIST)	//递归调用
					{
						return USEAIDLIST ;
					}
					IC_SelectPSE(&tmp);
				}
				else	//非DDF
				{
					ret = TLV_GetValue("\x4F", ADF, &Len, 1);
					if (ret == TLVERR_NONE)		//记录为ADF, 存在方式:M
					{
						memcpy(AIDList[*AIDNum].AID, ADF, Len);
						AIDList[*AIDNum].AIDLen = Len;
						AIDList[*AIDNum].Enable = TRUE;
					} else
					{
						return(USEAIDLIST);
					}

					ret = TLV_GetValue("\x50", AppLabel, &Len, 1);	//应用标签, 存在方式:M
					if (ret == TLVERR_NONE)
					{
						memcpy(AIDList[*AIDNum].AppLabel, AppLabel, Len);
						AIDList[*AIDNum].AppLabelLen = Len;
					} else
					{
						return(USEAIDLIST);
					}

					ret = TLV_GetValue("\x9F\x12", AppPreferedName, &Len, 1);	//应用优先名称, 存在方式:O
					if (ret == TLVERR_NONE)
					{
						memcpy(AIDList[*AIDNum].AppPreferedName, AppPreferedName, Len);
						AIDList[*AIDNum].AppPreferedName[Len] = 0;
						AIDList[*AIDNum].AppPreferedNameLen = Len;
					}
					else
					{
						memcpy(AIDList[*AIDNum].AppPreferedName, AIDList[*AIDNum].AppLabel, AIDList[*AIDNum].AppLabelLen);
			//			AIDList[*AIDNum].AppPreferedName[Len] = 0;
						AIDList[*AIDNum].AppPreferedNameLen = AIDList[*AIDNum].AppLabelLen;
					}
					
					ret = TLV_GetValue("\x87", (char *)&AppPriorityID, &Len, 1);	//应用优先权标识符, 存在方式:O
					if (ret == TLVERR_NONE)
					{
						AIDList[*AIDNum].AppPriorityID = AppPriorityID;
					}
					*AIDNum = *AIDNum+1;
				}
				
NEXT61TAG:				
				Offset += 1+Tag61LenLen+Tag61Len;
				if (Offset >= Tag70Len)
				{
					break;
				}
			} 
			else
			{
				Offset++ ;
				if (Offset >= Tag70Len)
				{
					break;
				}
				continue;
//				return QUIT;
			}
		}
		RecordNo++;
		if(tag61_flag == 0)
		{
			return (USEAIDLIST);
		}
	}

	return(SUCC);
}


int GetPOSAID(char AllowPartialAID, TAIDList AIDList[], int *AIDNum)
{
	TSelectMode SelectMode ;
	int		POSAIDNum, i, ret, Len, PDOLLen, AppLabelLen, AidLen;
	char	DF[32], PDOL[256], AppLabel[16], AppPreferedName[17], AppPriorityID;
	int flag6283 ;
	int 	CodeCount=0;
	int     count=0;

//	POSAIDNum = sizeof(POSAIDList)/sizeof(TAIDList);
	POSAIDNum = m_PosAidNum;
	*AIDNum = 0;
	SelectMode = SELECTMODE_FIRST;

	for (i=0; i<POSAIDNum; )
	{
//		clrscr();printf("i=%d", i);getkeycode(0);
		//选择POS支持的应用的AID
		ret = IC_SelectADF(POSAIDList[i].AID, POSAIDList[i].AIDLen, SelectMode, PDOL, &PDOLLen);
		count++;
		if(ret == ICERR_CODENOTSUPPORT)
		{
			CodeCount++;
		}
		
		if(ret == ICERR_FUNCTIONNOTSUPPORT)	//应用选择返回6A81
		{
			return FAIL;
		}
//		clrscr();printf("IC_SelectADF ret=%d", ret);getkeycode(0);
		if (ret != ICERR_NONE)
		{
			if (ret != ICERR_INVALIDSELECTFILE )
			{
				i++;
				SelectMode = SELECTMODE_FIRST;
				continue;
			}
		}
		flag6283 = ret ;
		ret = TLV_GetValue("\x84", DF, &AidLen, 1);
		if (ret != TLVERR_NONE)
		{
			return(ret);
		}
		if (flag6283 == ICERR_INVALIDSELECTFILE)
		{
			if (!AllowPartialAID || AidLen <= POSAIDList[i].AIDLen)
			{
				i++;
				SelectMode = SELECTMODE_FIRST;
			}
			else
				SelectMode = SELECTMODE_NEXT ;
			continue ;
		}
//		clrscr();printf("Len=%d\nAidLen=%d", AidLen,POSAIDList[i].AIDLen);getkeycode(0);
		//DF名与POS的AID完全匹配
		if (((AidLen == POSAIDList[i].AIDLen) && (memcmp(DF, POSAIDList[i].AID, AidLen) == 0))
		   || ((AidLen > POSAIDList[i].AIDLen) && AllowPartialAID))
		{

			AIDList[*AIDNum].Enable = TRUE;
			memcpy(AIDList[*AIDNum].AID, DF, AidLen);
			AIDList[*AIDNum].AIDLen = AidLen;

			//应用标签
			ret = TLV_GetValue("\x50", AppLabel, &AppLabelLen, 1);
			if (ret != TLVERR_NONE)
			{
				memcpy(AIDList[*AIDNum].AppLabel, POSAIDList[i].AppLabel, POSAIDList[i].AppLabelLen);
				AIDList[*AIDNum].AppLabelLen = POSAIDList[i].AppLabelLen;
			} else
			{
				memcpy(AIDList[*AIDNum].AppLabel, AppLabel, AppLabelLen);
				AIDList[*AIDNum].AppLabelLen = AppLabelLen;
			}

			ret = TLV_GetValue("\x9F\x12", AppPreferedName, &Len, 1);	//应用优先名称, 存在方式:O
			if (ret == TLVERR_NONE)
			{
				memcpy(AIDList[*AIDNum].AppPreferedName, AppPreferedName, Len);
//				AIDList[*AIDNum].AppPreferedName[Len] = 0;
				AIDList[*AIDNum].AppPreferedNameLen = Len;
			}
			else
			{
/*
				memcpy(AIDList[*AIDNum].AppPreferedName, POSAIDList[i].AppPreferedName, POSAIDList[i].AppPreferedNameLen);
				AIDList[*AIDNum].AppPreferedNameLen = POSAIDList[i].AppPreferedNameLen;
*/
				memcpy(AIDList[*AIDNum].AppPreferedName, AIDList[*AIDNum].AppLabel, AIDList[*AIDNum].AppLabelLen);
				AIDList[*AIDNum].AppPreferedNameLen = AIDList[*AIDNum].AppLabelLen;
			}
			
			ret = TLV_GetValue("\x87", (char *)&AppPriorityID, &Len, 1);	//应用优先权标识符, 存在方式:O
			if (ret == TLVERR_NONE)
			{
				AIDList[*AIDNum].AppPriorityID = AppPriorityID;
			}
			else
			{
				AIDList[*AIDNum].AppPriorityID = POSAIDList[i].AppPriorityID;
			}
	
			*AIDNum += 1;
			if (AidLen > POSAIDList[i].AIDLen)
			{
				if (AllowPartialAID)	//部分名称选择处理
				{
					SelectMode = SELECTMODE_NEXT;
					continue;
				} else
				{
					i++;
					SelectMode = SELECTMODE_FIRST;
					continue;
				}
			}
			i++;
			SelectMode = SELECTMODE_FIRST;
		} 
		else
		{
			i++;
			SelectMode = SELECTMODE_FIRST;
		}
	}
	if(count == CodeCount)
	{
		return ICERR_CODENOTSUPPORT;
	}
	else
	{
		return(SUCC);	
	}
}

int CreateAppList(char AllowPartialAID, TAIDList oAIDList[], int *oAIDNum)
{
	int ret, AIDNum, i, j, NewAIDNum, POSAIDNum;
	char  SFI ;
	TAIDList	AIDList[32], TmpAID;
	char	Found;

	TLV_Init(NULL);
//	POSAIDNum = sizeof(POSAIDList)/sizeof(TAIDList);
	POSAIDNum = m_PosAidNum;
	ret = IC_SelectPSE(&SFI);
	if ((ret != ICERR_NONE) && (ret != ICERR_FILENOTFOUND) && (ret!=ICERR_CODENOTSUPPORT)
		 && (ret != ICERR_LENERROR)&&(ret > TLVERR_BASE))//CASE CL001/002/003	
	{
		return(ret);
	}

	memset((char *)AIDList, 0, sizeof(AIDList));
	AIDNum = 0;
	if (ret == ICERR_NONE)		//选择PSE成功时
	{
		//获取卡片上的应用列表
		ret = GetAID(SFI, AIDList, &AIDNum);
		if(ret == QUIT)
		{
			return QUIT;
		}
		if (ret == USEAIDLIST)
		{
			memset(AIDList, 0 , sizeof(TAIDList)*AIDNum);
			AIDNum = 0;
		}
//		clrscr();printf("GetAID ret=%d\n卡片应用数=%d", ret, AIDNum);getkeycode(0);
	}
	
	//建立POS与卡片均支持的候选列表
	NewAIDNum = AIDNum;
	for (i=0; i<AIDNum; i++)
	{
		Found = FALSE;
		for (j=0; j<POSAIDNum; j++)
		{
#if (0)
			clrscr();
			for (k=0; k<AIDList[i].AppLabelLen; k++) printf("%02x ", AIDList[i].AppLabel[k]);
			printf("\n");
			for (k=0; k<POSAIDList[j].AppLabelLen; k++) printf("%02x ", POSAIDList[j].AppLabel[k]);
			getkeycode(0);
#endif			
			if (((AIDList[i].AIDLen > POSAIDList[j].AIDLen) && (AllowPartialAID) && (memcmp(AIDList[i].AID, POSAIDList[j].AID, POSAIDList[j].AIDLen) == 0))
				|| ((AIDList[i].AIDLen == POSAIDList[j].AIDLen) && (memcmp(AIDList[i].AID, POSAIDList[j].AID, AIDList[i].AIDLen)==0)))
			{
				Found = TRUE;
				break;
			}
		}
		if (Found == FALSE)
		{
			AIDList[i].Enable = FALSE;
			NewAIDNum--;
		}
		
	}
	AIDNum = NewAIDNum;
//	clrscr();printf("POS与卡片均支持应用数=%d", AIDNum);getkeycode(0);
	
	if (AIDNum == 0)	//无POS和卡片都允许的应用
	{
		ret = GetPOSAID(AllowPartialAID, AIDList, &AIDNum);
//		clrscr();printf("无均支持应用,枚举POS应用,卡片支持=%d", AIDNum);getkeycode(0);
		if(ret == ICERR_CODENOTSUPPORT)
		{
			return ICERR_CODENOTSUPPORT;
		}
	}

	//按优先级排序
	for (i=0; i<AIDNum-1; i++)
	{
		for (j=i+1; j<AIDNum; j++)
		{
			if ((AIDList[i].AppPriorityID & 0x0F) > (AIDList[j].AppPriorityID & 0x0F))
			{
				memcpy((char *)&TmpAID, (char *)&AIDList[i], sizeof(TAIDList));
				memcpy((char *)&AIDList[i], (char *)&AIDList[j], sizeof(TAIDList));
				memcpy((char *)&AIDList[j], (char *)&TmpAID, sizeof(TAIDList));
			}
		}
		
	}

	j = 0;
	for (i=0; i<AIDNum; i++)
	{
		if (AIDList[i].Enable == TRUE)
		{
			memcpy((char *)&(oAIDList[j++]), (char *)&(AIDList[i]), sizeof(TAIDList));
		}
	}
	*oAIDNum = AIDNum;
	
	return(SUCC);
}

int ShowAppList(TAIDList AIDList[], int AIDListNum)
{
	int i;
	
	clrscr();
	for (i=0; i<AIDListNum; i++)
	{
		if (AIDList[i].AppPreferedNameLen > 0)
		{
			printf("%d.%s\n", i+1, AIDList[i].AppPreferedName);
		} else
		{
			printf("%d.%s\n", i+1, AIDList[i].AppLabel);
		}
	}
	getkeycode(0);
	return SUCC ;
}
