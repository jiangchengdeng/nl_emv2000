/**
* @file TermActAnal.h
* @brief 终端行为分析

* @version Version 1.0
* @author 叶明统
* @date 2005-8-12
*/
#ifndef _TERM_ACT_ANAL_
#define _TERM_ACT_ANAL_

#define AAC 		0x00
#define TC   		0x01
#define ARQC 	0x10
#define AAR		0x11

void _PreTermActAna(TTermActAna * pTermActAna) ;
int TermActAnalyze( TTermActAna * pTermActAna ) ;

#endif