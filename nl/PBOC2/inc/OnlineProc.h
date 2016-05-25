
#ifndef _ONLINE_PROC_
#define _ONLINE_PROC_

#define BASE						0x10
#define ONLINE_SUCC 			BASE+1
#define ONLINE_FAIL  			BASE+2
#define AAR_OFFLINE_REFUSE 	BASE+3 
#define AAR_OFFLINE_APPROVE 	BASE+4 

void _PreTransProc(TTransProc * pTransProc, char * pPIN) ;
int TransProc(TTransProc * pTransProc) ;

#endif
