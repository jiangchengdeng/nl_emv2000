#ifndef DESH
#define DESH

#include<stdio.h>
#include<string.h>
typedef unsigned char uchar;

void Des(uchar *binput, uchar *boutput, uchar *bkey);
void unDes(uchar *binput, uchar *boutput, uchar *bkey);

extern uchar DESKEY[];

#endif
