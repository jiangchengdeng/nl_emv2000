#ifndef _OFFAUTH_H_
#define _OFFAUTH_H_

extern int EncryptPin(char *pin, char *encpin);
extern int OffAuth(char RID[5], char *staticdata);

#endif

