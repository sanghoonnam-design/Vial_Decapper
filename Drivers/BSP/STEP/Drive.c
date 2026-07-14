#define __DRIVE_C__
    #include "Drive.h"
#undef  __DRIVE_C__

#include "TMC429.h"
#include "TMC2660.h"
#include "LED.h"

typedef union
{
    struct
    {
        U8 HomeSensor   : 1;
        U8 NegativeLimit: 1;
        U8 PositiveLimit: 1;
        U8 Reserved     : 5;
    }Bit;
    
    U8 Byte;
}uINPUT_t;

typedef union
{
    struct
    {
        U8 Run           : 1;
        U8 Dir           : 1;
        U8 HomingRun     : 1;
        U8 HomingComplete: 1;
        
        U8 HomeSensor   : 1;
        U8 NegativeLimit: 1;
        U8 PositiveLimit: 1;
        U8 Reserved     : 1;
    }Bit;
    
    U8 Byte;
}uDriveStatus_t;

typedef enum
{
    HOMING_NONE = 0,
    HOMING_START,
    HOMING_H_SEARCH,
    HOMING_H_REACH,
    HOMING_H_WAIT,
    HOMING_L_SEARCH,
    HOMING_L_REACH_OFF,
    HOMING_L_REACH_ON,
    HOMING_L_WAIT,
    HOMING_DONE,
    HOMING_COMPLETE,
}eHomingStep_t;

typedef struct
{
    eHomingStep_t       Step;
        
    S32 H_Acc;
    S32 H_Vel;
    S32 L_Acc;
    S32 L_Vel;
    U8  Dir;
}sHomingSequence_t;

static uDriveStatus_t           DriveSts[MOTION_CH_MAX];
static sHomingSequence_t        HomingSeq[MOTION_CH_MAX];

static sMotionLimit_t           MotionLimit[MOTION_CH_MAX];
static sMotionSwLimitPos_t      MotionSwLimitPos[MOTION_CH_MAX];

void Drive_Init(void)
{
    TMC2660_Init();
    TMC429_Init();    
}

void Drive_PowerEnable(U8 ch, U8 OnOff)
{
    TMC2660_PowerEnable(ch, OnOff);
}

void Drive_SelMaxCurrent(U8 ch, U8 type)
{    
    TMC2660_SetMaxCurrent(ch, type);
}

void Drive_SetCurrent(U8 ch, F32 rCur, U8 sRate)
{
    F32 MaxCur;
    sMotorCurrent_t     cur;
    
    MaxCur = TMC2660_GetMaxCurrent(ch);
    
    cur.iRun  = (U8)(rCur/MaxCur * 31.);
    cur.iHold = (U8)((F32)sRate/100. * (F32)cur.iRun);
    
    TMC2660_SetCurrentScale(ch, cur);
}

void Drive_SetResoultion(U8 ch, U8 res)
{
    TMC2660_SetResolution(ch, res);
}

void Drive_SetHwLimit(U8 ch, sMotionLimit_t limit)
{
    MotionLimit[ch] = limit;
}
                      
void Drive_SetSwLimitPos(U8 ch, sMotionSwLimitPos_t SwLimit)
{
    MotionSwLimitPos[ch] = SwLimit;
}

void Drive_GetStatus(U8 ch, sMotionStatus_t* Status)
{
    Status->Pos = TMC429_GetPosition(ch);
    Status->Vel = TMC429_GetVelocity(ch);
    Status->Enc = 0;
    
    Status->Bit.NegLimit      = DriveSts[ch].Bit.NegativeLimit;
    Status->Bit.PosLimit      = DriveSts[ch].Bit.PositiveLimit;
    Status->Bit.bHomeComplete = DriveSts[ch].Bit.HomingComplete;
    Status->Bit.bHomingRun    = DriveSts[ch].Bit.HomingRun;
    Status->Bit.bMoving       = TMC2660_GetMotorRun(ch);

    if(Status->Vel > 0)
    {
        LED_DriveOnOff(ch, LED_CW, true);
        LED_DriveOnOff(ch, LED_CCW, false);
    }
    else if(Status->Vel < 0)
    {
        LED_DriveOnOff(ch, LED_CW, false);
        LED_DriveOnOff(ch, LED_CCW, true);
    }
    else
    {
        LED_DriveOnOff(ch, LED_CW, false);
        LED_DriveOnOff(ch, LED_CCW, false);
    }
}

void Drive_Stop(sMotionCommand_t* pCmd)
{
    S32 Acc;
        
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; }
    
    if(pCmd->Axis == 0xFF)
    {   
        TMC429_MotorStop(MOTION_CH0, Acc);
        TMC429_MotorStop(MOTION_CH1, Acc);
        
        DriveSts[MOTION_CH0].Bit.Run = FALSE;
        DriveSts[MOTION_CH1].Bit.Run = FALSE;
        
        if(HomingSeq[MOTION_CH0].Step != HOMING_COMPLETE) { HomingSeq[MOTION_CH0].Step = HOMING_NONE; }
        if(HomingSeq[MOTION_CH1].Step != HOMING_COMPLETE) { HomingSeq[MOTION_CH1].Step = HOMING_NONE; }
    }
    else
    {
        TMC429_MotorStop(pCmd->Axis, Acc);
        
        DriveSts[pCmd->Axis].Bit.Run = FALSE;
        if(HomingSeq[pCmd->Axis].Step != HOMING_COMPLETE) { HomingSeq[pCmd->Axis].Step = HOMING_NONE; }
    }   
}

void Drive_SetPosition(sMotionCommand_t* pCmd)
{   
    TMC429_MotorStop(pCmd->Axis, 2047);
    TMC429_SetPosition(pCmd->Axis, pCmd->Pos);
}

void Drive_ContinusVelMove(sMotionCommand_t* pCmd)
{
    S32 Acc, Vel;
    
    if(DriveSts[pCmd->Axis].Bit.HomingRun) { return; }
    
    if((pCmd->Vel > 0) && DriveSts[pCmd->Axis].Bit.PositiveLimit) { return; }
    if((pCmd->Vel < 0) && DriveSts[pCmd->Axis].Bit.NegativeLimit) { return; }
    
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; } 
    
    Vel = (S32)((F32)pCmd->Vel / (F32)STEP_PULSE_RATE_R);
    if(Vel > MAX_VELOCITY) { Vel = MAX_VELOCITY; }
    if(Vel < MIN_VELOCITY) { Vel = MIN_VELOCITY; } 
    
    TMC2660_SetRunCurrent(pCmd->Axis);

    DriveSts[pCmd->Axis].Bit.Dir = TMC429_VelMove(pCmd->Axis, Acc, Vel);
    DriveSts[pCmd->Axis].Bit.Run = TRUE;
}

void Drive_VelMove(sMotionCommand_t* pCmd)
{
    S32 Acc, Vel;
    
    if(DriveSts[pCmd->Axis].Bit.HomingRun) { return; }      
    
    if((pCmd->Vel > 0) && DriveSts[pCmd->Axis].Bit.PositiveLimit) { return; }
    if((pCmd->Vel < 0) && DriveSts[pCmd->Axis].Bit.NegativeLimit) { return; }
    
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; } 
    
    Vel = (S32)((F32)pCmd->Vel / (F32)STEP_PULSE_RATE_R);
    if(Vel > MAX_VELOCITY) { Vel = MAX_VELOCITY; }
    if(Vel < MIN_VELOCITY) { Vel = MIN_VELOCITY; } 
    
    TMC2660_SetRunCurrent(pCmd->Axis);

    DriveSts[pCmd->Axis].Bit.Dir = TMC429_VelMove(pCmd->Axis, Acc, Vel);
    DriveSts[pCmd->Axis].Bit.Run = TRUE;
}

void Drive_RelMove(sMotionCommand_t* pCmd)
{
    S32 Acc, Vel;
    
    if(DriveSts[pCmd->Axis].Bit.HomingRun) { return; }         
    
    if((pCmd->Pos > 0) && DriveSts[pCmd->Axis].Bit.PositiveLimit) { return; }
    if((pCmd->Pos < 0) && DriveSts[pCmd->Axis].Bit.NegativeLimit) { return; }
    
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; } 
    
    Vel = (S32)((F32)pCmd->Vel / (F32)STEP_PULSE_RATE_R);
    if(Vel < 0) { Vel = (-1) * Vel; }
    if(Vel > MAX_VELOCITY) { Vel = MAX_VELOCITY; }
    
    TMC2660_SetRunCurrent(pCmd->Axis);

    DriveSts[pCmd->Axis].Bit.Dir = TMC429_RelMove(pCmd->Axis, Acc, Vel, pCmd->Pos);
    DriveSts[pCmd->Axis].Bit.Run = TRUE;
}

void Drive_AbsMove(sMotionCommand_t* pCmd)
{
    S32 Acc, Vel;
    
    if(DriveSts[pCmd->Axis].Bit.HomingRun) { return; }
    
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; } 
    
    Vel = (S32)((F32)pCmd->Vel / (F32)STEP_PULSE_RATE_R);
    if(Vel < 0) { Vel = (-1) * Vel; }
    if(Vel > MAX_VELOCITY) { Vel = MAX_VELOCITY; }
 
    TMC2660_SetRunCurrent(pCmd->Axis);

    DriveSts[pCmd->Axis].Bit.Dir = TMC429_AbsMove(pCmd->Axis, Acc, Vel, pCmd->Pos);
    DriveSts[pCmd->Axis].Bit.Run = TRUE;
}

void Drive_Homing(sMotionCommand_t* pCmd)
{  
    S32 Acc, Vel;
    
    if((HOMING_NONE < HomingSeq[pCmd->Axis].Step) && (HomingSeq[pCmd->Axis].Step < HOMING_COMPLETE)) { return; }    
    
    Acc = (S32)((F32)pCmd->Acc / (F32)A_MAX_BASE);
    if(Acc > MAX_ACC) { Acc = MAX_ACC; }
    if(Acc < MIN_ACC) { Acc = MIN_ACC; } 
        
    Vel = (S32)((F32)pCmd->Vel / (F32)STEP_PULSE_RATE_R);
    if(Vel > MAX_VELOCITY) { Vel = MAX_VELOCITY; }
    if(Vel < MIN_VELOCITY) { Vel = MIN_VELOCITY; }
    
    if(Vel > 0) { HomingSeq[pCmd->Axis].Dir = 0; }
    else        { HomingSeq[pCmd->Axis].Dir = 1; }
    
    HomingSeq[pCmd->Axis].H_Acc = Acc;
    HomingSeq[pCmd->Axis].H_Vel = Vel;
    HomingSeq[pCmd->Axis].L_Acc = Acc;
    HomingSeq[pCmd->Axis].L_Vel = (-1)*(Vel>>1);
    
    HomingSeq[pCmd->Axis].Step = HOMING_START;
}

void Drive_GetIO(void)
{
    static uINPUT_t CurIn[MOTION_CH_MAX], PreIn[MOTION_CH_MAX], TempIn[MOTION_CH_MAX];
    static U16 debounceCnt[MOTION_CH_MAX]; 
    sMotionLimit_t*     sLimit[MOTION_CH_MAX];
    
    static U8 bFirstInit = FALSE;
    U8 ch;
    
    sLimit[MOTION_CH0] = &MotionLimit[MOTION_CH0];
    sLimit[MOTION_CH1] = &MotionLimit[MOTION_CH1];
    
    CurIn[MOTION_CH0].Bit.Reserved = 0;
    CurIn[MOTION_CH1].Bit.Reserved = 0;
    
    CurIn[MOTION_CH0].Bit.HomeSensor = 0;
    CurIn[MOTION_CH1].Bit.HomeSensor = 0;
    
    CurIn[MOTION_CH0].Bit.NegativeLimit = 0;
    CurIn[MOTION_CH1].Bit.NegativeLimit = 0;
    
    CurIn[MOTION_CH0].Bit.PositiveLimit = 0;
    CurIn[MOTION_CH1].Bit.PositiveLimit = 0;
    
    if(bFirstInit == FALSE)
    {
        bFirstInit = TRUE;
        
        for(ch=0; ch<MOTION_CH_MAX; ch++)
        {
            PreIn [ch].Byte = CurIn[ch].Byte;
            TempIn[ch].Byte = CurIn[ch].Byte;
        }
    }
    
    for(ch=0; ch<MOTION_CH_MAX; ch++)
    {
        if(CurIn[ch].Byte != PreIn[ch].Byte)
        {
            debounceCnt[ch]++;
            if(debounceCnt[ch] > 10)
            {
                PreIn [ch].Byte = CurIn[ch].Byte;
                TempIn[ch].Byte = CurIn[ch].Byte;
            }
        }
        else
        {
            debounceCnt[ch] = 0;
        }
        
        DriveSts[ch].Bit.NegativeLimit = ((sLimit[ch]->EnableNegLimit && sLimit[ch]->PolarityNegLimit && TempIn[ch].Bit.NegativeLimit) ||
                                         (sLimit[ch]->EnableNegLimit && !sLimit[ch]->PolarityNegLimit && !TempIn[ch].Bit.NegativeLimit));
       
        DriveSts[ch].Bit.PositiveLimit = ((sLimit[ch]->EnablePosLimit && sLimit[ch]->PolarityPosLimit && TempIn[ch].Bit.PositiveLimit) ||
                                         (sLimit[ch]->EnablePosLimit && !sLimit[ch]->PolarityPosLimit && !TempIn[ch].Bit.PositiveLimit));                
        
        DriveSts[ch].Bit.HomeSensor = TempIn[ch].Bit.HomeSensor;
    }    
}

void Drive_LimitControl(void)
{
    static S32 CurPos[MOTION_CH_MAX];
    sMotionLimit_t*      sLimit[MOTION_CH_MAX];
    sMotionSwLimitPos_t* sLimitPos[MOTION_CH_MAX];
    U8 ch;    
    
    sLimit[MOTION_CH0]    = &MotionLimit[MOTION_CH0];
    sLimit[MOTION_CH1]    = &MotionLimit[MOTION_CH1];
    sLimitPos[MOTION_CH0] = &MotionSwLimitPos[MOTION_CH0];
    sLimitPos[MOTION_CH1] = &MotionSwLimitPos[MOTION_CH1];
    
    for(ch=0; ch<MOTION_CH_MAX; ch++)
    {
        CurPos[ch] = TMC429_GetPosition(ch);
        
        if((DriveSts[ch].Bit.PositiveLimit && !DriveSts[ch].Bit.Dir && DriveSts[ch].Bit.Run) ||
           (DriveSts[ch].Bit.NegativeLimit && DriveSts[ch].Bit.Dir && DriveSts[ch].Bit.Run))
        {
            TMC429_MotorStop(ch, 2047);
            DriveSts[ch].Bit.Run = FALSE;
        }
        
        if(sLimit[ch]->EnableSoftLimit && DriveSts[ch].Bit.Run)
        {
            if(((sLimitPos[ch]->SwPosLimit < CurPos[ch]) && !DriveSts[ch].Bit.Dir) ||
               ((sLimitPos[ch]->SwNegLimit > CurPos[ch]) && DriveSts[ch].Bit.Dir))
            {
                TMC429_MotorStop(ch, 2047);
                DriveSts[ch].Bit.Run = FALSE;
            }
        }
    }
}

void Drive_HomeControl(U8 ch)
{
    sHomingSequence_t*   Homing;
    
    Homing = &HomingSeq[ch];
    
    if((HOMING_NONE < Homing->Step) && (Homing->Step < HOMING_COMPLETE)) { DriveSts[ch].Bit.HomingRun = TRUE;  }
    else                                                                 { DriveSts[ch].Bit.HomingRun = FALSE; }
    
    if(Homing->Step == HOMING_COMPLETE) { DriveSts[ch].Bit.HomingComplete = TRUE;  }
    else                                { DriveSts[ch].Bit.HomingComplete = FALSE; }
        
    if((Homing->Step == HOMING_NONE) || (Homing->Step == HOMING_COMPLETE)) { return; }
       
    switch(Homing->Step)
    {
    case HOMING_START: 
                 Homing->Step = HOMING_H_SEARCH;
        DriveSts[ch].Bit.Run  = FALSE;
        break;
    
    case HOMING_H_SEARCH: 
        DriveSts[ch].Bit.Dir  = TMC429_VelMove(ch, Homing->H_Acc, Homing->H_Vel);
                 Homing->Step = HOMING_H_REACH;
        break;

    case HOMING_H_REACH: 
        if(!Homing->Dir)
        {
            if(!DriveSts[ch].Bit.PositiveLimit && !DriveSts[ch].Bit.HomeSensor) { return; }

            TMC429_MotorStop(ch, Homing->H_Acc);            
            Homing->Step = HOMING_H_WAIT;
        }
        else
        {
            if(!DriveSts[ch].Bit.NegativeLimit && !DriveSts[ch].Bit.HomeSensor) { return; }

            TMC429_MotorStop(ch, Homing->H_Acc);            
            Homing->Step = HOMING_H_WAIT;
        }
        break;

    case HOMING_H_WAIT: 
        if(TMC2660_GetMotorRun(ch) == TRUE) { return; }
        Homing->Step = HOMING_L_SEARCH;
        break;
        
    case HOMING_L_SEARCH: 
        DriveSts[ch].Bit.Dir = TMC429_VelMove(ch, Homing->L_Acc, Homing->L_Vel);
        
        if(!Homing->Dir)
        {
            if(!DriveSts[ch].Bit.PositiveLimit && !DriveSts[ch].Bit.HomeSensor)
            {
                Homing->Step = HOMING_L_REACH_OFF;
            }
            else
            {
                Homing->Step = HOMING_L_REACH_ON;
            }                      
        }
        else
        {
            if(!DriveSts[ch].Bit.NegativeLimit && !DriveSts[ch].Bit.HomeSensor)
            {
                Homing->Step = HOMING_L_REACH_OFF;
            }
            else
            {
                Homing->Step = HOMING_L_REACH_ON;
            }
        }
        break;
    
    case HOMING_L_REACH_OFF: 
        if(!Homing->Dir)
        {
            if(DriveSts[ch].Bit.PositiveLimit || DriveSts[ch].Bit.HomeSensor)
            {
                Homing->Step = HOMING_L_REACH_ON;
            }
        }
        else
        {
            if(DriveSts[ch].Bit.NegativeLimit || DriveSts[ch].Bit.HomeSensor)
            {
                Homing->Step = HOMING_L_REACH_ON;
            }
        }
        break;
        
    case HOMING_L_REACH_ON: 
        if(!Homing->Dir)
        {
            if(!DriveSts[ch].Bit.PositiveLimit && !DriveSts[ch].Bit.HomeSensor)
            {
                TMC429_MotorStop(ch, Homing->L_Acc);            
                Homing->Step = HOMING_L_WAIT;
            }
        }
        else
        {
            if(!DriveSts[ch].Bit.NegativeLimit && !DriveSts[ch].Bit.HomeSensor)
            {
                TMC429_MotorStop(ch, Homing->L_Acc);            
                Homing->Step = HOMING_L_WAIT;
            }
        }
        break;    

    case HOMING_L_WAIT: 
        if(TMC2660_GetMotorRun(ch) == TRUE) { return; }
        Homing->Step = HOMING_DONE;
        break;
     
    case HOMING_DONE: 
        TMC429_SetPosition(ch, 0);
        Homing->Step = HOMING_COMPLETE;
        break; 
   
    case HOMING_COMPLETE: 
        break;        
        
    case HOMING_NONE: 
        break;
    }
}


#ifdef DRIVE_DEBUG
__attribute__ ((unused)) static U8 state = 0;
void Drive_Test(void)
{
    U8  ch;
    U8  SelMaxCur;
    F32 RunCur;
    U8  StopCurRate;
    U8  StepResolution;
    __attribute__ ((unused)) static U32 delayCount = 0;
    static U8  PowerEnable[2] = {1,1};

    static U8 SetDrive = true;

    if(SetDrive == true)
    {
        SelMaxCur = CUR_MAX_31A;

        StepResolution = R_3200;

        RunCur = 1.2f * 1.0f;// 1.2A * 100%
        StopCurRate = 10; //Run Current * 30%
        Drive_SetCurrent(0, RunCur, StopCurRate);

        RunCur = 1.2f * 1.0f;// 1.2A * 100%
        StopCurRate = 10; //Run Current * 30%
        Drive_SetCurrent(1, RunCur, StopCurRate);

        for(ch=0; ch<STEP_CH_MAX; ch++)
        {
            Drive_SelMaxCurrent(ch, SelMaxCur);
            Drive_SetResoultion(ch, StepResolution);
        }

        TestMotionCmd[0].Axis = STEP_CH0;
        TestMotionCmd[0].Mode = MODE_NONE;
        TestMotionCmd[0].Vel = 3200*2;
        TestMotionCmd[0].Acc = TestMotionCmd[0].Vel/0.05f;
        TestMotionCmd[0].Pos = 0;
        Drive_Stop(&TestMotionCmd[0]);

        TestMotionCmd[1].Axis = STEP_CH1;
        TestMotionCmd[1].Mode = MODE_NONE;
        TestMotionCmd[1].Vel = 3200*2;
        TestMotionCmd[1].Acc = TestMotionCmd[1].Vel/0.05f;
        TestMotionCmd[1].Pos = 0;
        Drive_Stop(&TestMotionCmd[1]);

        SetDrive = false;

        for(ch=0; ch<STEP_CH_MAX; ch++)
        	Drive_PowerEnable(ch, PowerEnable[ch]);
    }



    for(ch=0; ch<STEP_CH_MAX; ch++)
    {
    	Drive_GetStatus(ch, &TestMotionStatus[ch]);
        switch(TestMotionCmd[ch].Mode)
        {
        /*
        MODE_STOP       = 0,
		MODE_SET_POS    = 1,
		MODE_CONTINUOUS = 2,
		MODE_VEL		= 3,
		MODE_REL  		= 4,
		MODE_ABS		= 5,
		MODE_HOMING     = 6,
		MODE_NONE
         */
        case MODE_STOP      : Drive_Stop(&TestMotionCmd[ch]);                break;
        case MODE_SET_POS   : Drive_SetPosition(&TestMotionCmd[ch]);         break;
        case MODE_CONTINUOUS: Drive_ContinusVelMove(&TestMotionCmd[ch]);     break;
        case MODE_VEL       : Drive_VelMove(&TestMotionCmd[ch]);             break;
        case MODE_REL       : Drive_RelMove(&TestMotionCmd[ch]);             break;
        case MODE_ABS       : Drive_AbsMove(&TestMotionCmd[ch]);             break;
        case MODE_HOMING    : Drive_Homing(&TestMotionCmd[ch]);              break;
        }
        if(TestMotionCmd[ch].Mode != MODE_NONE) { TestMotionCmd[ch].Mode = MODE_NONE; }

        TMC2660_ControlCurrent(ch);

        TestTMC2660_Status[ch] = TMC2660_ReadResponse(ch, 1);
    }
//
//    switch(state)
//    {
//    case 0 :
//    	TestMotionCmd[0].Pos = 0;
//    	Drive_SetPosition(&TestMotionCmd[0]);
//    	state++;
//    	break;
//    case 1 :
//    	TestMotionCmd[0].Pos = 3200;
//    	Drive_RelMove(&TestMotionCmd[0]);
//    	state++;
//    	break;
//    case 2 :
//    	if(TestMotionStatus[0].Pos == 3200)
//    	{
//    		state++;
//    	}
//    	break;
//    case 3 :
//    	TestMotionCmd[0].Pos = -3200;
//		Drive_RelMove(&TestMotionCmd[0]);
//		state++;
//    case 4 :
//    	if(TestMotionStatus[0].Pos == 0)
//    	{
//    		state++;
//    	}
//    	break;
//    case 5 :
//    	delayCount++;
//    	if(delayCount > 500)
//    	{
//    		delayCount = 0;
//    		state = 1;
//    	}
//    	break;
//    }
}
#endif
