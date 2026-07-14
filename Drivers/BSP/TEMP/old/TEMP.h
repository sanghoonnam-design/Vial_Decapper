#ifndef   __TEMP_H__
#define   __TEMP_H__

#include "project.h"

#ifdef __TEMP_C__
	#define TEMP_EXT
#else
	#define TEMP_EXT extern
#endif

#define TEMP_CH_MAX             4

#define TEMP_CH0                0
#define TEMP_CH1                1
#define TEMP_CH2                2
#define TEMP_CH3                3

#define TEMP_KTYPE              1
#define TEMP_PT100              0  

#define AD7124_CSL              0
#define AD7124_CSH              1

#define RESOLUTION              16777216.0f
#define REF_VOLT                2.5f
#define G16                     16.0f
#define G32                     32.0f
#define G128                    128.0f

#define REG_STATUS              0x00
#define REG_ADC_CTRL            0x01
#define REG_DATA                0x02
#define REG_IO_CTRL1            0x03
#define REG_IO_CTRL2            0x04
#define REG_ID                  0x05
#define REG_ERROR               0x06
#define REG_CH0                 0x09
#define REG_CH15                0x18
#define REG_CONFIG0             0x19
#define REG_CONFIG1             0x1A
#define REG_CONFIG2             0x1B
#define REG_CONFIG7             0x20
#define REG_FILTER0             0x21
#define REG_FILTER1             0x22
#define REG_FILTER2             0x23

#define SIZE_STATUS             0x01
#define SIZE_ADC_CTRL           0x02
#define SIZE_DATA               0x03
#define SIZE_IO_CTRL1           0x03
#define SIZE_IO_CTRL2           0x02
#define SIZE_ID                 0x01
#define SIZE_ERROR              0x03
#define SIZE_CH                 0x02
#define SIZE_CONFIG             0x02
#define SIZE_FILTER             0x03

#define MODE_REF_ENABLE         (1<<8)
#define MODE_FULL_POWER         (2<<6)
#define MODE_SINGLE             (1<<2)

#define CH_ENABLE               (1<<15)
#define CH_SENSOR               0x10
#define CH_AVSS                 0x11

#define IO1_CUR250              3
#define IO1_CUR500              4

#define CFG_UNIPOLAR            (0<<11)
#define CFG_BIPOLAR             (1<<11)
#define CFG_REF_BUFP            (1<<8)
#define CFG_REF_BUFM            (1<<7)
#define CFG_AIN_BUFP            (1<<6)
#define CFG_AIN_BUFM            (1<<5)
#define CFG_REF_SEL0            (0<<3)
#define CFG_REF_SEL2            (2<<3)
#define CFG_GAIN_1              (0<<0)
#define CFG_GAIN_16             (4<<0)
#define CFG_GAIN_32             (5<<0)
#define CFG_GAIN_128            (7<<0)

#define FLT_FS128               0x80

#define CONFIG0                 0
#define CONFIG1                 1
#define CONFIG2                 2
#define CONFIG7                 7











#define ADS1220_1ST             0
#define ADS1220_2ND             1

#define CONFIG_REG0             0x00
#define CONFIG_REG1             0x01

#define ADS1220_RESET           0x06
#define ADS1220_SYNC            0x08
#define ADS1220_PWRDN           0x02
#define ADS1220_RDATA           0x10
#define ADS1220_RREG            0x20
#define ADS1220_WREG            0x40

typedef enum ConfigReg0_MUX_t
{
    MUX_AINp0_AINn1 = 0,
    MUX_AINp0_AINn2           ,                  //1
    MUX_AINp0_AINn3           ,                  //2
    MUX_AINp1_AINn2           ,                  //3
    MUX_AINp1_AINn3           ,                  //4
    MUX_AINp2_AINn3           ,                  //5
    MUX_AINp1_AINn0           ,                  //6
    MUX_AINp3_AINn2           ,                  //7
    MUX_AINp0_AINnVSS         ,                  //8
    MUX_AINp1_AINnVSS         ,                  //9
    MUX_AINp2_AINnVSS         ,                  //10
    MUX_AINp3_AINnVSS         ,                  //11
    MUX_Vrefp_MINUS_Vrefn_DIV4,                  //12
    MUX_VDD_MINUS_VSS_DIV4    ,                  //13
                               MUX_VDD_VSS_DIV2  //14
}ConfigReg0_MUX_t;

typedef enum ConfigReg0_GAIN_t
{
    GAIN_1 = 0,
    GAIN_2 ,          //1
    GAIN_4 ,          //2
    GAIN_8 ,          //3
    GAIN_16,          //4
    GAIN_32,          //5
    GAIN_64,          //6
            GAIN_128  //7
}ConfigReg0_GAIN_t;

typedef enum ConfigReg0_PGA_t
{
    PGA_ENABLE = 0,
    PGA_DISABLE
}ConfigReg0_PGA_t;

typedef enum ConfigReg1_DR_t
{
    DR_20_SPS = 0,
    DR_45_SPS,  //1
    DR_90_SPS,          //2
    DR_175_SPS,         //3
    DR_330_SPS,         //4
    DR_600_SPS,         //5
    DR_1000_SPS         //6
}ConfigReg1_DR_t;

typedef enum ConfigReg1_MODE_t
{
    MODE_NORMAL = 0,
    MODE_DUTY_CYCLE,
    MODE_TURBO
}ConfigReg1_MODE_t;

typedef enum ConfigReg1_CM_t
{
    CM_SINGLE_SHOT = 0,
    CM_CONTINUOUS_CONV,
}ConfigReg1_CM_t;

typedef enum ConfigReg1_TS_t
{
    TS_DISABLE_TEMP = 0,
    TS_ENABLE_TEMP,
}ConfigReg1_TS_t;

typedef enum ConfigReg1_BCS_t
{
    BCS_DISABLE_CURR_SOURCE_OFF = 0,
    BCS_DISABLE_CURR_SOURCE_ON,
}ConfigReg1_BCS_t;

#pragma pack(push,1)

typedef union ConfigReg0_t
{
    struct
    {
    ConfigReg0_PGA_t  pga  : 1;
    ConfigReg0_GAIN_t gain : 3;
    ConfigReg0_MUX_t  mux  : 4;
    }bit;
    U8 byte;
}ConfigReg0_t;

typedef union ConfigReg1_t
{
    struct
    {
    ConfigReg1_BCS_t  bcs  : 1;
    ConfigReg1_TS_t   ts   : 1;
    ConfigReg1_CM_t   cm   : 1;
    ConfigReg1_MODE_t mode : 2;
    ConfigReg1_DR_t   dr   : 3;
    }bit;
    U8 byte;
}ConfigReg1_t;

#pragma pack(pop)

TEMP_EXT void TEMP_Init(void);

TEMP_EXT F32  TEMP_GetTemprature(U8 ch);
TEMP_EXT F32  TEMP_GetKTYPE(U8 ch);
TEMP_EXT F32  TEMP_GetNTC(U8 ch);

#endif


