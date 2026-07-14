#include "Drive.h"
sMotionCommand_t     MotionCmd;
sMotionStatus_t      MotionStatus[2];

U8 rotateFlag = 0;
U32 pulseCount = 3200;
void STEP_Init(void)
{
    U8  ch;
    U8  SelMaxCur;
    U8  StepResolution;

    sMotionLimit_t              MotionLimit;
    sMotionSwLimitPos_t         MotionSwLimitPos;
    sMotionCommand_t     MotionCmd;

	 SelMaxCur = CUR_MAX_31A;
	 //SelMaxCur = CUR_MAX_17A;

	StepResolution = R_3200;

	Drive_SetCurrent(0, 1.2f*0.8, 30);
	Drive_SetCurrent(1, 2.0f*0.8, 30);

	for(ch=0; ch<STEP_CH_MAX; ch++)
	{
		Drive_SelMaxCurrent(ch, SelMaxCur);
		Drive_SetResoultion(ch, StepResolution);

		Drive_SetHwLimit(ch, MotionLimit);
		Drive_SetSwLimitPos(ch, MotionSwLimitPos);
	}
}

void VerifyRotateMicroStepFunc(void)
{
	Drive_Init(); // Step motor driver init.

	Drive_PowerEnable(0, 1);
	Drive_PowerEnable(1, 1);

	STEP_Init();

	MotionCmd.Mode = 0;
	MotionCmd.Axis = 0;
	MotionCmd.Acc = 3200*80;
	MotionCmd.Vel = 3200*6;
	MotionCmd.Pos = 0;
	Drive_SetPosition(&MotionCmd);

	MotionCmd.Mode = 4;
	MotionCmd.Axis = 1;
	MotionCmd.Acc = 3200*80;
	MotionCmd.Vel = 3200*6;
	MotionCmd.Pos = 1;
	Drive_SetPosition(&MotionCmd);

	while(1)
	{
		if(rotateFlag)
		{
			rotateFlag = 0;
	
			for(int i=0; i<pulseCount; i++)
			{
				Drive_GetStatus(0, &MotionStatus[0]);
				Drive_GetStatus(1, &MotionStatus[1]);
	
				MotionCmd.Mode = 4;
				switch(MotionCmd.Mode)
				{
				case MODE_STOP      : Drive_Stop(&MotionCmd);                break;
				case MODE_SET_POS   : Drive_SetPosition(&MotionCmd);         break;
				case MODE_CONTINUOUS: Drive_ContinusVelMove(&MotionCmd);     break;
				case MODE_VEL       : Drive_VelMove(&MotionCmd);             break;
				case MODE_REL       : Drive_RelMove(&MotionCmd);             break;
				case MODE_ABS       : Drive_AbsMove(&MotionCmd);             break;
				case MODE_HOMING    : Drive_Homing(&MotionCmd);              break;
				}
				if(MotionCmd.Mode != MODE_NONE) { MotionCmd.Mode = MODE_NONE; }
				vTaskDelay(10);
			}
		}
	}
}

--------------------------------------------------------------------------------------------
#include "Drive.h"

#define SET_POS		(3200*4)
sMotionCommand_t     MotionCmd;
sMotionStatus_t      MotionStatus[2];
void RotateCwCcwTestFunc(void)
{
	TickType_t start;
	
	Drive_Init(); // Step motor driver init.

	Drive_PowerEnable(0, 1);
	Drive_PowerEnable(1, 1);

	STEP_Init();

	MotionCmd.Mode = 0;
	MotionCmd.Axis = 0;
	MotionCmd.Acc = 3200*80;
	MotionCmd.Vel = 3200*6;
	MotionCmd.Pos = 0;
	Drive_SetPosition(&MotionCmd);

	MotionCmd.Mode = 4;
	MotionCmd.Axis = 1;
	MotionCmd.Acc = 3200*80;
	MotionCmd.Vel = 3200*6;
	MotionCmd.Pos = 1;
	Drive_SetPosition(&MotionCmd);
	
	start = xTaskGetTickCount();
	while(1)
	{
		Drive_GetStatus(0, &MotionStatus[0]);
		Drive_GetStatus(1, &MotionStatus[1]);
		
		tick++;

		if((xTaskGetTickCount() - start) > 2000)
		{
			start = xTaskGetTickCount();
			rotateFlag = 1;
		}

		if(rotateFlag == 1)
		{
			if(MotionStatus[0].Pos == SET_POS)
			{
				MotionCmd.Axis = 0;
				MotionCmd.Pos = -SET_POS;
				Drive_RelMove(&MotionCmd);
			}else if(MotionStatus[0].Pos == 0)
			{
				MotionCmd.Axis = 0;
				MotionCmd.Pos = SET_POS;
				Drive_RelMove(&MotionCmd);
			}
			if(MotionStatus[1].Pos == SET_POS)
			{
				MotionCmd.Axis = 1;
				MotionCmd.Pos = -SET_POS;
				Drive_RelMove(&MotionCmd);
			}else if(MotionStatus[1].Pos == 0)
			{
				MotionCmd.Axis = 1;
				MotionCmd.Pos = SET_POS;
				Drive_RelMove(&MotionCmd);
			}
			rotateFlag = 0;
		}
		
		vTaskDelay(5);
		TMC2660_ControlCurrent(0);
		TMC2660_ControlCurrent(1);

	}
}

