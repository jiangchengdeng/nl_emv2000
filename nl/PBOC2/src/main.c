#include	<string.h>
#include	<stdlib.h>

#include	<posapi.h>
#include	<define.h>
#include	"capk.h"
#include	"ic.h"
#include	"appselect.h"
#include 	"posdef.h"
#include	"tools.h"
#include  "tvrtsi.h"
#include 	"AppData.h"
#include 	"AppInit.h"
#include 	"ReadAppData.h"
#include 	"processlimit.h"
#include  "CardVerify.h"
#include  "termrisk.h"
#include  "TermActAnal.h"
#include  "OnlineProc.h"
#include  "Completion.h"
#include  "IssuerScript.h"
#include  "offauth.h"
#include  "tlv.h"
#include  "des.h"

#define LOGFILE "proc.log"
#define LOGLONG 64

TTermConfig g_termconfig;		//终端配置
TTermParam  g_termparam;		//终端参数
int iNeedSignature ;
int iReadCardRecord = 0 ;
char iAcceptForced =0;

/**
* @fn magstrip_proc
* @brief 磁卡处理过程

* @return QUIT(-2)	退出
		  FAIL(-1)	失败
		  TIMEOUT(-3) 超时
		  INSERTIC(2) 插入IC卡
		  STRIPE(1)	  刷卡
*/
int magstrip_proc(int flag)
{
	int ret,i;
	char srvcode[2];	
	char len;
	int pan_len;
	char tk2[120];
	char tk3[120];
	char pan[40];
	char expdate[4];
	char pin[13];
	int amount;
	char pinbcd[6];
	char buff[128];

	while(1)
	{
		clrscr();
		printf("\n请刷卡\n========>>");
		memset(tk2, 0, sizeof(tk2));
		memset(tk3, 0, sizeof(tk3));
		memset(pan, 0, sizeof(pan));
		memset(expdate, 0, sizeof(expdate));
		ret=readcard(STRIPE, TK2, 20, 0, tk2, tk3);
		switch(ret)
		{
		case STRIPE:
			beep();
			break;
		case QUIT:
			return QUIT;
		case FAIL:
			ERR_MSG("   刷卡错误!");
			return FAIL;
		default:
			continue;
		}	
		break;
	}
	
	if(tk2[0]==0x7f)
	{
		ERR_MSG("   磁道无数据!");
		return FAIL;
	}

	for(i=0;i<38;i++)
	{
		if(tk2[i]=='=')	
		{
			len = i;
			break;
		}
	}
	
	if(i>=38)	
	{
		ERR_MSG(" 磁道数据错误!");
		return FAIL;
	}

	AsciiToBcd((unsigned char *)pan, (unsigned char *)tk2, len, 0);			//帐号
	AsciiToBcd((unsigned char *)expdate, (unsigned char *)tk2+len+1, 4, 0); //有效期
	AsciiToBcd((unsigned char *)srvcode, (unsigned char *)tk2+len+5, 3, 1); //服务代码
	pan_len = len/2;
	//显示帐号，有效期
	clrscr();
	printf("帐号: ");
	for(i=0; i<len/2; i++)
	{
		printf("%02x", pan[i]);
	}
	printf("\n有效期: ");
	for(i=0; i<2; i++)
	{
		printf("%02x", expdate[i]);
	}
	printf("\n");

	if(flag == 1)
	{
		if(srvcode[0]==0x02 || srvcode[0]==0x06)
		{
			printf("请插入IC卡!");
			getkeycode(0);
			return STRIPE;
		}
	}

	amount = 0;
	amount = InputAmount();
	if(amount < 0)
	{
		return FAIL;
	}

	while(1)
	{
		clrscr();
		memset(pin,0,sizeof(pin));
		printf("请输入密码:\n");
		gotoxy(4,5);
		ret=getnumstr(pin,12,PASSWD,0);
		if(ret==QUIT)	return FAIL;
		if(ret<4)
		{
			ERR_MSG("\n  密码长度错误");
			continue;
		}
		break;
	}
	memset(pinbcd, 0, sizeof(pinbcd));
	AsciiToBcd((unsigned char *)pinbcd, (unsigned char *)pin, strlen(pin), 0);

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%012d", amount);
	AsciiToBcd((unsigned char *)buff+12, (unsigned char *)buff, 12, 0);
	SetAppData("\x9f\x02", buff+12, 6);
	
	SetAppData("\x9C", "\x00", 1) ; //交易类型
	SetAppData("\x5A", pan, pan_len); //帐号
	SetAppData("\x9f\x39", "\x02", 1);

	clrscr();
	printf("正在上送磁卡交易");

	memset(buff, 0, sizeof(buff));
	Des((uchar *)pinbcd, (uchar *)buff, DESKEY);
	ret = Online(7, buff);

	if(ret == SUCC)
	{
		printf("\n  交易上送成功");
	}
	else
	{
		printf("\n  交易上送失败");
	}
	getkeycode(3);
	
	return SUCC;
	
}

/*
int manual_proc(void)
{
	char pin[16];
	int ret;
	
	POS_Enter_Mode=0x06;
	if(Get_PAN()==FAIL)	return FAIL;
	if(Get_amount()==FAIL)	return FAIL;
	while(1)
	{
		clrscr();
		memset(pin,0,sizeof(pin));
		printf("  Please Input    Password:\n");
		gotoxy(4,5);
		ret=getnumstr(pin,12,PASSWD,0);
		if(ret==QUIT)	return FAIL;
		if(ret<4)
		{
			ERR_MSG("\n  Length Of Password Is Wrong!\n  Try Again...");
			continue;
		}
		if(pin_check(pin,ret)==FAIL)
		{
			ERR_MSG("\n  Invalid Character In Password!");
			continue;
		}
		pin[12]=0;
		memset(P_PIN,0xff,sizeof(P_PIN));
		AsciiToBcd(P_PIN,pin,16,0);
		onlinepin=1;
		break;
	}
	if(Online(7)==FAIL)	return FAIL;
	clrscr();
	printf("  Transaction\n   Complete!");
	if(!memcmp(Auth_Res_Code,"00",2))	BlackPrintf(0,4,"    Approve!    ");
	else	BlackPrintf(0,4,"    Decline!    ");
	printf("\nPress Enter...");
	getkeycode(0);
	return SUCC;
}

*/
//初始化CA公钥文件
//成功
int InitCAPK(void)
{
	int fp;
	int ret;
	TCAPK capk;

	//判断是否存在公钥文件，如果存在的话退出
	fp = fopen(FILE_CAPK, "r");
	if (fp >= 0)
	{
		fclose(fp);
		return SUCC;
	}

	ret = 1 ;
	savevar((char *)ret, TSC_OFF, TSC_LEN) ;
	ret = 0 ;
	savevar((char *)ret, TERM_PARAMNUM_OFF, TERM_PARAMNUM_LEN) ;
	
	clrscr();
	printf("\n  初始化CA公钥\n  请稍候...");

	//初始化公钥文件
	ret = CAPK_Init();
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}
	
	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x90;
	capk.Len = 0x40;
	memcpy(capk.Value, "\xC2\x6B\x3C\xB3\x83\x3E\x42\xD8\x27\x0D\xC1\x0C\x89\x99\xB2\xDA\x18\x10\x68\x38\x65\x0D\xA0\xDB\xF1\x54\xEF\xD5\x11\x00\xAD\x14\x47\x41\xB2\xA8\x7D\x68\x81\xF8\x63\x0E\x33\x48\xDE\xA3\xF7\x80\x38\xE9\xB2\x1A\x69\x7E\xB2\xA6\x71\x6D\x32\xCB\xF2\x60\x86\xF1", 0x40);
	memcpy(capk.Exponent, "\x00\x00\x03",3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x94;
	capk.Len = 0xf8;
	memcpy(capk.Value, "\xD1\xBE\x39\x61\x5F\x39\x5A\xC9\x33\x7E\x33\x07\xAA\x5A\x7A\xC3\x5E\xAE\x00\x36\xBF\x20\xB9\x2F\x9A\x45\xD1\x90\xB2\xF4\x61\x6A\xBF\x9D\x34\x0C\xBF\x5F\xBB\x3A\x2B\x94\xBD\x8F\x2F\x97\x7C\x0A\x10\xB9\x0E\x59\xD4\x20\x1A\xA3\x26\x69\xE8\xCB\xE7\x53\xF5\x36\x11\x9D\xF4\xFB\x5E\x63\xCE\xD8\x7F\x11\x53\xCE\x91\x4B\x12\x4F\x3E\x6B\x64\x8C\xD5\xC9\x76\x55\xF7\xAB\x4D\xF6\x26\x07\xC9\x5D\xA5\x05\x17\xAB\x8B\xE3\x83\x66\x72\xD1\xC7\x1B\xCD\xE9\xBA\x72\x93\xFF\x34\x82\xF1\x24\xF8\x66\x91\x13\x0A\xB0\x81\x77\xB0\x2F\x45\x9C\x02\x5A\x1F\x3D\xFF\xE0\x88\x4C\xE7\x81\x22\x54\x2E\xA1\xC8\xEA\x09\x2B\x55\x2B\x58\x69\x07\xC8\x3A\xD6\x5E\x0C\x6F\x91\xA4\x00\xE4\x85\xE1\x11\x92\xAA\x4C\x17\x1C\x5A\x1E\xF5\x63\x81\xF4\xD0\x91\xCC\x7E\xF6\xBD\x86\x04\xCB\xC4\xC7\x4D\x5D\x77\xFF\xA0\x7B\x64\x1D\x53\x99\x8C\xDB\x5C\x21\xB7\xBC\x65\xE0\x82\xA6\x51\x3F\x42\x4A\x4B\x25\x2E\x0D\x77\xFA\x40\x56\x98\x6A\x0A\xB0\xCD\xA6\x15\x5E\xD9\xA8\x83\xC6\x9C\xC2\x99\x2D\x49\xEC\xBD\x47\x97\xDD\x28\x64\xFF\xC9\x6B\x8D",0xf8);
	memcpy(capk.Exponent, "\x01\x00\x01",3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x95;
	capk.Len = 0x90;
	memcpy(capk.Value, "\xBE\x9E\x1F\xA5\xE9\xA8\x03\x85\x29\x99\xC4\xAB\x43\x2D\xB2\x86\x00\xDC\xD9\xDA\xB7\x6D\xFA\xAA\x47\x35\x5A\x0F\xE3\x7B\x15\x08\xAC\x6B\xF3\x88\x60\xD3\xC6\xC2\xE5\xB1\x2A\x3C\xAA\xF2\xA7\x00\x5A\x72\x41\xEB\xAA\x77\x71\x11\x2C\x74\xCF\x9A\x06\x34\x65\x2F\xBC\xA0\xE5\x98\x0C\x54\xA6\x47\x61\xEA\x10\x1A\x11\x4E\x0F\x0B\x55\x72\xAD\xD5\x7D\x01\x0B\x7C\x9C\x88\x7E\x10\x4C\xA4\xEE\x12\x72\xDA\x66\xD9\x97\xB9\xA9\x0B\x5A\x6D\x62\x4A\xB6\xC5\x7E\x73\xC8\xF9\x19\x00\x0E\xB5\xF6\x84\x89\x8E\xF8\xC3\xDB\xEF\xB3\x30\xC6\x26\x60\xBE\xD8\x8E\xA7\x8E\x90\x9A\xFF\x05\xF6\xDA\x62\x7B",0x90);
	memcpy(capk.Exponent, "\x00\x00\x03",3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x96;
	capk.Len = 0x80;
	memcpy(capk.Value, "\xB7\x45\x86\xD1\x9A\x20\x7B\xE6\x62\x7C\x5B\x0A\xAF\xBC\x44\xA2\xEC\xF5\xA2\x94\x2D\x3A\x26\xCE\x19\xC4\xFF\xAE\xEE\x92\x05\x21\x86\x89\x22\xE8\x93\xE7\x83\x82\x25\xA3\x94\x7A\x26\x14\x79\x6F\xB2\xC0\x62\x8C\xE8\xC1\x1E\x38\x25\xA5\x6D\x3B\x1B\xBA\xEF\x78\x3A\x5C\x6A\x81\xF3\x6F\x86\x25\x39\x51\x26\xFA\x98\x3C\x52\x16\xD3\x16\x6D\x48\xAC\xDE\x8A\x43\x12\x12\xFF\x76\x3A\x7F\x79\xD9\xED\xB7\xFE\xD7\x6B\x48\x5D\xE4\x5B\xEB\x82\x9A\x3D\x47\x30\x84\x8A\x36\x6D\x33\x24\xC3\x02\x70\x32\xFF\x8D\x16\xA1\xE4\x4D\x8D",0x80);
	memcpy(capk.Exponent, "\x00\x00\x03", 3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x97;
	capk.Len = 0x60;
	memcpy(capk.Value, "\xAF\x07\x54\xEA\xED\x97\x70\x43\xAB\x6F\x41\xD6\x31\x2A\xB1\xE2\x2A\x68\x09\x17\x5B\xEB\x28\xE7\x0D\x5F\x99\xB2\xDF\x18\xCA\xE7\x35\x19\x34\x1B\xBB\xD3\x27\xD0\xB8\xBE\x9D\x4D\x0E\x15\xF0\x7D\x36\xEA\x3E\x3A\x05\xC8\x92\xF5\xB1\x9A\x3E\x9D\x34\x13\xB0\xD9\x7E\x7A\xD1\x0A\x5F\x5D\xE8\xE3\x88\x60\xC0\xAD\x00\x4B\x1E\x06\xF4\x04\x0C\x29\x5A\xCB\x45\x7A\x78\x85\x51\xB6\x12\x7C\x0B\x29",0x60);
	memcpy(capk.Exponent, "\x00\x00\x03", 3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x98;
	capk.Len = 0x70;
	memcpy(capk.Value, "\xCA\x02\x6E\x52\xA6\x95\xE7\x2B\xD3\x0A\xF9\x28\x19\x6E\xED\xC9\xFA\xF4\xA6\x19\xF2\x49\x2E\x3F\xB3\x11\x69\x78\x9C\x27\x6F\xFB\xB7\xD4\x31\x16\x64\x7B\xA9\xE0\xD1\x06\xA3\x54\x2E\x39\x65\x29\x2C\xF7\x78\x23\xDD\x34\xCA\x8E\xEC\x7D\xE3\x67\xE0\x80\x70\x89\x50\x77\xC7\xEF\xAD\x93\x99\x24\xCB\x18\x70\x67\xDB\xF9\x2C\xB1\xE7\x85\x91\x7B\xD3\x8B\xAC\xE0\xC1\x94\xCA\x12\xDF\x0C\xE5\xB7\xA5\x02\x75\xAC\x61\xBE\x7C\x3B\x43\x68\x87\xCA\x98\xC9\xFD\x39",0x70);
	memcpy(capk.Exponent, "\x00\x00\x03", 3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x99;
	capk.Len = 0x80;
	memcpy(capk.Value, "\xAB\x79\xFC\xC9\x52\x08\x96\x96\x7E\x77\x6E\x64\x44\x4E\x5D\xCD\xD6\xE1\x36\x11\x87\x4F\x39\x85\x72\x25\x20\x42\x52\x95\xEE\xA4\xBD\x0C\x27\x81\xDE\x7F\x31\xCD\x3D\x04\x1F\x56\x5F\x74\x73\x06\xEE\xD6\x29\x54\xB1\x7E\xDA\xBA\x3A\x6C\x5B\x85\xA1\xDE\x1B\xEB\x9A\x34\x14\x1A\xF3\x8F\xCF\x82\x79\xC9\xDE\xA0\xD5\xA6\x71\x0D\x08\xDB\x41\x24\xF0\x41\x94\x55\x87\xE2\x03\x59\xBA\xB4\x7B\x75\x75\xAD\x94\x26\x2D\x4B\x25\xF2\x64\xAF\x33\xDE\xDC\xF2\x8E\x09\x61\x5E\x93\x7D\xE3\x2E\xDC\x03\xC5\x44\x45\xFE\x7E\x38\x27\x77", 0x80);
	memcpy(capk.Exponent, "\x00\x00\x03", 3);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x03;
	capk.Len = 0x70;
	memcpy(capk.Value, "\xB3\xE5\xE6\x67\x50\x6C\x47\xCA\xAF\xB1\x2A\x26\x33\x81\x93\x50\x84\x66\x97\xDD\x65\xA7\x96\xE5\xCE\x77\xC5\x7C\x62\x6A\x66\xF7\x0B\xB6\x30\x91\x16\x12\xAD\x28\x32\x90\x9B\x80\x62\x29\x1B\xEC\xA4\x6C\xD3\x3B\x66\xA6\xF9\xC9\xD4\x8C\xED\x8B\x4F\xC8\x56\x1C\x8A\x1D\x8F\xB1\x58\x62\xC9\xEB\x60\x17\x8D\xEA\x2B\xE1\xF8\x22\x36\xFF\xCF\xF4\xF3\x84\x3C\x27\x21\x79\xDC\xDD\x38\x4D\x54\x10\x53\xDA\x6A\x6A\x0D\x3C\xE4\x8F\xDC\x2D\xC4\xE3\xE0\xEE\xE1\x5F",0x70);
	memcpy(capk.Exponent, "\x00\x00\x03",3);
	memcpy(capk.Hash, "\xFE\x70\xAB\x3B\x4D\x5A\x1B\x99\x24\x22\x8A\xDF\x80\x27\xC7\x58\x48\x3A\x8B\x7E",20);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x01;
	capk.Len = 0x80;
	memcpy(capk.Value, "\xC6\x96\x03\x42\x13\xD7\xD8\x54\x69\x84\x57\x9D\x1D\x0F\x0E\xA5\x19\xCF\xF8\xDE\xFF\xC4\x29\x35\x4C\xF3\xA8\x71\xA6\xF7\x18\x3F\x12\x28\xDA\x5C\x74\x70\xC0\x55\x38\x71\x00\xCB\x93\x5A\x71\x2C\x4E\x28\x64\xDF\x5D\x64\xBA\x93\xFE\x7E\x63\xE7\x1F\x25\xB1\xE5\xF5\x29\x85\x75\xEB\xE1\xC6\x3A\xA6\x17\x70\x69\x17\x91\x1D\xC2\xA7\x5A\xC2\x8B\x25\x1C\x7E\xF4\x0F\x23\x65\x91\x24\x90\xB9\x39\xBC\xA2\x12\x4A\x30\xA2\x8F\x54\x40\x2C\x34\xAE\xCA\x33\x1A\xB6\x7E\x1E\x79\xB2\x85\xDD\x57\x71\xB5\xD9\xFF\x79\xEA\x63\x0B\x75",0x80);
	memcpy(capk.Exponent, "\x00\x00\x03",3);
	memcpy(capk.Hash, "\xD3\x4A\x6A\x77\x60\x11\xC7\xE7\xCE\x3A\xEC\x5F\x03\xAD\x2F\x8C\xFC\x55\x03\xCC",20);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}

	memset(&capk, 0, sizeof(capk));
	capk.HashAlgorithm = 0x01;
	capk.PkAlgorithm = 0x01;
	capk.Index = 0x07;
	capk.Len = 0x90;
	memcpy(capk.Value, "\xA8\x9F\x25\xA5\x6F\xA6\xDA\x25\x8C\x8C\xA8\xB4\x04\x27\xD9\x27\xB4\xA1\xEB\x4D\x7E\xA3\x26\xBB\xB1\x2F\x97\xDE\xD7\x0A\xE5\xE4\x48\x0F\xC9\xC5\xE8\xA9\x72\x17\x71\x10\xA1\xCC\x31\x8D\x06\xD2\xF8\xF5\xC4\x84\x4A\xC5\xFA\x79\xA4\xDC\x47\x0B\xB1\x1E\xD6\x35\x69\x9C\x17\x08\x1B\x90\xF1\xB9\x84\xF1\x2E\x92\xC1\xC5\x29\x27\x6D\x8A\xF8\xEC\x7F\x28\x49\x20\x97\xD8\xCD\x5B\xEC\xEA\x16\xFE\x40\x88\xF6\xCF\xAB\x4A\x1B\x42\x32\x8A\x1B\x99\x6F\x92\x78\xB0\xB7\xE3\x31\x1C\xA5\xEF\x85\x6C\x2F\x88\x84\x74\xB8\x36\x12\xA8\x2E\x4E\x00\xD0\xCD\x40\x69\xA6\x78\x31\x40\x43\x3D\x50\x72\x5F",0x90);
	memcpy(capk.Exponent, "\x00\x00\x03",3);
	memcpy(capk.Hash, "\xB4\xBC\x56\xCC\x4E\x88\x32\x49\x32\xCB\xC6\x43\xD6\x89\x8F\x6F\xE5\x93\xB1\x72",20);
	ret = CAPK_Save(&capk);
	if(ret != CAPKERR_NONE)
	{
		return FAIL;
	}
	
	return SUCC;
}


/**
* @fn GetTermPara
* @brief 从文件中读取终端配置信息

* @param void
* @return void
*/
int GetTermData(char * file)
{
	int fp;
	int len;
	
	//打开终端参数文件
//	clrscr() ;printf(file) ; getkeycode(0) ;
	fp = fopen(file, "r");
	if(fp < 0)
	{
		return FAIL;
	}

	//读取终端参数
	len = fread((char *)&g_termparam, sizeof(TTermParam), fp);
	if(len != sizeof(TTermParam))
	{
		fclose(fp);
		return FAIL;
	}
	fclose(fp);

	//打开终端配置文件
	fp = fopen(TermConfigFile, "r");
	if(fp < 0)
	{
		return FAIL;
	}

	//读取终端配置
	len = fread((char *)&g_termconfig, sizeof(TTermConfig), fp);
	if(len != sizeof(TTermConfig))
	{
		fclose(fp);
		return FAIL;
	}

	fclose(fp);

	return SUCC;
}


/**
* @fn deal_amount
* @brief 将金额数据转化成可显示或打印的数据

* @param (in) 	char *buffer	12字节的金额字符串
* @param (out)	char *pAmount	输出的可显示金额字符串
* @return void
*/
void deal_amount(char *buffer, char *pAmount)
{
	int i,j,flag=0;

	memset(pAmount,' ',14);
	memset(pAmount+9,'0',4);
	for(i=0,j=0;i<12;i++,j++)
	{
		switch(buffer[i])
		{
			case 'D':
				pAmount[j]='-';
				break;
			case 'C':/*'C'*/
				pAmount[j]='+';
				break;
			case '0':
				if(flag!=0)//'0'不在首位
					pAmount[j]=buffer[i];
				break;
			default:flag=1;
				pAmount[j]=buffer[i];
		}
		if(i==9)
		{
			j++;
			pAmount[j]='.';
		}
	}
	pAmount[13]=0;
}


/**
* @fn AppSelect
* @brief 选择应用

* @param (in) 	int  selectflag     应用选择标志(TRUE-第一次选择, FALSE-多次选择)
* @param (in) 	TAIDList aidlist[]	应用列表
* @param (in) 	aidnum				应用数
* @param (out)	char *pdol 			PDOL
* @param (out)	char *pdol_len		PDOL的长度
* @return 成功 返回选择的应用编号(0下标)
		  失败 FAIL(-1)
		  放弃 QUIT(-2)
*/
int AppSelect(int selectflag, TAIDList aidlist[], int aidnum, char *pdol, int *pdol_len)
{
	int i;
	int j;
	int ret;
	int choice;
	char Value[256];
	int Len;

	//第一次选择应用时，卡上只有一个应用，同时不需要持卡人确认，直接选择应用
	if((aidnum == 1) && selectflag )
	{
		if(aidlist[0].AppPriorityID & 0x80)
		{
			clrscr();
			for (j=0; j<aidlist[0].AppPreferedNameLen; j++)
			{
				printf("%c", aidlist[0].AppPreferedName[j]);
			}
			printf("\n");
			printf("Enter--确认\n");
			printf("ESC----放弃");
			while(1)
			{
				ret = getkeycode(0);
				if(ret == ENTER)
				{
					choice = 1;
					break;
				}
				if(ret == ESC)
				{
					return QUIT;
				}
			}
		}
		else
		{
			choice = 1;
		}
	}
	else
	{
		clrscr();
		printf("请选择应用:\n");
		for (i=1; i<=aidnum; i++)
		{
			printf("%d.", i);
			for (j=0; j<aidlist[i-1].AppPreferedNameLen; j++)
			{
				printf("%c", aidlist[i-1].AppPreferedName[j]);
			}
			printf("\n");
		}
		
		do{
			choice = getkeycode(0);
			if(choice == ESC)	//按ESC放弃退出
			{
				return QUIT;
			}
			choice = choice - 0x30;
		}while(choice <1 || choice> aidnum );
	}
	
	ret = IC_SelectADF(aidlist[choice-1].AID, aidlist[choice-1].AIDLen, SELECTMODE_FIRST, pdol, pdol_len);
	if (iReadCardRecord)
	{
		//读IC卡内交易日志
		memset(Value, 0, sizeof(Value));
		Len = 0;
		TLV_GetValue("\x9f\x4d", Value, &Len, 1);	//检查是否有交易日志文件
		if (Len != 0)
		{
			SetAppData("\x9f\x4d", Value, Len);
			ReadCardLog();
		}
		else
		{
			clrscr() ;
			printf("\n\n    没有日志文件") ;
			getkeycode(0);
		}
		iReadCardRecord = 0 ;
		return FUNCQUIT;
	}
//	if (ReadCardLog() == SUCC)
//		nlexit(0);
	
	if(ret < 0)
	{
		for(i=choice-1; i<aidnum-1; i++)
		{
			memcpy(aidlist+i, aidlist+i+1, sizeof(TAIDList));
		}
		return FAIL;
	}
	else
	{
		return choice-1;
	}
}


/**
* @fn InputAmount
* @brief 输入交易金额

* @param void
* @return 以分为单位的金额(整型)
*/
int InputAmount(void)
{ 
    int i,padKey;
	char amount[13];
	char stramount[14];

	memset(stramount, 0, sizeof(stramount));
    memset( amount , '0' , 12 );
    amount[12]=0x00;
	
    clrscr();
	printf("请输入交易金额:\n");
	DispXY( 14 , 4 , "元" );
    while (1)
    {
        deal_amount( amount , stramount );
        DispXY( 1 , 4 , stramount);

        padKey = getkeycode(180);
        if (padKey>='0'&&padKey<='9')
        {
            for(i=0;i<11;i++)
                amount[i]=amount[i+1];
            amount[11] = padKey;//数据存在最后使用一位
        }
        else
        {
            switch (padKey)
            {
            case ESC:
				return FAIL;
            case BACKSPACE:
                for(i=10;i>=0;i--)
                    amount[i+1]=amount[i];
                amount[0] = '0';
                break;
            case ENTER:				
                return atol(amount);
            }
        }
    }//while
}


void TermDataLoad(void)
{
	struct postime pt ;
	char timedate[3] ;
	int ret;
	char buf[128];
	char randnum[4];

	getpostime(&pt) ;
	timedate[0] = pt.yearl ;
	timedate[1] = pt.month ;
	timedate[2] = pt.day ;
	SetAppData("\x9A", timedate, 3) ;

	timedate[0] = pt.hour ;
	timedate[1] = pt.minute;
	timedate[2] = pt.second;
	SetAppData("\x9f\x21", timedate, 3) ;

	//生成随机数
	srand(pt.second+pt.minute*60+pt.hour*60*60);
	ret = rand();
	memset(randnum, 0, sizeof(randnum));
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", ret);
	AsciiToBcd((unsigned char *)randnum, (unsigned char *)buf, 8, 0);
	ret = SetAppData("\x9f\x37", randnum, 4);
	if(ret < 0)
	{
		ERR_MSG(" 无法生成随机数");
		return;
	}
	
	//将部分终端配置存入数据缓冲区
	SetAppData("\x9f\x33", g_termconfig.TermCap, 3);
	SetAppData("\x9f\x40", g_termconfig.TermCapAppend, 5);
	SetAppData("\x9f\x1e", g_termconfig.IFDSerialNum, 8);
	SetAppData("\x9f\x1a", g_termconfig.TermCountryCode, 2);
	SetAppData("\x9f\x1c", g_termconfig.TermID, 8);
	SetAppData("\x9f\x35", &(g_termconfig.TermType), 1);
	SetAppData("\x5f\x2a", g_termconfig.TransCurrCode, 2);
	SetAppData("\x5f\x36", &(g_termconfig.TransCurrExp), 1);

	SetAppData("\x9f\x1b", g_termparam.FloorLimit, 4);
	SetAppData("\x9f\x15", g_termparam.MerchantTypeCode, 2);
	SetAppData("\x9f\x16", g_termparam.MerchantID, 15);
	SetAppData("\x9f\x01", g_termparam.AcqID, 6);
	SetAppData("\x9f\x09", g_termparam.AppVersion, 2); //终端应用版本号
	SetAppData("\x9f\x39", &(g_termparam.PosEntry), 1);
	SetAppData("\x9f\x3c", g_termparam.TranRefCurrCode, 2);
	SetAppData("\x9f\x3d", &(g_termparam.TranRefCurrExp), 1);


	ScriptNum = 0 ;
	memset(_ISResult, 0x00, sizeof(_ISResult)) ;

}

int ReloadTermParam(void) 
{
	char param ;
	char file[20] ;
	
	clrscr();
	printf("请输入配置序号:\n(0-9):\n");
	
	while(1)
	{
		param = getkeycode(0) ;
		if (param>= 0x30 && param <= 0x39)
			break ;
		if (param == ESC)
			return SUCC ;
	}
	memset(file, 0x00, sizeof(file)) ;
	sprintf(file,"para%d.dat", param- 0x30) ;
	if( GetTermData(file) < 0)
	{
		return FAIL;
	}
	InitAppData();
	TermDataLoad() ;
	param = param - 0x30;
	savevar(&param, TERM_PARAMNUM_OFF, TERM_PARAMNUM_LEN);
	
	clrscr() ;
	printf("\n  读取成功 \n") ;
	printf("  当前配置为%d", param) ;
	getkeycode(2) ;
	return SUCC ;
}

int nlmain()
{
	int state;
	int refreshflag;
	int selectflag;

	int ret ;
	int iAmount;
	TAIDList aidlist[10];
	int aidnum;
	char buff[512];
	int bufflen;
	int curaidno;
	TCVML cvml;
	char pin[13];
	TTermActAna termactana ;
	TTermRisk     termrisk ;
	TTransProc    transproc ;
	TCompletion  completion ;
	char para_no;
	
 	ret = InitCAPK();
	if(ret != SUCC)
	{
		clrscr();
		printf("\n 公钥初始化失败");
		printf("\n 请重新开机");
		getkeycode(0);
		return FAIL;
	}

	//读取终端配置信息
	getvar(&para_no, TERM_PARAMNUM_OFF, TERM_PARAMNUM_LEN) ;
	memset(buff, 0x00, sizeof(buff)) ;
	sprintf(buff, "para%d.dat", para_no) ;
//	clrscr() ;printf(buff) ; getkeycode(0) ;
	ret = GetTermData(TermParaFile);
	
	if(ret <0 )
	{
		ERR_MSG("读配置文件失败!");
		return FAIL;
	}


	//读终端支持AID列表
	ret = GetTermAidList();
	if(ret <0 )
	{
		ERR_MSG("   读终端支持\n   AID列表失败!");
		return FAIL;
	}

	while(1)
	{
		//初始化应用数据缓冲区
		InitAppData();
		
//		TermDataLoad() ;
		
		//判断是否插入IC卡，选择操作
		refreshflag = 1;
		while(1)
		{	
			if(refreshflag == 1)
			{
				refreshflag = 0;

				clrscr();
				printf("   新大陆电脑\n");
				printf("  借记贷记应用");
				printf("      测试程序\n");
			}


			//判断POS上是否插入IC卡
			state=getDS5002state();

			//使用IC卡1
			if(state&IC1_EXIST)
			{
				IC_SetPort(ICPORT1);
				break;
			}
/*
			//使用IC卡2
			if(state&IC2_EXIST)
			{
				IC_SetPort(ICPORT2);
				break;
			}
*/
			//手动选择交易
			ret = getkeycode(1);
			switch(ret)
			{
				case F1:	//磁卡交易
					magstrip_proc(1);
					refreshflag = 1;
					break;

				case F2:	//修改时间
					ChangeDateTime();
					refreshflag = 1;
					break;
				case F3:
					pushscreen();
					BatchData();
					popscreen();
					break;
				case F4 :
					clrscr();
					printf("\n准备读取交易日志\n  请插入IC卡") ;
					iReadCardRecord = 1 ;
					break;
/*
				case F5:
					refreshflag = 1;
					ReloadTermParam() ;
					break ;
*/
				case ESC:
					return SUCC ;
				default:
					break;
			}

		}


		//输入金额
		if(iReadCardRecord == 0) //非读日志状态，输入金额
		{
			iAmount = InputAmount();
			if(iAmount < 0)
			{
				continue;
			}
		}

		TermDataLoad() ;
		iAcceptForced = 0;

		memset(buff, 0, sizeof(buff));
		IntToC4((unsigned char *)buff,iAmount);
		SetAppData("\x81", buff, 4);
		memset(buff, 0, sizeof(buff));
		sprintf(buff, "%012d", iAmount);
		AsciiToBcd((unsigned char *)buff+12, (unsigned char *)buff, 12, 0);
		SetAppData("\x9f\x02", buff+12, 6);
		SetAppData("\x9C", "\x00", 1) ; //交易类型

		//IC卡上电
		ret = IC_PowerUp();
		if( ret != ICERR_NONE)
		{
			clrscr();
			printf("请进行磁卡交易\n");
			printf("按任意键继续");
			getkeycode(0);
			magstrip_proc(0);
			continue;
		}
		
		//创建应用列表并选择应用
		clrscr();
		printf("\n 查找应用...\n") ;
		aidnum = 0;
		memset(aidlist, 0x00, sizeof(aidlist));

		ret = CreateAppList(g_termparam.PartAIDFlag, aidlist, &aidnum);

		if(ret < 0)
		{
			IC_PowerDown();
			if(ret == ICERR_CODENOTSUPPORT)
			{
				clrscr();
				printf("请进行磁卡交易\n");
				printf("按任意键继续");
				getkeycode(0);
				magstrip_proc(0);
			}
			else
			{
				clrscr();
				printf("\n PSE选择失败\n");
				printf("  交易终止!\n");
				printf("  请更换IC卡!");
				getkeycode(0);
			}
			continue;
		}

		if(aidnum <= 0)
		{
//			if(magstrip_proc()!=Magstrip_SUCC)	manual_proc();
			clrscr();
			printf("\n  无可支持应用\n");
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			IC_PowerDown();
			getkeycode(0);
			continue;
		}

		selectflag = TRUE; //第一次选择应用
		while(aidnum>0)
		{
			memset(buff, 0, sizeof(buff));
			bufflen = 0;
			curaidno = AppSelect(selectflag, aidlist, aidnum, buff, &bufflen);
			if (curaidno == FUNCQUIT)
			{
				refreshflag = 1;
				iReadCardRecord = 0;
				IC_PowerDown();
				break;
			}
			//放弃选择
			if( curaidno == QUIT)
			{
				clrscr();
				printf("\n\n");
				printf("  放弃交易!\n");
				printf("  请更换IC卡!");
				getkeycode(0);
				IC_PowerDown();
				break;
			}
			
			if(curaidno <0 )
			{
				clrscr();
				aidnum--;
				printf("\n 应用选择失败\n");
				if (aidnum == 0)
					printf("  请更换IC卡!");
				else
					printf("  请重新选择应用!") ;
				getkeycode(0);
			}
			else
			{
				//将PDOL保存到数据缓冲区
				SetAppData("\x9f\x38", buff, bufflen) ;
				SetAppData("\x4F", aidlist[curaidno].AID, aidlist[curaidno].AIDLen) ;
				SetAppData("\x9F\x06", aidlist[curaidno].AID, aidlist[curaidno].AIDLen) ;
				SetAppData("\x87", &aidlist[curaidno].AppPriorityID, 1) ;
				SetAppData("\x50", aidlist[curaidno].AppLabel, aidlist[curaidno].AppLabelLen) ;
				SetAppData("\x9F\x12", aidlist[curaidno].AppPreferedName, aidlist[curaidno].AppPreferedNameLen) ;
				//应用初始化
				clrscr();
				printf("\n 应用初始化...\n") ;
				msdelay(100);
				ret = AppInit() ;

		 		if (ret == QUIT)
				{
					clrscr();
					printf("\n应用初始化失败\n");
					printf("请重新选择应用!\n");
					getkeycode(0);
				}
				else if (ret != SUCC)
				{
					clrscr();
					printf("\n 应用初始化失败\n") ;
					printf("  交易终止!\n");
					printf("  请更换IC卡!");
					getkeycode(0);						
					break;
				}
				else
					break ;
			}
			
			selectflag = FALSE;

		}

		if(curaidno < 0 || ret != SUCC)
		{
			IC_PowerDown();
			continue;
		}
		
		//读应用数据
		memset(buff, 0x00, sizeof(buff)) ;
 		clrscr() ;
		printf("\n 读应用数据...\n") ;
		msdelay(100);
		ret = ReadAppData(buff) ;
		if (ret< 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n 读应用数据失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}

		//脱机数据认证
 		clrscr() ;
		printf("\n脱机数据认证...\n") ;
		msdelay(100);
		ret = OffAuth(aidlist[curaidno].AID, buff);
		if (ret< 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n脱机数据认证失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}

		//处理限制
 		clrscr() ;
		printf("\n处理限制...\n") ;
		msdelay(100);
		ret = ProcessLimit(FALSE);
		if(ret < 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n  处理限制失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}

		//持卡人认证
 		clrscr() ;
		printf("\n持卡人认证...\n") ;
		msdelay(100);

		memset(&cvml, 0, sizeof(cvml));
		memset(pin, 0xFF, sizeof(pin));
		_PreCardVerify(&cvml, iAmount) ;
		ret = CardVerify(&cvml, pin) ;
		if(ret < 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n 持卡人认证失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}

		//终端风险管理
 		clrscr() ;
		printf("\n终端风险管理...\n") ;	
		msdelay(100);
		memset(&termrisk, 0x00, sizeof(termrisk)) ;
		_PreTermRiskManage(&termrisk);
		ret = TermRiskManage(&termrisk, iAmount);
		if(ret < 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n终端风险管理失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}
		
		//终端行为分析
 		clrscr() ;
		printf("\n行为分析...\n") ;
		msdelay(100);
		memset(&termactana, 0x00, sizeof(termactana)) ;
		_PreTermActAna(&termactana) ;
		ret = TermActAnalyze(&termactana) ;
		if(ret < 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n终端行为分析失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");

			getkeycode(0);
			continue;
		}

/*		
		clrscr() ;
		p = GetAppData("\x9f\x27", NULL) ;
		printf("CID:%02x\n", *p) ;
		p = GetAppData("\x9f\x36", NULL) ;
		printf("ATC:%02x%02x\n",p[0],p[1]) ;
		p = GetAppData("\x9f\x26\n", NULL) ;
		printf("AC:%02x%02x%02x%02x%02x%02x%02x%02x\n",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		getkeycode(0) ;
*/
		clrscr() ;
		printf("\n交易处理...\n");
		msdelay(100);
		memset(&transproc, 0x00, sizeof(transproc)) ;
		_PreTransProc(&transproc, pin) ;
		ret = TransProc(&transproc) ;		
		if(ret < 0)
		{
			IC_PowerDown();
			clrscr();
			printf("\n交易处理失败\n") ;
			printf("  交易终止!\n");
			printf("  请更换IC卡!");
			getkeycode(0);
			continue;
		}
		
		clrscr() ;
		printf("\n结束交易\n") ;
		msdelay(100);
		memset(&completion, 0x00, sizeof(completion)) ;
		_PreCompletion(&completion, ret) ;

		Completion(&completion) ;
	

		IC_PowerDown();
		//break ;
 	}
	return SUCC;
}

int BatchData()
{
	int fp;
	char buff[1024];
	int len;
	int state;
	
	if ((fp = fopen(LOGFILE, "w")) == FAIL)
	{
		clrscr() ;
		printf("无法打开交易日志文件") ;
		return FAIL ;
	}
	
	memset(buff, 0x00, sizeof(buff)) ;
	fseek(fp, 0L, SEEK_END) ;

	len = ftell(fp) ;
	len /=  LOGLONG ;
	fclose(fp) ;
	
	clrscr() ;
	printf(" 有%d条交易记录\n", len) ;
	printf("  批数据采集...") ;
	for (state = 0 ; state < len ; state++)
	{
		GetLog(state, buff) ;
		ParseLog(buff);
		Online(3, NULL) ;
	}
	fdel(LOGFILE) ;
	return SUCC;

}
