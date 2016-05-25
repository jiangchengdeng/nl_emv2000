/*
 *	PBOC DATA TRANSMITION PROTOCOL:
 *	STX + DATA LENGTH + DATA + ETX + LRC
 *	STX : 0x02
 *	DATA LANGTH : 2 BYTES LENGTH
 *	DATA: LLVAR
 *	ETX	: 0x03
 *	LRC	: Use STR + DATA LENGTH + DATA + ETX to caculate LRC
 */
#ifndef _PBOC_COMM_H_
#define _PBOC_COMM_H_

#define TLVP_STX 0x02
#define TLVP_ETX 0x03
#define TLVP_TIMEOUT 5

#define PACK_BUF_LEN 512 ///<THE MAX LENGTH OF THE DATA TO BE TRANSMITTED(INCLUDE STR...LRC)

extern int InitComm(void);
extern int SendData(char *data, int data_len);
extern int ReceiveData(char *data, int * data_len);

#endif