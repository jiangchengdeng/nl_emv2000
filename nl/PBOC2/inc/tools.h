#ifndef _TOOLS_H_
#define _TOOLS_H_

#define ERR_TOOLS_BASE -1000
#define ERR_TOOLS_INIFILE  	ERR_TOOLS_BASE-1   		//配置文件不存在
#define ERR_TOOLS_SEGNAME  	ERR_TOOLS_BASE-2		//段落不存在
#define ERR_TOOLS_ITEMNAME 	ERR_TOOLS_BASE-3		//项目不存在

extern int  AsciiToBcd (unsigned char *bcd_buf, const unsigned char *ascii_buf, int len, char type);
extern int  BcdToAscii(unsigned char *ascii_buf, const unsigned char *bcd_buf, int len, char type);
extern int IntToC4 (unsigned char *pbuf, unsigned int num);
extern int IntToC2 (unsigned char *pbuf, unsigned int num);
extern int C4ToInt(unsigned int *num, unsigned char *pbuf);
extern int C2ToInt (unsigned int *num, unsigned char *pbuf);
extern int BcdToByte(int num);
extern int BcdToInt(const char *bcd);
extern char ByteToBcd(int num);
extern int IntToBcd(char *bcd, int num);
extern char CalcLRC(const char *buf, int len);
extern int ReadLine(char *pcLineBuf, int iLineBufLen, int fd);
extern int WriteLine (const char *pcLineBuf, int iLineBufLen, int fd);
extern int ReadItemFromINI (const char *inifile, const char *item, char *value);
extern int rtrim(char *dest, const char *src);
extern int ltrim(char *dest, const char *src);
extern int alltrim(char *dest, const char *src);

extern void Beep(int num);
extern int  Beep_Msg(char * str,int beepnum,int waitetime);
extern int  ERR_MSG(char * str);
extern void MSG(char * str);
extern void SUCC_MSG(char * str);

extern void SetTime (const char *pchDate, const char *pchTime);
extern void GetTime (char *pchDate, char *pchTime);

extern void DispXY(int x, int y, char *string);
extern void Disp_deg_buf(char *buf,int buflen);

#endif
