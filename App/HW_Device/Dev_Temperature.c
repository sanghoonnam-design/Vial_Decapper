/*******************************************************************************
 * Dev_Temperature.h
 *
 * Created on : 2025.10.06
 *     Author : RND. Kim BumSu.
 *
 * 1. 냉장고 추가 버전
 ******************************************************************************/
#include "Dev_Temperature.h"
#include "XSystem_DB.h"
#include "XSystemInfo.h"

tsXTemp xTemp;

void Initialize_Temperature(void)
{
    memset((char *)&xTemp, 0, sizeof(tsXTemp));

    xTemp.On /*          */ = Temp_On;
    xTemp.Off /*         */ = Temp_Off;

    xTemp.Update_Tx /*   */ = Temp_Update_Tx;
    xTemp.Update_Rx /*   */ = Temp_Update_Rx;
    //=========================================================================
}

void Temp_Update_Tx(void)
{
    ;
}

void Temp_Update_Rx(void)
{
    ;
}

void Temp_On(void)
{
}

void Temp_Off(void)
{
}
