/*******************************************************************************
 * XEEPROMParam.c
 *
 * Created on: 2025.02.20
 * Author    : RND, Kang PilSoon.
 * Note      :
 *  1. EEPROM 저장을 다룬다.
 *  2. (주의) 사용시 HW 타이머를 off->on 해서 사용할것
 *     XTimer_Start(), XTimer_Stop()를 사용 할것.
 *
 *
 ******************************************************************************/
#include "XEEPROMParam.h"
#include "_01_XSystemManagement.h"
#include "XSystemInfo.h"
#include "XModbus.h"
#include "XDebug.h"
#include "Version.h"

tsEEPROM_Config gEEPROM;
U08 isEEPROM_OK = NO;

static bool IsFactorySetting(tsEEPROM_Config *config);
static bool ValidateEEPROMData(tsEEPROM_Config *config);
static bool FindLatestValidBlock(U08 *latestBlock);
static bool EEPROM_ReadBlock(U08 blockIndex, tsEEPROM_Config *config);
// static bool EEPROM_WriteBlock(U08 blockIndex, tsEEPROM_Config *config);
static bool EEPROM_WriteBlock(U08 blockIndex, const tsEEPROM_Config *config);

/** *****************************************************************************
 * @attention USER 코드 START
 * *****************************************************************************/
/**
 * @brief EEPROM 기본 설정 적용
 */
void SYSPL_DefaultSetting(void)
{
    // gEEPROM -> xPL
    SYSPL_UpdateSystemParams();

    // xPL 기본값 수정
    SystemDB_PL_Init();
}

/**
 * @brief 공장 초기 설정 적용
 */
void SYSPL_FactorySetting(void)
{
    LOG_MSG_SEND("Factory Setting...");
    /*[]. 구조체 초기화 */
    memset(&gEEPROM, 0, sizeof(tsEEPROM_Config));

    /*[]. EEPROM 초기화 */
    EEPROMPL_ClearEEPROM();

    /** @note USER CODE */
    /*[]. 구조체 factory setting */
    gEEPROM.updateCount = 1;

    gEEPROM.header.FactorySetConfirm_Key = EEPROM_FACTORY_SETTING_KEY;
    gEEPROM.header.FW_Version = FW_VERSION;
    gEEPROM.header.SystemType = GetSystemType();
    gEEPROM.header.ModelType = GetModelType();

    gEEPROM.hwInfo.network.ip[0] = 192;
    gEEPROM.hwInfo.network.ip[1] = 168;
    gEEPROM.hwInfo.network.ip[2] = 0;
    gEEPROM.hwInfo.network.ip[3] = 160;

    gEEPROM.hwInfo.network.subnet[0] = 255;
    gEEPROM.hwInfo.network.subnet[1] = 255;
    gEEPROM.hwInfo.network.subnet[2] = 255;
    gEEPROM.hwInfo.network.subnet[3] = 0;

    gEEPROM.hwInfo.network.gw[0] = 192;
    gEEPROM.hwInfo.network.gw[1] = 168;
    gEEPROM.hwInfo.network.gw[2] = 0;
    gEEPROM.hwInfo.network.gw[3] = 1;

    gEEPROM.hwInfo.network.portNum = 8000;

    /*[]. factory 기본값 입력 */
    SystemDB_PL_Init_Factory(&gEEPROM.sysInfo.xPL);

    gEEPROM.crc = CRC_Calculate((U8 *)&gEEPROM, sizeof(tsEEPROM_Config) - sizeof(U16));

    /*[]. 시스템 다시한번 반영 */
    SYSPL_UpdateSystemParams();

    /*[]. 최종 EEPROM에 save */
    // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
    xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

    EEPROMPL_SaveToEEPROM();
    // EEPROMPL_SaveToFlash(); //TODO
}

// gEEPROM -> Device 파라미터
void SYSPL_UpdateSystemParams(void)
{
    /*[]. 시스템 데이터 최신화 */
    xSystemInfo.PL_Set_FW_Version(gEEPROM.header.FW_Version);
    xSystemInfo.pl_systemType = (U08)gEEPROM.header.SystemType;
    xSystemInfo.pl_modelType = (U08)gEEPROM.header.ModelType;

    xSystemInfo.network.ip[0] = gEEPROM.hwInfo.network.ip[0];
    xSystemInfo.network.ip[1] = gEEPROM.hwInfo.network.ip[1];
    xSystemInfo.network.ip[2] = gEEPROM.hwInfo.network.ip[2];
    xSystemInfo.network.ip[3] = gEEPROM.hwInfo.network.ip[3];

    xSystemInfo.network.subnet[0] = gEEPROM.hwInfo.network.subnet[0];
    xSystemInfo.network.subnet[1] = gEEPROM.hwInfo.network.subnet[1];
    xSystemInfo.network.subnet[2] = gEEPROM.hwInfo.network.subnet[2];
    xSystemInfo.network.subnet[3] = gEEPROM.hwInfo.network.subnet[3];

    xSystemInfo.network.gw[0] = gEEPROM.hwInfo.network.gw[0];
    xSystemInfo.network.gw[1] = gEEPROM.hwInfo.network.gw[1];
    xSystemInfo.network.gw[2] = gEEPROM.hwInfo.network.gw[2];
    xSystemInfo.network.gw[3] = gEEPROM.hwInfo.network.gw[3];

    xSystemInfo.network.portNum = gEEPROM.hwInfo.network.portNum;

    /*[]. xPL 최신화 */
    memcpy(&xPL, &gEEPROM.sysInfo.xPL, sizeof(tsXParameterList));
}

// Device 파라미터 -> gEEPROM
void SYSPL_UpdategEepromStructure(void)
{
    gEEPROM.header.FW_Version = (U32)xSystemInfo.pl_FW_Version;
    gEEPROM.header.SystemType = (U32)xSystemInfo.pl_systemType;
    gEEPROM.header.ModelType = (U32)xSystemInfo.pl_modelType;

    gEEPROM.hwInfo.network.ip[0] = (U32)xSystemInfo.network.ip[0];
    gEEPROM.hwInfo.network.ip[1] = (U32)xSystemInfo.network.ip[1];
    gEEPROM.hwInfo.network.ip[2] = (U32)xSystemInfo.network.ip[2];
    gEEPROM.hwInfo.network.ip[3] = (U32)xSystemInfo.network.ip[3];
    gEEPROM.hwInfo.network.subnet[0] = (U32)xSystemInfo.network.subnet[0];
    gEEPROM.hwInfo.network.subnet[1] = (U32)xSystemInfo.network.subnet[1];
    gEEPROM.hwInfo.network.subnet[2] = (U32)xSystemInfo.network.subnet[2];
    gEEPROM.hwInfo.network.subnet[3] = (U32)xSystemInfo.network.subnet[3];
    gEEPROM.hwInfo.network.gw[0] = (U32)xSystemInfo.network.gw[0];
    gEEPROM.hwInfo.network.gw[1] = (U32)xSystemInfo.network.gw[1];
    gEEPROM.hwInfo.network.gw[2] = (U32)xSystemInfo.network.gw[2];
    gEEPROM.hwInfo.network.gw[3] = (U32)xSystemInfo.network.gw[3];
    gEEPROM.hwInfo.network.portNum = (U32)xSystemInfo.network.portNum;

    memcpy(&gEEPROM.sysInfo.xPL, &xPL, sizeof(tsXParameterList));
}

/** *****************************************************************************
 * @attention USER 코드 END
 * *****************************************************************************/

void EEPROMPL_Initialize(void)
{
    memset((char *)&gEEPROM, 0x0, sizeof(tsEEPROM_Config));
}

/**
 * @brief EEPROM or Flash 에서 데이터 로드
 */
void SYSPL_UpdateFromEeprom_Flash(tsEEPROM_Config *config)
{
    U08 latestBlock;
    static tsEEPROM_Config tempParam;

    xprintf("Loading from EEPROM... (size : %d bytes) ", sizeof(tsEEPROM_Config));
    uint32_t startTime = HAL_GetTick();
    uint32_t elapsedTime;

    //[]. 1단계: 최신데이터 검색
    if (FindLatestValidBlock(&latestBlock) && EEPROM_ReadBlock(latestBlock, config))
    {
        xprintf("Valid block found, validating EEPROM data...");
        if (ValidateEEPROMData(config))
        {
            SYSPL_DefaultSetting();                                                             // 초기화를 시켜야 되는 파라미터 처리
                                                                                                //
            elapsedTime = HAL_GetTick() - startTime;                                            //
            xprintf("[Step 1] EEPROM loaded successfully! - completed in %lu ms", elapsedTime); //
            return;                                                                             // 정상적으로 로드됨
        }
        else
        {                                                                                               // 최신 데이터가 깨져 있는 경우 backup block 에서 데이터를 읽어서 처리 한다.
            if (EEPROMPL_RestoreFromEEPROMBackup(latestBlock) && EEPROM_ReadBlock(latestBlock, config)) // 백업하고 다시 읽어옴(재확인).
            {
                SYSPL_DefaultSetting();

                elapsedTime = HAL_GetTick() - startTime;
                LOG_MSG_SEND("[Step 1-1] EEPROM backup successfully! - completed in %lu ms", elapsedTime);
                return;
            }
            else
            {
                // TODO 에러코드 추가
                ERR_MSG_SEND("EEPROM Backup failed...");
            }
        }
    }

    //[]. 2단계: EEPROM 손상 시 Flash에서 복구 시도
    if (EEPROMPL_LoadFromFlash(&tempParam) && ValidateEEPROMData(&tempParam))
    {
        EEPROM_WriteBlock(0, &tempParam);      // EEPROM에 다시 저장
        EEPROM_ReadBlock(latestBlock, config); // 다시 읽어옴.
        SYSPL_DefaultSetting();                // 일부 수정하여 파라미터에 적용

        elapsedTime = HAL_GetTick() - startTime;
        ERR_MSG_SEND("[Step 2] EEPROM corrupted, recovered from flash. - completed in %lu ms", elapsedTime);

        return;
    } // TODO 에러코드 추가

    //[]. 3단계: 최종 실패 시 Factory Reset
    SYSPL_FactorySetting(); // TODO 에러코드 추가

    elapsedTime = HAL_GetTick() - startTime;
    ERR_MSG_SEND("[Step 3] SYSPL_FactorySetting() - completed in %lu ms", elapsedTime);
}

/**
 * @brief EEPROM 에서만 데이터 로드
 * @return 성공여부
 */
bool EEPROMPL_LoadFromEEPROM(tsEEPROM_Config *config)
{
    U08 latestBlock;

    xprintf("Loading from EEPROM... (size : %d bytes) ", sizeof(tsEEPROM_Config));
    uint32_t startTime = HAL_GetTick();
    uint32_t elapsedTime;

    //[]. 1단계: 최신데이터 검색
    if (FindLatestValidBlock(&latestBlock) && EEPROM_ReadBlock(latestBlock, config))
    {
        xprintf("Valid block found, validating EEPROM data...");
        if (ValidateEEPROMData(config))
        {
            SYSPL_DefaultSetting();                                                             // 초기화를 시켜야 되는 파라미터 처리
                                                                                                //
            elapsedTime = HAL_GetTick() - startTime;                                            //
            xprintf("[Step 1] EEPROM loaded successfully! - completed in %lu ms", elapsedTime); //

            return true; // 정상적으로 로드됨
        }
        else
        { // read backup data

            if (EEPROMPL_RestoreFromEEPROMBackup(latestBlock) && EEPROM_ReadBlock(latestBlock, config)) // 백업하고 다시 읽어옴(재확인).
            {
                SYSPL_DefaultSetting();

                elapsedTime = HAL_GetTick() - startTime;
                xprintf("[Step 1-1] EEPROM backup successfully! - completed in %lu ms", elapsedTime);

                return true;
            }
            else
            {
                // TODO 에러코드 추가
                ERR_MSG_SEND("EEPROM Backup failed...");
            }
        }
    }

    return false;
}

/**
 * @brief EEPROM 데이터 저장 (웨어 레벨링 적용)
 */
void EEPROMPL_SaveToEEPROM(void)
{
    U08 latestBlock;
    U08 newBlock;

    xprintf("Saving to EEPROM... (size : %d bytes) ", sizeof(tsEEPROM_Config));
    // uint32_t startTime = HAL_GetTick();

    SYSPL_UpdategEepromStructure();

    if (!FindLatestValidBlock(&latestBlock))
    { // EEPROM 이 지워졌거나, factory setting 인 경우
        latestBlock = 0;
        newBlock = 0;
        gEEPROM.updateCount = 1;
        ERR_MSG_SEND("No valid block found, starting from block 0.");
    }
    else
    {
        newBlock = (latestBlock + 1) % EEPROM_MAX_BLOCKS;
        gEEPROM.updateCount++; // update count 갱신
    }

    gEEPROM.crc = CRC_Calculate((U8 *)&gEEPROM, sizeof(tsEEPROM_Config) - sizeof(U16)); // crc 갱신

    EEPROM_WriteBlock(newBlock, &gEEPROM);

    //    uint32_t elapsedTime = HAL_GetTick() - startTime;
    //    LOG_MSG_SEND("Writing to new block %d with updateCount %d. - completed in %lu ms", newBlock, gEEPROM.updateCount, elapsedTime);
}

/**
 * @brief EEPROM이 손상된 경우 EEPROM 백업 블럭에서 복구 시도
 * @return 복구 성공 여부
 */
bool EEPROMPL_RestoreFromEEPROMBackup(U08 blockIndex)
{
    U08 backupBlock = (blockIndex + 1) % EEPROM_MAX_BLOCKS;
    U16 mainAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (blockIndex * EEPROM_BLOCK_SIZE);
    U16 backupAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (backupBlock * EEPROM_BLOCK_SIZE);

    tsEEPROM_Config backupData;
    EEPROM_ReadBytes(backupAddress, (U8 *)&backupData, sizeof(tsEEPROM_Config));

    // 백업 데이터가 유효한지 검증
    if (ValidateEEPROMData(&backupData))
    {
        // 데이터가 유효하면 메인 블록에 복원
        EEPROM_WriteBytes(mainAddress, (U8 *)&backupData, sizeof(tsEEPROM_Config));
        return true;
    }
    else
    {
        ERR_MSG_SEND("Backup block validation failed during restore!");
        return false;
    }
}

/**
 * @brief EEPROM이 손상된 경우 Flash에서 복구 시도
 * @return 복구 성공 여부
 */
bool EEPROMPL_LoadFromFlash(tsEEPROM_Config *config)
{
    // tsEEPROM_Config flashBackup;
    // if (Flash_Read(FLASH_APP_PARAM_BACKUP_ADDRESS, &flashBackup, sizeof(tsEEPROM_Config)))
    // {
    //     if (flashBackup.header.FactorySetConfirm_Key == EEPROM_FACTORY_SETTING_KEY &&
    //         CRC_Calculate((U08 *)&flashBackup, sizeof(tsEEPROM_Config) - sizeof(U16)) == flashBackup.crc)
    //     {
    //         gEEPROM = flashBackup;
    //         return EEPROMPL_SaveToEEPROM(); // EEPROM에 저장
    //     }
    // }
    return false;
}

bool EEPROMPL_SaveToFlash(void) // backup flash
{
    SYSPL_UpdategEepromStructure();
    // TODO: 나중에 작성
    // return Flash_Write(FLASH_BACKUP_ADDRESS, &gEEPROM, EEPROM_SIZE);
    return 0;
}

static bool IsFactorySetting(tsEEPROM_Config *config)
{
    return config->header.FactorySetConfirm_Key == EEPROM_FACTORY_SETTING_KEY;
}

static bool ValidateEEPROMData(tsEEPROM_Config *config)
{
    if (!IsFactorySetting(config))
        return false;

    U16 calculatedCRC = CRC_Calculate((U8 *)config, sizeof(tsEEPROM_Config) - sizeof(config->crc));

    if (calculatedCRC == config->crc)
        isEEPROM_OK = YES; // eeprom 양호

    return (calculatedCRC == config->crc);
}

#if 0
/* 최신 유효한 블록 찾기 */
bool FindLatestValidBlock(U08 *latestBlock)
{
    uint16_t maxUpdateCount = 0;
    bool found = false;
    for (int i = 0; i < EEPROM_MAX_BLOCKS; i++)
    {
        tsEEPROM_Config tempConfig;
        if (EEPROM_ReadBlock(i, &tempConfig) && IsFactorySetting(&tempConfig))  // 무겁다. 로직이
        {
            if (ValidateEEPROMData(&tempConfig) && tempConfig.updateCount > maxUpdateCount)
            {
                maxUpdateCount = tempConfig.updateCount;
                *latestBlock = i;
                found = true;
            }
        }
    }
    return found;
}
#else
static bool FindLatestValidBlock(U08 *latestBlock) // 속도 개선 버전
{
    uint16_t maxUpdateCount = 0;
    bool found = false;

    // xprintf("Finding latest valid block...");

    // for (int i = 0; i < EEPROM_MAX_BLOCKS; i++)
    for (int i = EEPROM_MAX_BLOCKS - 1; i >= 0; i--) // [속도개선] 최신 블록부터 역순 검색
    {

#pragma pack(push, 1) // [주의] 메모리 padding 발생
        struct
        {
            U16 updateCount;
            U32 factorySetConfirmKey;
        } tempData;
#pragma pack(pop)

        // 읽을 데이터: FactorySetConfirm_Key와 updateCount만
        EEPROM_ReadBytes(EEPROM_APP_PARAM_BASE_ADDRESS + (i * EEPROM_BLOCK_SIZE), (U8 *)&tempData, sizeof(tempData));

        //        xprintf("Block %d: ConfirmKey = 0x%08X, updateCount = %d", i, tempData.factorySetConfirmKey, tempData.updateCount);

        // Factory setting 확인 및 updateCount 확인
        if (tempData.factorySetConfirmKey == EEPROM_FACTORY_SETTING_KEY)
        {
            if (tempData.updateCount >= maxUpdateCount)
            {
                maxUpdateCount = tempData.updateCount;
                *latestBlock = i;
                found = true;
                //                xprintf("Found valid block %d with updateCount %d", i, tempData.updateCount);
            }
        }
    }

    if (!found)
    {
        ERR_MSG_SEND("No valid block found.");
    }

    return found;
}
#endif

static bool EEPROM_ReadBlock(U08 blockIndex, tsEEPROM_Config *config)
{
    uint16_t address = EEPROM_APP_PARAM_BASE_ADDRESS + (blockIndex * EEPROM_BLOCK_SIZE);
    EEPROM_ReadBytes(address, (U8 *)config, sizeof(tsEEPROM_Config));

    if (config->header.FactorySetConfirm_Key != EEPROM_FACTORY_SETTING_KEY) // 속도 개선
        return false;                                                       // return ValidateEEPROMData(); 까지 가기전에 확인해서 리턴

    return ValidateEEPROMData(config);
}

#if 0
bool EEPROM_WriteBlock(U08 blockIndex, tsEEPROM_Config *config)
{
    uint16_t address = EEPROM_APP_PARAM_BASE_ADDRESS + (blockIndex * EEPROM_BLOCK_SIZE);
    EEPROM_WriteBytes(address, (U8 *)config, sizeof(tsEEPROM_Config));
    return true;
}
#else
/**
 * @brief EEPROM 블록 쓰기 (전원 차단 대비, Atomic Write 적용)
 */
// static bool EEPROM_WriteBlock(U08 blockIndex, tsEEPROM_Config *config)
// {
//     U08 backupBlock = (blockIndex + 1) % EEPROM_MAX_BLOCKS; // 미러링을 위한 백업 블록 선택
//     uint16_t mainAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (blockIndex * EEPROM_BLOCK_SIZE);
//     uint16_t backupAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (backupBlock * EEPROM_BLOCK_SIZE);

//     xprintf("Writing block %d (Backup Block %d)...", blockIndex, backupBlock);

//     // [1] 먼저 백업 블록에 데이터 저장 (전원 차단 대비)
//     EEPROM_WriteBytes(backupAddress, (U8 *)config, sizeof(tsEEPROM_Config));

//     // [2] 백업 블록의 데이터가 정상적으로 쓰였는지 확인
//     tsEEPROM_Config verifyConfig;
//     EEPROM_ReadBytes(backupAddress, (U8 *)&verifyConfig, sizeof(tsEEPROM_Config));
//     if (!ValidateEEPROMData(&verifyConfig))
//     {
//         ERR_MSG_SEND("Backup block validation failed!");
//         return false; // 백업 블록이 손상되었으면 저장 중단
//     }

//     // [3] 백업 블록 데이터가 무결성 검사를 통과하면, 메인 블록에 저장
//     EEPROM_WriteBytes(mainAddress, (U8 *)config, sizeof(tsEEPROM_Config));

//     xprintf("Successfully wrote block %d (Backup Block %d)...\r\n", blockIndex, backupBlock);

//     return true;
// }

static bool EEPROM_WriteBlock(U08 blockIndex, const tsEEPROM_Config *config)
{
    U08 backupBlock = (blockIndex + 1) % EEPROM_MAX_BLOCKS; // 미러링을 위한 백업 블록 선택
    uint16_t mainAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (blockIndex * EEPROM_BLOCK_SIZE);
    uint16_t backupAddress = EEPROM_APP_PARAM_BASE_ADDRESS + (backupBlock * EEPROM_BLOCK_SIZE);

    //    xprintf("Writing block %d (Backup Block %d)...", blockIndex, backupBlock);

    // 구조체를 바이트 배열처럼 접근하기 위해 const 포인터로 변환
    const U08 *data = (const U08 *)config;
    U32 totalSize = sizeof(tsEEPROM_Config);

    // [1] 먼저 백업 블록에 데이터 저장 (전원 차단 대비)
    U32 written = 0;
    while (written < totalSize)
    {
        U32 chunk = (totalSize - written > 64) ? 64 : (totalSize - written);
        EEPROM_WriteBytes(backupAddress + written, (U8 *)&data[written], (uint16_t)chunk);
        written += chunk;
    }

    // [2] 백업 블록의 데이터가 정상적으로 쓰였는지 확인
    static tsEEPROM_Config verifyConfig;
    EEPROM_ReadBytes(backupAddress, (U08 *)&verifyConfig, sizeof(tsEEPROM_Config));
    if (!ValidateEEPROMData(&verifyConfig))
    {
        ERR_MSG_SEND("Backup block validation failed!");
        return false; // 백업 블록이 손상되었으면 저장 중단
    }

    // [3] 백업 블록 데이터가 무결성 검사를 통과하면, 메인 블록에 저장
    written = 0;
    while (written < totalSize)
    {
        U32 chunk = (totalSize - written > 64) ? 64 : (totalSize - written);
        EEPROM_WriteBytes(mainAddress + written, (U8 *)&data[written], (uint16_t)chunk);
        written += chunk;
    }

    xprintf("Successfully wrote block %d (Backup Block %d)...\r\n", blockIndex, backupBlock);

    return true;
}
#endif

bool EEPROMPL_SaveToEEPROMandFlash(void)
{
    // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();
        
    // SYSPL_UpdategEepromStructure();

    EEPROMPL_SaveToEEPROM();
    // EEPROMPL_SaveToFlash(); //TODO

    return true; // TODO
}

#if 1 // TODO 불안정함. ㅠㅠ
void EEPROMPL_ClearEEPROM(void)
{
    U8 zeroBuffer[64] = {0};
    volatile U32 addr = 0;

    xprintf("\r\nClear EEPROM %d bytes...", EEPROM_SIZE);

    while (addr < EEPROM_SIZE)
    {
        // 마지막 남은 크기가 zeroBuffer보다 작으면 그 크기만큼 기록.
        U32 chunkSize = (EEPROM_SIZE - addr) > sizeof(zeroBuffer) ? sizeof(zeroBuffer) : (EEPROM_SIZE - addr);

        EEPROM_WriteBytes(addr, zeroBuffer, chunkSize);
        addr += chunkSize;
    }
}
#endif

/**
 * @brief EEPROM 메모리 디버깅용.
 * @return EEPROM 과
 */
bool EEPROMPL_CompareEEPROMandFlash(void)
{
    bool rtn = false;

    // TODO
    if (1)
    {
        // TODO
        LOG_MSG_SEND("EEPROM == Flash");
        rtn = true;
    }
    else
    {
        // TODO
        ERR_MSG_SEND("EEPROM != Flash");
        rtn = false;
    }

    return rtn;
}

/**
 * @brief EEPROM 메모리 디버깅용.
 * @note  TODO: user code
 */
void EEPROMPL_PrintEepromStructure(void *memory, uint32_t blockIndex, uint32_t totalSize)
{
    U08 *memPtr = (U08 *)memory;
    U32 offset = blockIndex * EEPROM_BLOCK_SIZE;

    // 범위 초과 방지
    if (offset >= totalSize)
    {
        xprintf("Invalid block index: %u (Out of range).\r\n", blockIndex);
        return;
    }

    xprintf("Block %u (Address: 0x%p - Offset: %u):\r\n", blockIndex, (void *)(memPtr + offset), offset);

    for (U32 i = 0; i < EEPROM_BLOCK_SIZE && (offset + i) < totalSize; i++)
    {
        // 16비트(2바이트)씩 출력
        if (i % 2 == 0)
        {
            U16 word = (memPtr[offset + i] << 8) | memPtr[offset + i + 1];
            xcprintf("%04X ", word);
        }

        if ((i + 1) % 8 == 0) // 8바이트마다 공백
            xcprintf("  ");

        if ((i + 1) % 16 == 0) // 16바이트마다 개행
            xcprintf("\r\n");
    }
    __prompt_ln();
}

void EEPROMPL_PrintEepromStructure_user(void) // 디버깅 코드
{
    U08 i = 1;

    __newLine();
    xprintf("\r\n  [--]%8d, %8d : gEEPROM.updateCount", gEEPROM.updateCount, gEEPROM.updateCount);
    xprintf("  [--]%8X, %8X : gEEPROM.header.FactorySetConfirm_Key\r\n", gEEPROM.header.FactorySetConfirm_Key, gEEPROM.header.FactorySetConfirm_Key);

    xprintf("  [--]%8d, %8d : system, gEEPROM.header.FW_Version", (U32)xSystemInfo.pl_FW_Version, gEEPROM.header.FW_Version);
    xprintf("  [--]%8d, %8d : system, gEEPROM.header.SystemType", (U32)xSystemInfo.pl_systemType, gEEPROM.header.SystemType);
    xprintf("  [--]%8d, %8d : system, gEEPROM.header.ModelType\r\n", (U32)xSystemInfo.pl_modelType, gEEPROM.header.ModelType);

    //====
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.ip[0]", i++, (U32)xSystemInfo.network.ip[0], gEEPROM.hwInfo.network.ip[0]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.ip[1]", i++, (U32)xSystemInfo.network.ip[1], gEEPROM.hwInfo.network.ip[1]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.ip[2]", i++, (U32)xSystemInfo.network.ip[2], gEEPROM.hwInfo.network.ip[2]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.ip[3]", i++, (U32)xSystemInfo.network.ip[3], gEEPROM.hwInfo.network.ip[3]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.subnet[0]", i++, (U32)xSystemInfo.network.subnet[0], gEEPROM.hwInfo.network.subnet[0]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.subnet[1]", i++, (U32)xSystemInfo.network.subnet[1], gEEPROM.hwInfo.network.subnet[1]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.subnet[2]", i++, (U32)xSystemInfo.network.subnet[2], gEEPROM.hwInfo.network.subnet[2]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.subnet[3]", i++, (U32)xSystemInfo.network.subnet[3], gEEPROM.hwInfo.network.subnet[3]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.gw[0]", i++, (U32)xSystemInfo.network.gw[0], gEEPROM.hwInfo.network.gw[0]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.gw[1]", i++, (U32)xSystemInfo.network.gw[1], gEEPROM.hwInfo.network.gw[1]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.gw[2]", i++, (U32)xSystemInfo.network.gw[2], gEEPROM.hwInfo.network.gw[2]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.gw[3]", i++, (U32)xSystemInfo.network.gw[3], gEEPROM.hwInfo.network.gw[3]);
    xprintf("  [%2d]%8d, %8d : system, gEEPROM.hwInfo.network.portNum", i++, (U32)xSystemInfo.network.portNum, gEEPROM.hwInfo.network.portNum);

    //  @USER CODE START

    // xprintf("  [%2d]%8d, %8d : system, gEEPROM.sysInfo.WaterLevel_SupplyTime", i++, (U32)xWaterLevel.PL_Get_WaterSupplyTime(), gEEPROM.sysInfo.WaterLevel_SupplyTime);

    // ...

    // @USER CODE END

    xprintf("\r\n  [--]%08X, %08X : gEEPROM.crc", gEEPROM.crc, gEEPROM.crc);
}
