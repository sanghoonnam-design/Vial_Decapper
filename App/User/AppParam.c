#define __APP_PARAM_C__
    #include "AppParam.h"
#undef  __APP_PARAM_C__

#include "EEPROM.h"
#include "Version.h"
#include "AppParam.h"


#define CONFIRM_PARAM                   ((U32)0xabcd0001)

#define CONFIRM_BASE_ADDRESS            (4)
#define APP_PARAM_BASE_ADDRESS          (100)

App_t     App;

static void SetConfirmNumber(U32 value);
static U32 GetConfirmNumber(void);

static U32 ConfirmNumber;
//static U32 AppParamSize;

void AppParam_Init(void)
{
//  AppParamSize = sizeof(AppParam_t);

    EEPROM_Init();
    
    ConfirmNumber = GetConfirmNumber();
    AppParam_Load();
    
    if(ConfirmNumber != CONFIRM_PARAM)
    {
        AppParam_FactorySet();
    }
}

void AppParam_Save(void)
{
    EEPROM_WriteBytes(APP_PARAM_BASE_ADDRESS,(U8*)&App.param,sizeof(AppParam_t));
}

void AppParam_Load(void)
{
    EEPROM_ReadBytes(APP_PARAM_BASE_ADDRESS,(U8*)&App.param,sizeof(AppParam_t));
}

void AppParam_FactorySet(void)
{
    SetConfirmNumber(CONFIRM_PARAM);
    
    App.param.network.ip[0] = 192;
    App.param.network.ip[1] = 168;
    App.param.network.ip[2] = 0;
    App.param.network.ip[3] = 150;

    App.param.network.subnet[0] = 255;
    App.param.network.subnet[1] = 255;
    App.param.network.subnet[2] = 255;
    App.param.network.subnet[3] = 0;
    
    App.param.network.gw[0] = 192;
    App.param.network.gw[1] = 168;
    App.param.network.gw[2] = 0;
    App.param.network.gw[3] = 1;
    
    App.param.network.portNum = 35000;
    
    App.param.rs232.baudrate    = BAUDRATE_19200;
    App.param.rs232.parity      = PARITY_NONE;
    App.param.rs232.stopbit     = UART_STOPBITS_1;
    
    AppParam_Save();
}

static U32 GetConfirmNumber(void)
{
    U32 value;
    
    EEPROM_ReadBytes(CONFIRM_BASE_ADDRESS, (U8*)&value, 4);
    
    return value;
}

static void SetConfirmNumber(U32 value)
{
    ConfirmNumber = value;
    EEPROM_WriteBytes(CONFIRM_BASE_ADDRESS, (U8*)&value, 4);
}