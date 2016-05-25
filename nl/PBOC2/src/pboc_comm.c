#include	<string.h>
#include	<stdlib.h>

#include	"../8200API/posapi.h"
#include	"../8200API/define.h"

#include	"../inc/pboc_comm.h"

#define ByteToBcd(n) ((((n) / 10) << 4) | ((n) % 10))
#define BcdToByte(n) (((n) >> 4) * 10 + ((n) & 0x0f))

/**
 *	@sn	char LRC(char * data, int data_len)
 *	@brief	caculate LRC value
 *	@param	data:	the data to be used to caculte LRC value
 *	@param	data_len:	the length of the data
 *	@return	the LRC value
 */
static char LRC(char * data, int data_len)
{
	int i = 0;
	char lrc = 0;

	for(i = 0; i < data_len; i ++)
	{
		lrc ^= data[i];
	}
	return lrc;
}
/**
 *	@sn	int InitComm(void)
 *	@param	initialize the AUX1 to BPS57600, DB8,STOP1,NP
 *	@return	void
 */
int InitComm(void)
{
	return initaux(AUX1, BPS57600, DB8 | STOP1 | NP);
}

/**
 *	@sn int SendData(char *data, int data_len)
 *	@brief the data is formated to transmition protocol STR + DATA LENGTH(2 bytes) + DATA(LLVAR) + ETX + LRC(STR...ETX)
 *	end sent at the end.
 *	@param data : the data to be transmited
 *	@param data_len : the length of the data to be transmited
 *	@return @li FAIL
 *		 @li SUCC
 *	@sa 
 */
int SendData(char *data, int data_len)
{
	char buffer[PACK_BUF_LEN];

	if((data_len < 0) || (data_len > (PACK_BUF_LEN - 5)))
	{
		return FAIL;
	}
	memset(buffer, 0, sizeof(buffer));
	//pack the data phase
	//add
	buffer[0] = TLVP_STX;
	// Add data length
	buffer[1] = ByteToBcd(data_len/100);
	buffer[2] = ByteToBcd(data_len%100);
	//Add data
	memcpy(&buffer[3], data, data_len);
	//Add ETC
	buffer[3 + data_len] = TLVP_ETX;
	//Add LRC
	buffer[4 + data_len] = LRC(buffer, 4 + data_len);
	if(portwrite(AUX1, 5 + data_len, buffer) != SUCC)
	{
		return FAIL;
	}
	return SUCC;
}

/*
 *	@sn int ReceiveData(char * data,int * data_len)
 *	@brief receive data and peel off the ransmition protocol
 *	@param data : store the received data 
 *	@param data_len : store the length of the data
 *	@return @li FAIL
 *		 @li SUCC
 *	@sa 
 */
int ReceiveData(char * data, int * data_len)
{
	int dataLen = 0, received_len = 0;
	char buffer[PACK_BUF_LEN];
	
	memset(buffer, 0, sizeof(buffer));
	//read STX
	if(portread(AUX1, 1, buffer, TLVP_TIMEOUT) != 1)
	{
		return FAIL;
	}	
	if(buffer[0] != TLVP_STX)
	{
		return FAIL;
	}
	received_len += 1;
	//read DATA_LEN
	if((portread(AUX1, 2, &buffer[received_len], TLVP_TIMEOUT) != 2))
	{
		return FAIL;
	}
	dataLen = 100*BcdToByte(buffer[1]) + BcdToByte(buffer[2]);
	if((dataLen < 0) || (dataLen > (PACK_BUF_LEN - 5)))
	{
		return FAIL;
	}
	received_len += 2;
	//read DATA
	if(portread(AUX1, dataLen, &buffer[received_len], TLVP_TIMEOUT) != dataLen)
	{
		return FAIL; 
	}
	received_len += dataLen;
	//read ETX
	if(portread(AUX1, 1, &buffer[received_len], TLVP_TIMEOUT) != 1)
	{
		return FAIL;
	}
	if(buffer[received_len] != TLVP_ETX)
	{
		return FAIL;
	}
	received_len += 1;
	//read LRC
	if(portread(AUX1, 1, &buffer[received_len], TLVP_TIMEOUT) != 1)
	{
		return FAIL;
	}
	if(buffer[received_len] != LRC(buffer, received_len))//check LRC
	{
		return FAIL;
	}
	received_len += 1;
	
	memcpy(data,  &buffer[3], dataLen);
	*data_len = dataLen;
	
	return SUCC;
}
