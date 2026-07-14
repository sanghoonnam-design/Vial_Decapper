#ifndef   __APP_PARAM_H__
#define   __APP_PARAM_H__

#include "project.h"
#include "Serial_Network_Def.h"  //TODO

#ifdef __APP_PARAM_C__
	#define APP_PARAM_EXT
#else
	#define APP_PARAM_EXT extern
#endif

#pragma pack(push,1)

// typedef struct Network
// {
//     U8 ip[4];
//     U8 subnet[4];
//     U8 gw[4];
//     U16 portNum;  
// } Network_t;

typedef struct AppParam
{
    Network_t   network;
    Serial_t    rs232;
    Serial_t    rs485A;
    Serial_t    rs485B;
} AppParam_t;

typedef struct AppData
{
    U16 data;
}AppData_t;

typedef struct App
{
    AppData_t  data;
    AppParam_t param;
}App_t;

#pragma pack(pop)


APP_PARAM_EXT App_t     App;

APP_PARAM_EXT void AppParam_Init(void);
APP_PARAM_EXT void AppParam_Save(void);
APP_PARAM_EXT void AppParam_Load(void);
APP_PARAM_EXT void AppParam_FactorySet(void);

#endif
