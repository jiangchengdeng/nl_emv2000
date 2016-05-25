/* RSA.C - RSA routines for RSAREF
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */

#include <string.h>
#include "rsa.h"
#include "nn.h"
#include "define.h"

static int RSAPublicBlock PROTO_LIST 
  ((unsigned char *, unsigned int *, unsigned char *, unsigned int,
    R_RSA_PUBLIC_KEY *));

/* RSA public-key encryption, according to PKCS #1.
 */
int RSAPublicEncrypt
  (output, outputLen, input, inputLen, publicKey)
unsigned char *output;                                      /* output block */
unsigned int *outputLen;                          /* length of output block */
unsigned char *input;                                        /* input block */
unsigned int inputLen;                             /* length of input block */
R_RSA_PUBLIC_KEY *publicKey;                              /* RSA public key */
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen;
  
  modulusLen = (publicKey->bits + 7) / 8;
  if (inputLen + 11 > modulusLen)
    return (RE_LEN);
  
  pkcsBlock[0] = 0;
  /* block type 2 */
  pkcsBlock[1] = 2;

  for (i = 2; i < modulusLen - inputLen - 1; i++) {
	/*填充0x80*/
	pkcsBlock[i] = 0x80;  
  }
  /* separator */
  pkcsBlock[i++] = 0;
  
  R_memcpy ((POINTER)&pkcsBlock[i], (POINTER)input, inputLen);
  
  status = RSAPublicBlock
    (output, outputLen, pkcsBlock, modulusLen, publicKey);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));
  
  return (status);
}

/* RSA public-key decryption, according to PKCS #1.
 */
int RSAPublicDecrypt (unsigned char *output, unsigned int *outputLen, 
	unsigned char *input, unsigned int inputLen, R_RSA_PUBLIC_KEY *publicKey)
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int  modulusLen, pkcsBlockLen;
  
  modulusLen = (publicKey->bits + 7) / 8;
  if (inputLen > modulusLen)
    return (RE_LEN);

  if ((status = RSAPublicBlock
      (pkcsBlock, &pkcsBlockLen, input, inputLen, publicKey))!=0)
    return (status);
  
  if (pkcsBlockLen != modulusLen)
    return (RE_LEN);
  
  *outputLen = modulusLen;

  R_memcpy ((POINTER)output, (POINTER)&pkcsBlock, *outputLen);
  
  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));
  
  return (0);
}


/* Raw RSA public-key operation. Output has same length as modulus.

   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
static int RSAPublicBlock (output, outputLen, input, inputLen, publicKey)
unsigned char *output;                                      /* output block */
unsigned int *outputLen;                          /* length of output block */
unsigned char *input;                                        /* input block */
unsigned int inputLen;                             /* length of input block */
R_RSA_PUBLIC_KEY *publicKey;                              /* RSA public key */
{
  NN_DIGIT c[MAX_NN_DIGITS], e[MAX_NN_DIGITS], m[MAX_NN_DIGITS],
    n[MAX_NN_DIGITS];
  unsigned int eDigits, nDigits;

  NN_Decode (m, MAX_NN_DIGITS, input, inputLen);
  NN_Decode (n, MAX_NN_DIGITS, publicKey->modulus, MAX_RSA_MODULUS_LEN);
  NN_Decode (e, MAX_NN_DIGITS, publicKey->exponent, MAX_RSA_MODULUS_LEN);
  nDigits = NN_Digits (n, MAX_NN_DIGITS);
  eDigits = NN_Digits (e, MAX_NN_DIGITS);
  
  if (NN_Cmp (m, n, nDigits) >= 0)
    return (RE_DATA);
  
  /* Compute c = m^e mod n.
   */
  NN_ModExp (c, m, e, eDigits, n, nDigits);

  *outputLen = (publicKey->bits + 7) / 8;
  NN_Encode (output, *outputLen, c, nDigits);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)c, 0, sizeof (c));
  R_memset ((POINTER)m, 0, sizeof (m));

  return (0);
}


/**
* @fn Recover
* @brief 用公钥解密通过私钥加密的数据

* @param (in)		unsigned char *data  		私钥加密后的数据
* @param (in)		unsigned char *modulus		公钥模
* @param (in) 	unsigned char *exponent	公钥指数
* @param (in)		int len					公钥模长度
* @param (out)	unsigned char *res			解密后输出的数据
* @return  成功: SUCC
* @return  失败: FAIL
*/
int Recover(unsigned char *data, unsigned char *modulus, unsigned char *exponent, int modulus_len, unsigned char *res)
{
	int retcode;
	unsigned int len;
	R_RSA_PUBLIC_KEY publickey;

	memset(&publickey, 0, sizeof(publickey));
	publickey.bits = modulus_len * 8;
	memcpy(publickey.modulus+ MAX_RSA_MODULUS_LEN-modulus_len, modulus , modulus_len);
	memcpy(publickey.exponent + MAX_RSA_MODULUS_LEN-3, exponent , 3);
	
	len = 0;
	retcode = RSAPublicDecrypt(res, &len, data, modulus_len, &publickey);
	if(retcode == 0)
	{
		return SUCC;
	}
	else
	{
		return FAIL;
	}
}


