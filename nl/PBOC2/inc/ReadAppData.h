#ifndef _READ_APP_DATA_
#define _READ_APP_DATA_

/**
 * @fn ReadAppData
 * @brief 从IC卡读数据

 * @param (out) char * StatDataAuth 需进行静态数据认证的数据
 * @return SUCC 成功
           FAIL   失败
 */
int ReadAppData(char * StatDataAuth)  ;
int ReadCardLog(void) ;
#endif
