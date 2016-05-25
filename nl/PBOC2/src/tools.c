//************************************************************************************

/**
*	@file tools.c
*	@brief 工具函数模块
*
*	包括各种通用函数
*	@version 1.0
*	@autor liuzq
*	@date 2005/03/15
*	@li ltrim
	@li	rtrim
	@li alltrim
    @li ReadLine
	@li WriteLine
	@li ReadItemFromINI
	@li Beep
	@li Beep_Msg
	@li CalcLRC
	@li ERR_MSG
	@li MSG
	@li SUCC_MSG
    @li AsciiToBcd
    @li BcdToAscii
    @li IntToC4
    @li IntToC2
    @li C4ToInt
    @li C2ToInt
    @li ByteToBcd
    @li BcdToByte
    @li IntToBcd
    @li BcdToInt
    @li CalcLRC
    @li SetTime
    @li GetTIme
	@li	DispXY
	@li	Disp_deg_buf
*/

//************************************************************************************

#include <string.h>
#include <stdlib.h>
#include "posapi.h"
#include "define.h"
#include "tools.h"

#define LINELENGTH 80           //配置文件中每行的长度

/*字符串左右去空函数 */

/*==*src源字符串，*dest目标字符串，type截取类型0-左去空1-右去空2-左右去空*/
int _StrTrim (char *dest, const char *src, int type)
{
    int             i;
    int             iOffsetLeft;
    int             iOffsetRight;
    int             len;

    iOffsetLeft = 0;
    iOffsetRight = 0;
    len = strlen (src);

    //计算源字符串中左边非空起始位置
    for (i = 0; i < len; i++)
    {
        if (src[i] != ' ')
        {
            iOffsetLeft = i;
            break;
        }
    }

    //源字符串为全空
    if (i == len)
    {
        dest[0] = '\0';
        return SUCC;
    }

    //计算源字符串中右边非空起始位置
    for (i = len - 1; i >= 0; i--)
    {
        if (src[i] != ' ')
        {
            iOffsetRight = i;
            break;
        }
    }


    switch (type)
    {
        case 0:                //左去空
            memcpy (dest, src + iOffsetLeft, len - iOffsetLeft);
            dest[len - iOffsetLeft] = '\0';
            break;
        case 1:                //右去空
            memcpy (dest, src, iOffsetRight + 1);
            dest[iOffsetRight + 1] = '\0';
            break;
        case 2:                //左右去空
            memcpy (dest, src + iOffsetLeft, iOffsetRight - iOffsetLeft + 1);
            dest[iOffsetRight - iOffsetLeft + 1] = '\0';
            break;
        default:
            return FAIL;
    }
    return SUCC;
}



/**
*	@fn 	int ltrim(char *dest, const char *src)
*	@brief	字符串去左空格
*	@param	src		[in] 源字符串
			dest	[out] 目标字符串
*	@return	@li SUCC	成功
			@li FAIL	失败	
*	@sa		define.h
*/
//************************************************************************************
int ltrim (char *dest, const char *src)
{
    return _StrTrim (dest, src, 0);
}


/**
*	@fn 	int rtrim(char *dest, const char *src)
*	@brief	字符串去右空格
*	@param	src		[in] 源字符串
			dest	[out] 目标字符串
*	@return	@li SUCC	成功
			@li FAIL	失败	
*	@sa		define.h
*/
//************************************************************************************
int rtrim (char *dest, const char *src)
{
    return _StrTrim (dest, src, 1);
}


/**
*	@fn 	int alltrim(char *dest, const char *src)
*	@brief	字符串去左右空格
*	@param	src		[in] 源字符串
			dest	[out] 目标字符串
*	@return	@li SUCC	成功
			@li FAIL	失败	
*	@sa		define.h
*/
//************************************************************************************
int alltrim (char *dest, const char *src)
{
    return _StrTrim (dest, src, 2);
}




/**
*	@fn 	int ReadLine (char *pcLineBuf, int iLineBufLen, int fd)
*	@brief	从文本文件中读一行字符串

*	@param	pcLineBuf		[out] 存放读取的字符串，最多读取iLineBufLen-1个字符
			iLineBufLen		[in] pcLineBuf的长度
			fd				[in] 文件句柄
*	@return	@li SUCC	成功
			@li FAIL	失败	
*	@sa		define.h
*/
//************************************************************************************
int ReadLine (char *pcLineBuf, int iLineBufLen, int fd)
{
    int             iRet, i;
    char            rgcTemp[2];

    // 读一行字符串
    i = 0;
    while (1)
    {
        if (i > iLineBufLen - 1)
        {
            break;
        }
        iRet = fread (rgcTemp, 1, fd);
        if (iRet == FAIL)
        {
            return FAIL;
        }
        else if (iRet != 1)
        {
            break;
        }
        else if ((rgcTemp[0] == '\r') || (rgcTemp[0] == '\n'))
        {
            break;
        }
        else
        {
            pcLineBuf[i++] = rgcTemp[0];
        }
    }
    pcLineBuf[i] = 0;
    return SUCC;
}



/**
*	@fn 	int WriteLine(const char *pcLineBuf, int fd)
*	@brief	向文本文件中写入一行字符串
*	@param	pcLineBuf		[in] 写入的字符串（以0x00结尾,最多80个字符)
			fd				[in] 文件句柄
*	@return	@li SUCC	成功
			@li FAIL	失败	
*	@sa		define.h
*/
//************************************************************************************
int WriteLine (const char *pcLineBuf, int iLineBufLen, int fd)
{
    int             iRet;
    char            rgcTemp[LINELENGTH + 2];
    int             iLen;

    if (iLineBufLen > LINELENGTH)
    {
        iLen = LINELENGTH;
    }
    else
    {
        iLen = iLineBufLen;
    }

    memset (rgcTemp, ' ', sizeof (rgcTemp));
    memcpy (rgcTemp, pcLineBuf, iLen);
    rgcTemp[LINELENGTH] = '\n';

    iRet = fwrite (rgcTemp, LINELENGTH + 1, fd);
    if (iRet != LINELENGTH + 1)
    {
        return FAIL;
    }
    else
    {
        return SUCC;
    }
}


/**
*	@fn 	ReadItemFromINI (const char *inifile, const char *item, char *value)
*	@brief	从配置文件中读取参数
*	@param	inifile		[in] 配置文件名
*			item 		[in] 项目名 
*           value		[out]项目的值
*	@return	@li 0   成功
			@li <0  失败	
*	@sa		tools.h
*/
//************************************************************************************
int ReadItemFromINI (const char *inifile, const char *item, char *value)
{
    char            buf[LINELENGTH + 2];
    char            tmpbuf[LINELENGTH + 2];
    char            itemname[LINELENGTH + 2];
    int             fd;
    int             ret;

    /* 生成 item=  */
    memcpy (itemname, item, strlen (item));
    itemname[strlen (item)] = '=';

    /* 打开INI文件 */
    if ((fd = fopen (inifile, "r")) == FAIL)
    {
        return ERR_TOOLS_INIFILE;
    }

    // 查找指定的项目
    while (1)
    {
        memset (tmpbuf, 0, sizeof (tmpbuf));
        memset (buf, 0, sizeof (buf));
        ret = ReadLine (tmpbuf, LINELENGTH, fd);
        alltrim (buf, tmpbuf);
        if (ret == FAIL)
        {
            fclose (fd);
            return ERR_TOOLS_ITEMNAME;
        }

        /* 找到匹配的item */
        if (memcmp (buf, itemname, strlen (itemname)) == 0)
        {
            strcpy (value, buf + strlen (itemname));
            return SUCC;
        }
    }
}


/**
*	@fn 	char CalcLRC(char *buf, int len)
*	@brief 	计算LRC
*	@param	buf     [in] 需要计算LRC的字符串
*	@return	@li	    计算得出的LRC
*	@sa	    tools.h	
*/
char CalcLRC (const char *buf, int len)
{
    int             i;
    char            ch;

    ch = 0x00;
    for (i = 0; i < len; i++)
    {
        ch ^= buf[i];
    }
    return (ch);
}





/*----蜂鸣次数-----------*/

/*----输入次数值---------*/
void Beep (int num)
{
    int             j = 0;

    while (1)
    {
        beep ();
        if (j++ == num)
            break;
    }
}

//1.2带警鸣的清屏信息显示，返回按键值 。使用API函数getkeycode().
int Beep_Msg (char *str, int beepnum, int waitetime)
{
    clrscr ();
    Beep (beepnum);
    printf ("%s", str);
    return getkeycode (waitetime);
}

//1.3成功信息清屏显示，鸣镝一下，等待时间两秒。
void SUCC_MSG (char *str)
{
    clrscr ();
    Beep (1);
    printf ("%s", str);
    getkeycode (3);
}

//1.4出错信息清屏显示，鸣镝三下，等待并返回按键
int ERR_MSG (char *str)
{
    clrscr ();
    Beep (3);
    printf ("\n%s", str);
    return (getkeycode (60));
}

//1.5清屏显示信息无等待
void MSG (char *str)
{
    clrscr ();
    printf ("%s", str);
 //   getkeycode(0) ;
}



/**
*	@fn 	int AsciiToBcd(unsigned char *bcd_buf, const unsigned char *ascii_buf, int len, char type)
*	@brief 	ASCII字符串转换为BCD字符串
*	@param  bcd_buf		[out] 转换输出的BCD字符串
	@param	ascii_buf	[in] 需要转换的ACSII字符串
	@param	len 	    [in] 输入数据长度(ASCII字符串的长度)
	@param	type		[in] 对齐方式  1－左对齐  0－右对齐
*	@return	@li	 0	成功
			@li  <0 失败(字符串中包含非法字符)
*	@sa	    tools.h	
*/
int AsciiToBcd (unsigned char *bcd_buf, const unsigned char *ascii_buf, int len, char type)
{
    int             cnt;
    char            ch, ch1;

    if (len & 0x01 && type)     /*判别是否为奇数以及往那边对齐 */
        ch1 = 0;
    else
        ch1 = 0x55;

    for (cnt = 0; cnt < len; ascii_buf++, cnt++)
    {
        if (*ascii_buf >= 'a')
            ch = *ascii_buf - 'a' + 10;
        else if (*ascii_buf >= 'A')
            ch = *ascii_buf - 'A' + 10;
        else if (*ascii_buf >= '0')
            ch = *ascii_buf - '0';
        else if (*ascii_buf == 0)
            ch = 0x0f;
        else
        {

            ch = (*ascii_buf);
            ch &= 0x0f;         //保留低四位
        }
        if (ch1 == 0x55)
            ch1 = ch;
        else
        {
            *bcd_buf++ = ch1 << 4 | ch;
            ch1 = 0x55;
        }
    }                           //for
    if (ch1 != 0x55)
        *bcd_buf = ch1 << 4;

    return 0;
}


/**
*	@fn 	int BcdToAscii(unsigned char *ascii_buf, const unsigned char *bcd_buf, int len, int type)
*	@brief 	BCD字符串转换为ASCII字符串
*	@param	ascii_buf	[out] 转换输出的ACSII字符串
	@param  bcd_buf		[in] 需要转换的BCD字符串
	@param	len 	    [in] 输出数据长度(ASCII字符串的长度)
	@param	type		[in] 对齐方式  1－左对齐  0－右对齐
*	@return	@li	 0	成功
*	@sa	    tools.h	
*/
int BcdToAscii (unsigned char *ascii_buf, const unsigned char *bcd_buf, int len, char type)
{
    int             cnt;
    unsigned char  *pBcdBuf;

    pBcdBuf = (unsigned char *) bcd_buf;

    if ((len & 0x01) && type)   /*判别是否为奇数以及往那边对齐 */
    {                           /*0左，1右 */
        cnt = 1;
        len++;
    }
    else
        cnt = 0;
    for (; cnt < len; cnt++, ascii_buf++)
    {
        *ascii_buf = ((cnt & 0x01) ? (*pBcdBuf++ & 0x0f) : (*pBcdBuf >> 4));
        *ascii_buf += ((*ascii_buf > 9) ? ('A' - 10) : '0');
    }
    *ascii_buf = 0;
    return 0;
}


/**
*	@fn 	int IntToC4(unsigned char *pbuf, unsigned int num)
*	@brief 	整型转换为4字节字符串（高位在前）
*	@param	pbuf     	[out] 转换输出的字符串
	@param  num 		[in] 需要转换的整型数
*	@return	@li	 0	成功
*	@sa	    tools.h	
*/
int IntToC4 (unsigned char *pbuf, unsigned int num)
{
    *pbuf = (unsigned char) (num >> 24);
    *(pbuf + 1) = (unsigned char) ((num >> 16) % 256);
    *(pbuf + 2) = (unsigned char) ((num >> 8) % 256);
    *(pbuf + 3) = (unsigned char) (num % 256);
    return 0;
}



/**
*	@fn 	int IntToC2(unsigned char *pbuf, unsigned int num)
*	@brief 	整型转换为2字节字符串（高位在前）
*	@param	pbuf     	[out] 转换输出的字符串
	@param  num 		[in] 需要转换的整型数
*	@return	@li	 0	成功
*	@sa	    tools.h	
*/
int IntToC2 (unsigned char *pbuf, unsigned int num)
{
    *(pbuf) = (unsigned char) (num >> 8);
    *(pbuf + 1) = (unsigned char) (num % 256);
    return 0;
}


/**
*	@fn 	int C4ToInt(unsigned int *num, unsigned char *pbuf)
*	@brief 	4字节字符串转换为整型（高位在前）
*	@param	num     	[out] 转换输出的整型数
	@param  num 		[in] 需要转换的字符串
*	@return	@li	 0	成功
*	@sa	    tools.h	
*/
int C4ToInt (unsigned int *num, unsigned char *pbuf)
{
    *num = ((*pbuf) << 24) + ((*(pbuf + 1)) << 16) + ((*(pbuf + 2)) << 8) + (*(pbuf + 3));
    return 0;
}


/**
*	@fn 	int C2ToInt(unsigned int *num, unsigned char *pbuf)
*	@brief 	2字节字符串转换为整型（高位在前）
*	@param	num     	[out] 转换输出的整型数
	@param  num 		[in] 需要转换的字符串
*	@return	@li	 0	成功
*	@sa	    tools.h	
*/
int C2ToInt (unsigned int *num, unsigned char *pbuf)
{
    *num = ((*pbuf) << 8) + (*(pbuf + 1));
    return 0;
}


/**
*	@fn 	char ByteToBcd(int num)
*	@brief 	整数(0-99)转换为一字节BCD
*	@param	num     [in] 需要转换的整型数(0-99)
*	@return	@li	 转换输出的BCD
*	@sa	    tools.h	
*/
char ByteToBcd (int num)
{
    return ((num / 10) << 4) | (num % 10);
}


/**
*	@fn 	int BcdToByte(char bcd)
*	@brief 	一字节BCD转换为整数(0-99)
*	@param	bcd     [in] 需要转换的bcd
*	@return	@li	 转换输出的整数
*	@sa	    tools.h	
*/
int BcdToByte (int num)
{
    return (num >> 4) * 10 + (num & 0x0f);
}


/**
*	@fn 	int IntToBcd(char *bcd, int num)
*	@brief 	整数(0-9999)转换为二字节BCD
*	@param  num		[in] 需要转换的整数(0-9999)
*	@param	bcd     [out] 转换输出的BCD(二字节)
*	@return	@li	 0 成功
*	@sa	    tools.h	
*/
int IntToBcd (char *bcd, int num)
{
    bcd[0] = ByteToBcd (num / 100);
    bcd[1] = ByteToBcd (num % 100);

    return 0;
}


/**
*	@fn 	int BcdToInt(char *bcd)
*	@brief 	二字节BCD转换为整数(0-9999)
*	@param	bcd     [in] 需要转换的BCD(二字节)
*	@return	@li	 转换输出的整数
*	@sa	    tools.h	
*/
int BcdToInt (const char *bcd)
{
    return BcdToByte (bcd[0]) * 100 + BcdToByte (bcd[1]);
}


/**
*	@fn 	GetTime
*	@brief 	从POS上取当前日期和时间
*	@param	(out)	char *date	返回的日期(yyyymmdd 8字节)
*	@param	(out)	char *time	返回的时间(hhmmss 6字节) 
*	@return	void
*	@sa	    tools.h	
*/
void GetTime (char *pchDate, char *pchTime)
{
    struct postime  strtime;

    getpostime (&strtime);

    BcdToAscii ((unsigned char *) (pchDate + 0), (unsigned char *) (&strtime.yearh), 2, 0);
    BcdToAscii ((unsigned char *) (pchDate + 2), (unsigned char *) (&strtime.yearl), 2, 0);
    BcdToAscii ((unsigned char *) (pchDate + 4), (unsigned char *) (&strtime.month), 2, 0);
    BcdToAscii ((unsigned char *) (pchDate + 6), (unsigned char *) (&strtime.day), 2, 0);
    BcdToAscii ((unsigned char *) (pchTime), (unsigned char *) (&strtime.hour), 2, 0);
    BcdToAscii ((unsigned char *) (pchTime + 2), (unsigned char *) (&strtime.minute), 2, 0);
    BcdToAscii ((unsigned char *) (pchTime + 4), (unsigned char *) (&strtime.second), 2, 0);
}



/**
*	@fn 	SetTime
*	@brief 	修改POS的日期和时间
*	@param	(in)	char *date	日期(yyyymmdd 8字节)
*	@param	(in)	char *time	时间(hhmmss 6字节) 
*	@return	void
*	@sa	    tools.h	
*/
void SetTime (const char *pchDate, const char *pchTime)
{
    struct postime  strtime;
    char            date[4];
    char            time[3];

    AsciiToBcd ((unsigned char *) date, (unsigned char *) pchDate, 8, 0);
    AsciiToBcd ((unsigned char *) time, (unsigned char *) pchTime, 6, 0);
    strtime.yearh = date[0];
    strtime.yearl = date[1];
    strtime.month = date[2];
    strtime.day = date[3];
    strtime.hour = time[0];
    strtime.minute = time[1];
    strtime.second = time[2];
    setpostime (strtime);
}

/**
*	@fn 	DispXY
*	@brief 	在指定(x,y)位置显示string的内容
*	@param	(in)	int x
*	@param	(in)	int y
*	@param	(in)	char *string
*	@return	void
*	@sa	    tools.h	
*/
void DispXY(int x, int y, char *string)
{
	gotoxy(x,y);
	printf("%s",string);
}


//以调试信息方式显示缓冲区
//不破坏原界面
/**
*	@fn 	Disp_deg_buf
*	@brief 	以调试信息方式显示缓冲区，不破坏原界面
*	@param	(in)	char *buf 需显示的字符串
*	@param	(in)	int buflen	字符串长度
*	@return	void
*	@sa	    tools.h	
*/
void Disp_deg_buf(char *buf,int buflen)
{
	int i;

	pushscreen();
	clrscr();
	for(i=1;i<=buflen;i++)
	{
		printf("%02x",*(buf+i-1));
		if(i%8==0)	printf("\n");
		if(i%32==0)
		{
			getkeycode(0);
			clrscr();
		}
	}
	getkeycode(0);
	popscreen();
}

int JudgeValue(char *buf,int min,int max)
{

	int tmp;
	
	tmp = atoi(buf);

	if((tmp>=min)&&(tmp<=max))
		return TRUE;
	else
		return FALSE;
}

void ChangePosDate()
{
	int tmp, tmp1;
	char sbuf[30],bufyear[6],bufmon[4],bufdate[4];
	char pchDate[8+1];
	char pchTime[6+1];
	while(1)
	{
		memset(pchDate, 0, sizeof(pchDate));
		memset(pchTime, 0, sizeof(pchTime));
		GetTime(pchDate, pchTime);
		
		clrscr();
		DispString(0,0,"当前日期:");
		printf("\n%4.4s年%2.2s月%2.2s日",pchDate, pchDate+4, pchDate+6);
		DispString(0,32,"输入日期:");

		// 设置年份
		while(1)
		{
			gotoxy(0,6);
			printf("    年  月  日");
			
			gotoxy(0,7);
			memset(bufyear, 0, sizeof(bufyear));
			if((tmp=getnumstr(bufyear,4,NORMAL,0))<0) //ESC
			{
				return;
			}
			
			if(tmp==0)
			{
				memcpy(bufyear, pchDate, 4);
				break;
			}
			else if(tmp==4)
			{
				if(JudgeValue(bufyear,1990,2189)==TRUE)
				{
					break;
				}
			}
		}

		
		// 设置月份
		while(1)
		{   
			sprintf(sbuf,"%s年  月  日",bufyear);
			gotoxy(0,6);
			printf("%s\n",sbuf);
			
			gotoxy(6,7);
			memset(bufmon, 0, sizeof(bufmon));
			if( (tmp=getnumstr(bufmon,2,NORMAL,0))<0)  //ESC键
				return;

			if(tmp==0)	//Enter键
			{
				memcpy(bufmon, pchDate+4, 2);
				break;
			}
			else if(tmp==2)
			{
				if(JudgeValue(bufmon,1,12)==TRUE)
				{
					break;
				}
			}
		}
		
		
		// 设置日期
		while(1)
		{   
			sprintf(sbuf,"%s年%s月  日",bufyear,bufmon);
			gotoxy(0,6);
			printf("%s\n",sbuf);
			
			gotoxy(10,7);
			memset(bufdate, 0, sizeof(bufdate));
			if( (tmp=getnumstr(bufdate,2,NORMAL,0))<0)  //ESC键
				return;
			
			if(tmp==0)	//Enter键
			{
				memcpy(bufdate, pchDate+6, 2);
				break;
			}
			else if(tmp == 2)
			{
			
				switch(atoi(bufmon))
				{
					case 1:
					case 3:
					case 5:
					case 7:
					case 8:
					case 10:
					case 12:
						tmp1=JudgeValue(bufdate,1,31);
						break;
					case 4:
					case 6:
					case 9:
					case 11:
						tmp1=JudgeValue(bufdate,1,30);
						break;
					case 2:
						if(atoi(bufyear)%4==0)
							tmp1=JudgeValue(bufdate,1,29);
						else
							tmp1=JudgeValue(bufdate,1,28);
						break;
				}
				if(tmp1 == TRUE)
				{
					break;
				}
			}
		}

		// 修改年、月、日，时、分、秒为当前值
		memcpy(pchDate, bufyear, 4);
		memcpy(pchDate+4, bufmon, 2);
		memcpy(pchDate+6, bufdate, 2);
		SetTime(pchDate, pchTime);
	}	
}

void ChangePosTime()
{
	int tmp;
	char sbuf[30];
	char bufhour[4],bufmin[4],bufsec[4];
	char pchDate[8+1];
	char pchTime[6+1];
	
	while(1)
	{

		memset(pchDate, 0, sizeof(pchDate));
		memset(pchTime, 0, sizeof(pchTime));
		GetTime(pchDate, pchTime);

		//.........................................................
		clrscr();
		DispString(0,0,"当前时间:");
		sprintf(sbuf,"%2.2s:%2.2s:%2.2s",pchTime,pchTime+2,pchTime+4);
		printf("\n%s",sbuf);
		DispString(0,32,"输入时间:");

		// 设置小时
		while(1)
		{   
			gotoxy(0,6);     
			printf("  :  :  ");
			
			gotoxy(0,7);
			memset(bufhour, 0, sizeof(bufhour));
			if( (tmp=getnumstr(bufhour,2,NORMAL,0))<0)  //ESC键
				return;

			if(tmp==0)	//Enter键
			{
				memcpy(bufhour, pchTime, 2);
				break;
			}
			else if(tmp == 2)
			{
				if(JudgeValue(bufhour,0, 23)==TRUE)
				{
					break;
				}
			}
		} 

		// 设置分钟
		while(1)
		{
			sprintf(sbuf,"%s:  :  ",bufhour);
			gotoxy(0,6);
			printf("%s\n",sbuf);
			
			gotoxy(3,7);
			memset(bufmin, 0, sizeof(bufmin));
			if( (tmp=getnumstr(bufmin,2,NORMAL,0))<0)  //ESC键
				return;

			if(tmp==0)	//Enter键
			{
				memcpy(bufmin, pchTime+2, 2);
				break;
			}
			else if(tmp==2)
			{
				if(JudgeValue(bufmin,0,59)==TRUE)
				{
					break;
				}
			}
		}         	        	
		
		// 设置秒钟
		while(1)
		{    
			sprintf(sbuf,"%s:%s:  ",bufhour,bufmin);
			gotoxy(0,6);
			printf("%s\n",sbuf);
			
			gotoxy(6,7);
			memset(bufsec, 0, sizeof(bufsec));
			if( (tmp=getnumstr(bufsec, 2, NORMAL,0))<0)  //ESC键
				return;
			if(tmp==0)	//Enter键
			{
				memcpy(bufsec, pchTime+4, 2);
				break;
			}
			else if(tmp==2)
			{
				if(JudgeValue(bufsec,0,59)==TRUE)
				{
					break;
				}
			}
		}      	        	
		

		// 修改年、月、日，时、分、秒为当前值
		memcpy(pchTime, bufhour, 2);
		memcpy(pchTime+2, bufmin, 2);
		memcpy(pchTime+4, bufsec, 2);
		SetTime(pchDate, pchTime);
	}
}

void ChangeDateTime(void)
{
	int kc;

	clrscr();
	do
	{
		gotoxy(1,1); printf("1. 修改日期");
		gotoxy(1,3); printf("2. 修改时间");
		gotoxy(2,6); printf("ESC...退出");

		kc=getkeycode(0);
		if(kc=='1'||kc=='2')
		{
			if(kc=='1')
				ChangePosDate();
			else
				ChangePosTime();
			clrscr();
		}
	}while(kc!=ESC);
}

