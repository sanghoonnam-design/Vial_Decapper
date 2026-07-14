#ifndef   __TMC429_H__
#define   __TMC429_H__

#include "project.h"

#ifdef __TMC429_C__
	#define TMC429_EXT
#else
	#define TMC429_EXT extern
#endif

#define MOITON_CH_SET           3
#define MOTION_CH_MAX           2

#define MOTION_CH0              0
#define MOTION_CH1              1
#define MOTION_CH2              2

typedef enum{
    IDX_XTARGET          = 0x00,
    IDX_XACTUAL          = 0x02,
    IDX_VMIN             = 0x04,
    IDX_VMAX             = 0x06,
    IDX_VTARGET          = 0x08,
    IDX_VACTUAL          = 0x0A,
    IDX_AMAX             = 0x0C,
    IDX_AACTUAL          = 0x0E,
    IDX_AGTAT_ALEAT      = 0x10,
    IDX_PMUL_PDIV        = 0x12,
    IDX_REFCONF_RM       = 0x14,
    IDX_IMASK_IFLAGS     = 0x16,
    IDX_PULSEDIV_RAMPDIV = 0x18,
    IDX_DX_REFTOLERANCE  = 0x1A,
    IDX_XLATCHED         = 0x1C,
    IDX_USTEP_COUNT_429  = 0x1E,
}TMC429_IDX;

typedef enum{
    JDX_LOW_WORD         = 0x60,
    JDX_HIGH_WORD        = 0x62,
    JDX_COVER_POS_LEN    = 0x64,
    JDX_COVER_DATA       = 0x66,
    JDX_IF_CONFIG_429    = 0x68,
    JDX_POS_COMP_429     = 0x6A,
    JDX_POS_COMP_INT_429 = 0x6C,
    JDX_TYPE_VERSION_429 = 0x72,
    JDX_REF_SWITCHES     = 0x7c,
    JDX_SMGP             = 0x7e,
}TMC429_JDX;

#define IFCONF_INV_REF          0x0001
#define IFCONF_SDO_INT          0x0002
#define IFCONF_STEP_HALF        0x0004
#define IFCONF_INV_STEP         0x0008
#define IFCONF_INV_DIR          0x0010
#define IFCONF_EN_SD            0x0020
#define IFCONF_POS_COMP_0       0x0000
#define IFCONF_POS_COMP_1       0x0040
#define IFCONF_POS_COMP_2       0x0080
#define IFCONF_POS_COMP_OFF     0x00C0
#define IFCONF_EN_REFR          0x0100

#define TMC429_WRITE            0x00
#define TMC429_READ             0x01

#define RM_RAMP                 0
#define RM_SOFT                 1
#define RM_VELOCITY             2
#define RM_HOLD                 3

#define REF_RIGHT_LEFT_SWITCH   0x00
#define REF_RIGHT_SWITCH 		0x01
#define REF_LEFT_SWITCH 		0x02
#define NO_REF                  0x03
#define SOFT_REF                0x04

#define MAX_ACC                 2047
#define MIN_ACC                 0

#define MAX_VELOCITY            2048
#define MIN_VELOCITY            -2048

#define RAMP_DIV                6
#define PULSE_DIV               1
#define STEP_PULSE_RATE_BASE    244  // 16MHz / (2^PULSE_DIV * 2048 * 32)  
#define STEP_PULSE_RATE_R       (STEP_PULSE_RATE_BASE >> PULSE_DIV)
#define A_MAX_BASE              3725  // (16MHz * 16MHz) / (2^(PULSE_DIV+RAMP_DIV+29))

typedef union
{
	struct
	{
		U8 xEQt1 : 1; //reached its target position 3
		U8 rs1 : 1; //reference switch 3
		U8 xEQt2 : 1; //reached its target position 3
		U8 rs2 : 1; //reference switch 3
		U8 xEQt3 : 1; //reached its target position 3
		U8 rs3 : 1; //reference switch 3
		U8 cdgw : 1;
		U8 interrupt : 1;
	};
	U8 byte;
}TMC429StatusBit;

TMC429_EXT void TMC429_Init(void);

TMC429_EXT S32  TMC429_GetPosition(U8 ch);
TMC429_EXT S32  TMC429_GetVelocity(U8 ch);
TMC429_EXT void TMC429_MotorStop(U8 ch, U32 Acc);
TMC429_EXT void TMC429_SetPosition(U8 ch, S32 Pos);
TMC429_EXT U8 TMC429_VelMove(U8 ch, U32 Acc, S32 Vel);
TMC429_EXT U8 TMC429_RelMove(U8 ch, U32 Acc, S32 Vel, S32 Pos);
TMC429_EXT U8 TMC429_AbsMove(U8 ch, U32 Acc, S32 Vel, S32 Pos);

TMC429_EXT void TMC429_SetDir(U8 dir); //change all axis dir!!!
TMC429_EXT U8 TMC429_GetStatus(void); //read ref switch, in position
TMC429_EXT void TMC429_SetRefSwitch(U8 ch, U8 en);

#endif
