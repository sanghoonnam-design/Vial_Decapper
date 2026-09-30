/*******************************************************************************
 * XSystemInfo.c
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XSystemInfo.h"
#include "Version.h"

#include "FreeRTOS.h"
#include "task.h"
#include "XSystem_DB.h"

#include "XModbus.h"
#include "bootloader.h"

tsXSystemInfo xSystemInfo;

/**
 * @brief 시스템 콜백·DB·하드웨어·저장 설정을 초기화하고 Decapper 식별/버전 정보를 맞춘다.
 * 저장 정보와 빌드의 버전·장비·모델이 다르면 최신 값을 저장한 뒤 표시 문자열과 네트워크를 초기화한다.
 */
void System_Initialize(void)
{
    /* system 구조체 초기화 */
    memset((char *)&xSystemInfo, 0, sizeof(tsXSystemInfo));
    //==========================================================================
    xSystemInfo.SL_Set_isStartMainLoop /*     */ = SystemInfo_Set_sl_isStartMainLoop;
    xSystemInfo.SL_Get_isStartMainLoop /*     */ = SystemInfo_Get_sl_isStartMainLoop;
    xSystemInfo.CD_Set_CpuUsage /*            */ = SystemInfo_Set_cd_CpuUsage;
    xSystemInfo.CD_Get_CpuUsage /*            */ = SystemInfo_Get_cd_CpuUsage;
    xSystemInfo.CD_Set_CpuTemperature /*      */ = SystemInfo_Set_cd_CpuTemperature;
    xSystemInfo.CD_Get_CpuTemperature /*      */ = SystemInfo_Get_cd_CpuTemperature;
    xSystemInfo.PL_Set_FW_Version /*          */ = SystemInfo_Set_pl_FW_Version;
    xSystemInfo.PL_Get_FW_Version /*          */ = SystemInfo_Get_pl_FW_Version;
    xSystemInfo.PL_Set_FW_Mode /*             */ = SystemInfo_Set_pl_FW_Mode;
    xSystemInfo.PL_Get_FW_Mode /*             */ = SystemInfo_Get_pl_FW_Mode;
    xSystemInfo.PL_Set_IsExecutedDiagnosis /* */ = SystemInfo_Set_pl_IsExecutedDiagnosis;
    xSystemInfo.PL_Get_IsExecutedDiagnosis /* */ = SystemInfo_Get_pl_IsExecutedDiagnosis;
    //==========================================================================

    /* System 변수 default 값 초기화 */
    xSystemInfo.SL_Set_isStartMainLoop(NO);
    xSystemInfo.CD_Set_CpuUsage(0.0f);
    xSystemInfo.CD_Set_CpuTemperature(0.0f);

    xSystemInfo.pl_systemType = (U08)GetSystemType();
    xSystemInfo.pl_modelType = GetModelType();

    xSystemInfo.PL_Set_FW_Mode(FW_MODE_DEFAULT);
    xSystemInfo.PL_Set_IsExecutedDiagnosis(YES);

    SystemDB_Initialize();

    HWDev_Initialize(); // HW  & Device 초기화
    InitErrorCode();    // error  초기화

    EEPROMPL_Initialize();                  //
    SYSPL_UpdateFromEeprom_Flash(&gEEPROM); // EEPROM 으로 부터 system 파라미터 초기화
    
    U16 crc = CRC_Calculate((U8 *)&gEEPROM, sizeof(tsEEPROM_Config) - sizeof(U16));
    
    if (gEEPROM.header.FactorySetConfirm_Key == EEPROM_FACTORY_SETTING_KEY)
    {
        if (crc != gEEPROM.crc)
        {
            gEEPROM.header.isCrcValid = NO;
        }
        else
        {
            gEEPROM.header.isCrcValid = YES;
        }
    }
    else
    {
        SYSPL_FactorySetting();
    }

    //==========================================================================
    if (xSystemInfo.pl_FW_Version != FW_VERSION || // 시스템 정보 최신화 체크
        GetSystemType() != gEEPROM.header.SystemType ||
        GetModelType() != gEEPROM.header.ModelType)
    {
        xSystemInfo.pl_FW_Version = FW_VERSION;
        xSystemInfo.pl_systemType = GetSystemType();
        xSystemInfo.pl_modelType = GetModelType();

        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();
        
        EEPROMPL_SaveToEEPROMandFlash();
    }
    makeVersionString();

    // ==========================================================================
    BootParamInfo_SetNetworkIP(xSystemInfo.network);
    Network_Init(); // EEPROM 로딩이 끝나면 network 정보 반영 및 초기화

#if configWatchDog_ENABLE
    IWDG_Init();
#endif
}

SET_GET_FUNC_0_IMPL(SystemInfo, U08, sl_isStartMainLoop)
SET_GET_FUNC_0_IMPL(SystemInfo, F32, cd_CpuUsage)
SET_GET_FUNC_0_IMPL(SystemInfo, F32, cd_CpuTemperature)
//SET_GET_FUNC_0_IMPL(SystemInfo, U32, pl_FW_Version)
SET_GET_FUNC_0_IMPL(SystemInfo, U08, pl_FW_Mode)
SET_GET_FUNC_0_IMPL(SystemInfo, U32, pl_IsExecutedDiagnosis)

/**
 * @brief 숫자형 펌웨어 버전을 저장하고 표시 문자열도 함께 갱신한다.
 * 버전 인코딩은 major*10000 + minor*100 + patch이다.
 */
void SystemInfo_Set_pl_FW_Version(U32 version)
{
    xSystemInfo.pl_FW_Version = version;
    makeVersionString();
}

/**
 * @brief xSystemInfo에 저장된 숫자형 펌웨어 버전을 반환한다.
 */
U32 SystemInfo_Get_pl_FW_Version(void)
{
    return xSystemInfo.pl_FW_Version;
}

void SystemInfo_Network_SetIP(Network_t *net, const U8 ip[4])
{
    if (net == NULL)
        return;
    memcpy(net->ip, ip, sizeof(net->ip));
}

/**
 * @brief 숫자 버전과 저장된 모델 번호를 major.minor.patchA모델 형식으로 변환한다.
 * 예: 1.0.0과 MODEL_05는 1.0.0A05로 표시한다.
 */
void makeVersionString(void) {
    U08 modelType = xSystemInfo.pl_modelType;
    int major = (xSystemInfo.pl_FW_Version / 10000);     // Major 버전
    int minor = (xSystemInfo.pl_FW_Version / 100) % 100; // Minor 버전
    int patch = xSystemInfo.pl_FW_Version % 100;         // Patch 버전
    snprintf(xSystemInfo.cd_FWVersion_str, sizeof(xSystemInfo.cd_FWVersion_str),
             "%d.%d.%dA%02u", major, minor, patch, (unsigned int)modelType);
}

/**
 * @brief 빌드 시 선택한 SYSTEM_TYPE 장비 식별 번호를 반환한다.
 */
U08 GetSystemType(void) {

    return SYSTEM_TYPE;
}

/**
 * @brief 빌드 시 선택한 MODEL_TYPE 번호를 반환한다. 지원 모델은 05/15/25/50이다.
 */
U08 GetModelType(void) {

    return MODEL_TYPE;
}

/**
 * @brief 빌드에 정의된 장비명 문자열을 반환한다. Decapper 빌드는 Vial_Decapper를 사용한다.
 */
const char *GetSystemTypeString(void) {
    return SYSTEM_TYPE_STR;
}

/**
 * @brief 빌드 모델 번호를 MODEL-05/15/25/50 문자열로 변환한다.
 * 정의되지 않은 모델 번호는 UNKNOWN으로 표시한다.
 */
const char *GetModelTypeString(void){
    switch (GetModelType()){
    case MODEL_05:
        return "MODEL-05";
    case MODEL_15:
        return "MODEL-15";
    case MODEL_25:
        return "MODEL-25";
    case MODEL_50:
        return "MODEL-50";
    default:
        return "UNKNOWN";
    }
}
