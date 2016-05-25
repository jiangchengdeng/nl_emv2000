/**
* @file sha1.c
* @brief 采用SHA1计算摘要

* @version Version 1.0
* @author 刘泽清
* @date 2005-08-04
*/

#include <string.h>
#include "posapi.h"
#include "define.h"


typedef unsigned long word32;

word32 H0=0x67452301;
word32 H1=0xEFCDAB89;
word32 H2=0x98BADCFE;
word32 H3=0x10325476;
word32 H4=0xC3D2E1F0;

word32 K[4]={0x5A827999,0x6ED9EBA1,0x8F1BBCDC,0xCA62C1D6};

//////////////////////////////////////////////////////////////
//function 1:	Generate Message									//
//input:														//
//	in:		Original message									//
//	inlen:	length of Original message							//
//output:														//
// out:		message(64byte)									//
//////////////////////////////////////////////////////////////
void GenerateMSG(char *in, int inlen, char *out, int outlen)
{
	int i;

	memset(out,0x00,outlen);
	memcpy(out,in,inlen);
	out[inlen]=0x80;
	i=(inlen+8)/64+1;
	out[outlen-3]=(inlen*8)/(256*256);
	out[outlen-2]=(inlen*8)/256;
	out[outlen-1]=(inlen*8)%256;
}

//////////////////////////////////////////////////////////////
//function 2:	logical function									//
//input:														//
//	B:	word32												//
//	C:	word32												//
//	D:	word32												//
//output:														//
// out:	word32												//
//function 3 and 5 is same, All use logical function 2					//
//////////////////////////////////////////////////////////////
word32 LFunc(word32 B,word32 C,word32 D,int type)
{
	if(type<=19&&type>=0)			return (B&C)|(~B&D);
	else if(type<=39&&type>=20)		return B^C^D;
	else if(type<=59&&type>=40)		return (B&C)|(B&D)|(C&D);
	else 							return B^C^D;
}

//////////////////////////////////////////////////////////////
//function 3:	COMPUTING THE MESSAGE DIGEST					//
//input:														//
//	in:		MSG												//
//	inlen:	length of MSG										//
//output:														//
// out:		dgt												//
//////////////////////////////////////////////////////////////

word32 SFunc(word32 x,int n)
{
	return ((x<<n)|(x>>(32-n)));
}


/**
* @fn CalcMsgDigest
* @brief 采用SHA1计算摘要

* @param (in)	char *message  		需要摘要的数据
* @param (in)	int len				数据长度
* @param (out)	unsigned char *res	计算出的摘要
* @return  void
*/
void CalcMsgDigest(char *message, int len, char *digest)
{
	char msgtmp[530];
	int block,msgtmplen;//,padding;
	int i,j,k;
	word32 W[80];
	word32 wtmp1,wtmp2;
	word32 h0,h1,h2,h3,h4;
	word32 A,B,C,D,E;

	h0=H0;
	h1=H1;
	h2=H2;
	h3=H3;
	h4=H4;
	block=(len+8)/64;
	msgtmplen=((len+8)/64+1)*64;
	GenerateMSG(message, len, msgtmp, msgtmplen);
	for(j=0;j<block+1;j++)
	{
		memset(W,0,80);
		A=h0;
		B=h1;
		C=h2;
		D=h3;
		E=h4;
		for(i=0;i<16;i++)
		{
				for(k=0;k<4;k++)
				{
					W[i]|=(word32)(msgtmp[4*i+64*j+k]);
					if(k<3)	W[i]=W[i]<<8;
				}
		}
		for(i=16;i<80;i++)
		{
			wtmp1=W[i-3]^W[i-8]^W[i-14]^W[i-16];
			W[i]=SFunc(wtmp1,1);
		}
		for(i=0;i<80;i++)
		{
			wtmp2=SFunc(A,5)+LFunc(B,C,D,i)+E+W[i]+K[i/20];
			E=D;
			D=C;
			C=SFunc(B,30);
			B=A;
			A=wtmp2;
		}
		h0+=A;
		h1+=B;
		h2+=C;
		h3+=D;
		h4+=E;
	}

	for(i=0;i<4;i++)	digest[i]=(h0<<(i*8))>>24;
	for(i=0;i<4;i++)	digest[i+4]=(h1<<(i*8))>>24;
	for(i=0;i<4;i++)	digest[i+8]=(h2<<(i*8))>>24;
	for(i=0;i<4;i++)	digest[i+12]=(h3<<(i*8))>>24;
	for(i=0;i<4;i++)	digest[i+16]=(h4<<(i*8))>>24;
}

