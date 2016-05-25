#ifndef _COMPLETION_H_
#define _COMPLETION_H_

#define APROVE_OFFLINE 0x00
#define REFUSE_OFFLINE 0x01

void _PreCompletion(TCompletion * pCompletion, int OnlineR) ;
void Completion(TCompletion * pCompletion) ;
int SaveLog(char iForceAccept);
void ParseLog(char * buf) ;
int GetLog(int i, char * buf) ;
#endif
