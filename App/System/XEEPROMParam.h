/*******************************************************************************
 * XEEPROMParam.h
 *
 * Created on: 2025.02.20
 * Author    : RND, Kang PilSoon.
 * Note      :
 *  1. EEPROM / Flash / SD 에 저장할 시스템 파라미터들을 다룬다.
 *
 *  2. RND 바이오(의료)장비을 위한 EEPROM 관리 방식 개선
 *    1) 데이터 무결성 검증 :
 *      1-1) CRC(Cyclic Redundancy Check) 추가하여 데이터를 읽을때 무결성을 검중한다.
 *      1-2) 마킹(Valid Flag) 사용: 첫번째 4바이트에 유효성 플래그(0xabcdA5A5)를
 *           사용하여 데이터가 정상적으로 기록되었는지 확인한다.
 *    2) 웨어 레벨링(Wear Leveling) 기법 추가
 *      2-1) EEPROM의 수명이 제한되어 있기 때문에, 같은 주소에 반복적으로 데이터를
 *           쓰면 특정 셀이 먼저 손상될 수 있음.
 *      2-2) 이를 방지하기 위해 라운드 로빈(Round Robin) 방식으로 여러 영역에
 *           순차적으로 데이터를 저장하고 가장 최신 데이터를 사용하는 기법 적용
 *    3) 전원 차단 대비 (Atomic Write)
 *      3-1) EEPROM은 쓰기 도중 전원이 차단되면 데이터가 깨질 수 있음
 *      3-2) **두 개의 버퍼(미러링)**를 사용하여 하나가 손상되었을 경우 다른 것을 사용
 *      3-2) 이전 데이터 유지 후 새로운 데이터가 정상적으로 저장되었는지 검증 후 업데이트.
 *    4) Flash 저장 (3중 저장)
 *      4-1) 모든 EEPROM 에 데이터가 손상되었다고 판단되면 Flash에 저장된 값을 사용.
 *      4-2) 이때 Falsh 값을 EEPROM 에 새로 저장을 시도한다.
 *      4-3) 상위 시스템에 EEPROM이 고장상태임을 알린다.
 *      4-4) Flash 저장은 사용자에 의한 매뉴얼로 이루어진다.
 *    5) 이모두가 실패한 경우 FW에 하드코딩된 Factory Setting 을 수행한다.
 *      5-1) 이때 상위 시스템에 상태를 알린다.
 *
 ******************************************************************************/
#ifndef __XEEPROMPARAM_H__
#define __XEEPROMPARAM_H__
#include "XGlobal.h"
#include "EEPROM.h"
#include "Serial_Network_Def.h"
#include "XSystem_DB.h"

#define EEPROM_APP_PARAM_BASE_ADDRESS (0)
#define FLASH_APP_PARAM_BACKUP_ADDRESS (0) // TBD

/* valid Flag */
#define EEPROM_FACTORY_SETTING_KEY ((U32)0xABCDA5A5)

/* wear leveling */
#if 0
#define EEPROM_BLOCK_SIZE 1024 // 1024 bytes block for wear leveling
#define EEPROM_MAX_BLOCKS 32   // Total number of blocks (for wear leveling)
#else
#define EEPROM_BLOCK_SIZE 4096 // 2048 bytes block for wear leveling
#define EEPROM_MAX_BLOCKS 8    // Total number of blocks (for wear leveling)
#endif

#pragma pack(push, 1)
/* Header */
typedef struct EEPROM_HeaderGroup
{
    U32 FactorySetConfirm_Key; /*!> Valid Flag      */
    U32 FW_Version;            /*!> FW 버전         */
    U32 SystemType;            /*!> 모듈 시스템 종류 */
    U32 ModelType;             /*!> 모듈 HW 버전    */
    U32 header_5;              /*> resv.           */
    U32 header_6;    		   /*> resv.           */
    U32 header_7;              /*> resv.           */
    U32 header_8;              /*> resv.           */
    U32 header_9;              /*> resv.           */
    U32 isCrcValid;            /*> crc check result */
} tsEEPROM_Header;             /*!=> 40 bytes      */

/* HW 정보 */
typedef struct EEPROM_HWInformationGroup
{
    Network_t network;

} tsEEPROM_HwInfo;

/* User Data */ //!> 이부분을 프로젝트에 맞게 수정
typedef struct EEPROM_SystemInformationGroup
{
    tsXParameterList xPL;

} tsEEPROM_SysInfo;

/**
 * @brief EEPROM에 저장할 Parameter List & Data
 * @note  [주의] 구조변경시 관련 코드도 바껴야 함.
 */
typedef struct EEPROMDataGroup
{
    U16 updateCount;          /*!> [주의: 위치고정, 수정금지] 마지막 적용 count : 이걸보고 최신버전을 판단함.*/
    tsEEPROM_Header header;   /*!> [주의: 위치고정, 수정금지] Header */
    tsEEPROM_HwInfo hwInfo;   /*!> HW information */
                              /**/
    tsEEPROM_SysInfo sysInfo; /*!> User Data */
                              /**/
    U16 crc;                  /*!> CRC16 : 시스템 무결성 판단용 */
} tsEEPROM_Config;
#pragma pack(pop)

extern tsEEPROM_Config gEEPROM;
extern U08 isEEPROM_OK;

//
void EEPROMPL_Initialize(void);

/*************************************************************************** */
void SYSPL_UpdateFromEeprom_Flash(tsEEPROM_Config *config); // EEPROM or Flash 에서 파라미터 데이터를 로드합니다.

/** ****************************************************************************
 * @note USER CODE: 사용자가 프로젝트에 맞게 수정하는 부분
 */
/**/ void SYSPL_DefaultSetting(void);           // EEPROM 에서 파라미터를 읽어와 필요한 파라미터 부분을(가공함) 적용합니다.
/**/ void SYSPL_FactorySetting(void);           // 공장 초기 설정을 적용합니다.
/****/ void SYSPL_UpdateSystemParams(void);     // gEEPROM        -> Device 파라미터
/****/ void SYSPL_UpdategEepromStructure(void); // Device 파라미터 -> gEEPROM
/**************************************************************************** */

bool EEPROMPL_LoadFromEEPROM(tsEEPROM_Config *config);      // EEPROM에서만 파라미터 데이터를 로드합니다.
/**/ bool EEPROMPL_RestoreFromEEPROMBackup(U08 blockIndex); // 원본 EEPROM 손상시 백업 EEPROM 에서 복구
void EEPROMPL_SaveToEEPROM(void);                           // 데이터를 EEPROM에 저장합니다.

bool EEPROMPL_LoadFromFlash(tsEEPROM_Config *config); // Flash 에서 파라미터 데이터를 로드합니다.
bool EEPROMPL_SaveToFlash(void);                      // 데이터를 Flash에 저장(백업)합니다.

bool EEPROMPL_SaveToEEPROMandFlash(void); // 데이터를 EEPROM & Flash에 저장(백업)합니다.
void EEPROMPL_ClearEEPROM(void);


// TODO: 사용자가 정한 이전블럭에서 최신블럭으로 복사하는 함수 추가


// 디버깅 코드
bool EEPROMPL_CompareEEPROMandFlash(void);                                                 // EEPROM 과 Flash를 비교한 결과 리턴
void EEPROMPL_PrintEepromStructure(void *memory, uint32_t blockIndex, uint32_t totalSize); // 디버깅 코드
void EEPROMPL_PrintEepromStructure_user(void);                                             // 디버깅 코드

#endif //@end: __XEEPROMPARAM_H__
