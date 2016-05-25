/**
* @file tvrtsi.h
* @brief 终端验证结果TVR，交易状态TSI各位的含义

* 该文件用于EMV2000项目
* @version Version 1.0
* @author 叶明统
* @date 2005-08-02
*/

#ifndef _TVR_TSI_
#define _TVR_TSI_

/* TVR - Terminal Verification Results */

/* 第一字节（最左边）*/
#define OFFLINE_AUTH_UNDO  0x0080  /* 未进行脱机数据认证      */
#define OFFLINE_SDA_FAIL   0x0040  /* 脱机静态数据认证失败    */
#define ICC_DATA_LOST      0x0020  /* IC卡数据缺失            */
#define CARD_IN_EXCEP_FILE 0x0010  /* 卡片出现在终端异常文件中*/
#define OFFLINE_DDA_FAIL   0x0008  /* 脱机动态数据认证失败    */
#define CDA_FAIL           0x0004  /* 复合动态数据认证失败    */
#define GEN_AC_FAIL        0x0004  /* 应用密码生成失败        */

/* 第二字节 */
#define VERSION_NOT_MATCHED 0x0180  /* IC卡和终端应用版本不一致 */
#define APP_EXPIRE          0x0140  /* 应用已过期               */
#define APP_NOT_EFFECT      0x0120  /* 应用尚未生效             */
#define SERV_REFUSE         0x0110  /* 卡片产品不允许所请求服务 */
#define NEW_CARD            0x0108  /* 新卡                     */

/* 第三字节 */
#define CV_NOT_SUCCESS    0x0280  /* 持卡人认证未成功 */
#define UNKNOW_CVM        0x0240  /* 未知的CVM        */
#define PIN_RETRY_EXCEED  0x0220  /* PIN重试次数超限  */
#define REQ_PIN_PAD_FAIL  0x0210  /* 要求输入PIN，但无PIN PAD或PIN PAD故障 */
#define REQ_PIN_NOT_INPUT 0x0208  /* 要求输入PIN，有PIN PAD，但未输入PIN   */
#define INPUT_ONLINE_PIN  0x0204  /* 输入联机PIN      */

/* 第四字节 */
#define TRADE_EXCEED_FLOOR_LIMIT       0x0380  /* 交易超过最低限额       */
#define EXCEED_CON_OFFLINE_FLOOR_LIMIT 0x0340  /* 超过连续脱机交易下限   */
#define EXCEED_CON_OFFLINE_UPPER_LIMIT 0x0320  /* 超过连续脱机交易上限   */
#define SELECT_ONLILNE_RANDOM          0x0310  /* 交易被随机选择联机处理 */
#define MERCHANT_REQ_ONLINE            0x0308  /* 商户要求联机交易       */

/* 第五字节 */
#define USE_DEFAULT_TDOL    0x0480   /* 使用缺省TDOL   */
#define ISSUER_AUTH_FAIL    0x0440   /* 发卡行认证失败 */

#define SCRIPT_FAIL_BEFORE_LAST_GEN_AC 0x0420 /* 最后一次生成应用密码命令之前
                                               脚本处理失败 */
#define SCRIPT_FAIL_AFTER_LAST_GEN_AC  0x0410 /* 最后一次生成应用密码命令之后
                                               脚本处理失败 */


/* TSI - Transaction Status Information */

/* 第一字节（最左边）*/
#define OFFLINE_DA_COMPLETION      0x0080  /* 脱机数据认证已完成 */
#define CV_COMPLETION              0x0040  /* 持卡人认证已完成   */
#define CARD_RISK_MANA_COMPLETION  0x0020  /* 卡片风险管理已完成 */
#define ISSUER_AUTH_COMPLETION     0x0010  /* 发卡行认证已完成   */
#define TERM_RISK_MANA_COMPLETION  0x0008  /* 终端风险管理已完成 */
#define SCRIPT_PROC_COMPLETION     0x0004  /* 脚本处理已完成     */


/* 初始化TVR字节 */
void InitTVR(void) ;

/* 设置TVR字节 */
void SetTVR( int iSetMask, int OnOff ) ;

/* 返回TVR字节 */
void GetTVR( char * tvrtmp ) ;

/* 初始化TSI字节 */
void InitTSI(void) ;

/* 设置TSI字节 */
void SetTSI( int iSetMask, int OnOff ) ;

/* 返回TSI字节 */
void GetTSI( char * tsitmp ) ;

#endif