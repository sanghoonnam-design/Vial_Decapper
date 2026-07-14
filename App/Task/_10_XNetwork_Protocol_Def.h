/*******************************************************************************
 * XNetwork_Protocol_Def.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 *
 *  0. 다음은 범용모니터링 프로그램에서 사용할 프로토콜을 정의한 내용이다.
 *     ==> 디버깅용 TCP 통신 프로토콜 정의
 *  1.
 ******************************************************************************/
#ifndef __XTCPCOMM_PROTOCOL_DEF_H__
#define __XTCPCOMM_PROTOCOL_DEF_H__

#include "XGlobal.h"

/**@@========================================================================*/
/**@@      [TCP/SW message protocol                                          */
/**@@========================================================================*/

/** [1]. size of a message   */
#define MSG_ID_SIZE (sizeof(int))
#define MSG_LENGTH_SIZE (sizeof(int)) //TODO

#define MAX_MSG_DATA_SIZE (200)

//==========================================================================h->c
/* [2]. [SEND ID] : [h]ost -> [c]lient  */
/* [2-1]. default Message : 기본 프로토콜 식별자는 아래와 같다. */
#define MSG_H2C_SL (0x10) // used X : /*!> h → c, send state-list structure        */
#define MSG_H2C_CD (0x20) // used O : /*!> h → c, send control-data structure      */
#define MSG_H2C_PL (0x30) // used O : /*!> h → c, send parameter-list structure    */
//==========================================================================h->c

/** [3]. 전체 메시지 규격: level 1 message */
typedef struct
{
    int ID;                        // message ID
    int Length;                    // message Length
    char pData[MAX_MSG_DATA_SIZE]; // size 100

} tsXMessage_TCP; /* h->c & c->h pData */

/** [4]. 2단계 메시지 규격: Lever2 message
 *  -1단계  char pData[MAX_MSG_DATA_SIZE]; 내부 구조중 하나
 */
typedef struct /* XMessage.pData */
{
    float Time; //
    int PL_ID;  // or CMD_ID, XParameterList.h
    int Index;  //

    float Value;

} tsXMsg_PLSet,    // set parameter
    tsXMsg_SLSet,  // set Status
    tsXMsg_CMDSet; // set Command // c->h

typedef union
{
    struct RtcGroup //==========================================================RTC
    {  // c->h PC 시간을 RTC 셋팅할때 사용
        float sec;   // 00~59
        float min;   // 00~59
        float hour;  // 0~23
        float day;   // 1~7,
        float date;  // 1~31
        float month; // 1~12
        float year;  // 00~99

    } DateTime; //=============================================================RTC

    //TODO

} tuXMsg_Special; // union

#endif /* __XTCPCOMM_Protocol_DEF_H__ */
