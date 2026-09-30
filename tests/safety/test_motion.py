import re, subprocess, pathlib, sys, os, shutil, tempfile
base=pathlib.Path(__file__).resolve().parent
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else base.parents[1]
def read(p):
    q=root/p
    return q.read_text(encoding='utf-8-sig', errors='replace')
def stripped(s): return re.sub(r'^\s*#include[^\n]*','',s,flags=re.M)+'\n'
def function(s,name):
    matches=list(re.finditer(r'(?m)^(?:static )?(?:bool|void|int|U08|S32)\s+'+name+r'\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{' ,s))
    if not matches: return ''
    m=matches[-1]; start=m.start(); i=m.end(); depth=1
    # Bodies here contain no brace characters in string literals.
    while depth:
        if s[i]=='{': depth+=1
        if s[i]=='}': depth-=1
        i+=1
    return s[start:i]+'\n'
c=read('App/HW_Device/CDecap.c'); module=read('App/Task/_10_XCommand_Module.c')
prefix=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
typedef uint8_t U8,U08; typedef uint16_t U16; typedef uint32_t U32; typedef int32_t S32; typedef float F32;
#define YES 1
#define NO 0
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define ENABLE 1
#define GPIO_PIN_SET 1
#define READ_IN 0
#define STEP_PULSE_RATE_R 122
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
#define xprintf(...) ((void)0)
#define LOG_MSG_SEND(...) ((void)0)
#define ERR_MSG_SEND(...) ((void)0)
'''
tm=read('Drivers/BSP/STEP/TMC2660.h')
prefix+=stripped(tm[:tm.index('/*\n#pragma pack(push,1)')])+'\n#endif\n'
prefix+=stripped(read('App/HW_Device/CDecap.h'))
prefix+=stripped(read('App/System/XSystem_DB.h'))
prefix+=stripped(read('App/Common/XErrorCode.h'))
prefix+=stripped(read('App/Common/XParser.h'))
drive=read('Drivers/BSP/STEP/Drive.h'); prefix+=drive[drive.index('typedef enum'):drive.index('DRIVE_EXT void Drive_Init')]
prefix+=r'''
tsXStateList xSL; tsXControlData xCD; tsXParameterList xPL; teXActionType xAT;
U32 gTriggerCount; static S32 position[2]; static int moveCount, stopCount; static sMotionCommand_t lastMove;
static int errorCode; static char response[128]; void *xSendMsg;
#define NO_COMMA 0
#define COMMA 1
void SetErrorCode(teErrorCode e,const char*f,int l){errorCode=e;}
int GetErrorCode_int(void){return errorCode;}
const char *GetErrorCode_char(void){return "E0000";}
void ClearError(void){errorCode=0;}
void XBuffer_AddString(void*b,const char*s,int comma){snprintf(response,sizeof response,"%s",s);}
void xParser_HandleError(const char*f,teXParsingErrorCode c,U08 n){}
void Drive_SelMaxCurrent(U8 a,U8 b){} void Drive_SetCurrent(U8 a,F32 b,U8 d){}
void Drive_SetResoultion(U8 a,U8 b){} void Drive_SetHwLimit(U8 a,sMotionLimit_t b){}
void Drive_SetSwLimitPos(U8 a,sMotionSwLimitPos_t b){} void Drive_PowerEnable(U8 a,U8 b){}
void TMC429_SetPosition(U8 a,S32 p){position[a]=p;}
S32 TMC429_GetPosition(U8 a){return position[a];}
S32 TMC429_GetVelocity(U8 a){return 0;}
U8 TMC2660_GetMotorRun(U8 a){return a?xSL.xR_Motor_Run.Motor_Run:xSL.xZ_Motor_Run.Motor_Run;}
void IOEXP_WriteIObit(U8 a,U8 b){} U8 IOEXP_ReadIObit(U8 a,U8 b){return 1;}
void Drive_RelMove(sMotionCommand_t*p){lastMove=*p;moveCount++;}
void Drive_AbsMove(sMotionCommand_t*p){lastMove=*p;moveCount++;}
void Drive_Stop(sMotionCommand_t*p){stopCount++;}
void TMC2660_SetHoldCurrent(U8 a){}
U8 TMC429_VelMove(U8 a,U32 b,S32 d){return 0;}
void TMC429_MotorStop(U8 a,U32 b){}
'''
code=prefix+stripped(c)
for name in ['CMD_RejectIfDecapperBusy','CMD_Handle_STOP','CMD_Handle_PAUSE','CMD_Handle_RESUME','CMD_Handle_HOME']:
    code+=function(module,name)
code+=r'''
static int failures;
#define CHECK(test,condition) do{if(!(condition)){printf("FAIL %s\n",test); failures++;}else printf("PASS %s\n",test);}while(0)
static void reset(void){
 memset(&xSL,0,sizeof xSL);memset(&xPL,0,sizeof xPL);memset(&xCD,0,sizeof xCD);
 xAT=ACTION_NONE;errorCode=0;gTriggerCount=0;
 xPL.Decapper.Limit_PosZ=160000;xPL.Decapper.Limit_PosR=100000;
 xPL.Decapper.SoftLimitEnable=1;
 xPL.Decapper.SwNegLimit[0]=-160000;xPL.Decapper.SwPosLimit[0]=160000;
 xPL.Decapper.SwNegLimit[1]=-100000;xPL.Decapper.SwPosLimit[1]=100000;
 CDecap_Init();moveCount=stopCount=0;
}
int main(void){tsXParsedData cmd={0};
 reset();position[0]=150000; CDecap_Relmove(SUB_zRMOVE,0,300000,20000,20000,160000);
 CHECK("relative endpoint outside limit never sent",moveCount==0);
 reset();xPL.Decapper.Limit_PosZ=10000;xCD.Decapper.Motor_TargetPos[0]=11000;CDecap_Absmove_Z();
 CHECK("Z absolute uses configured limit",moveCount==0);
 reset();CDecap_Relmove(SUB_zRMOVE,0,300000,20000,500,160000);
 CHECK("valid relative move unchanged",moveCount==1 && lastMove.Pos==500);
 reset();position[0]=159500;CDecap_Relmove(SUB_zRMOVE,0,300000,20000,500,160000);
 CHECK("exact positive boundary accepted",moveCount==1 && lastMove.Pos==500);
 reset();position[0]=-159500;CDecap_Relmove(SUB_zRMOVE,0,300000,20000,-500,160000);
 CHECK("exact negative boundary accepted",moveCount==1 && lastMove.Pos==-500);
 reset();position[0]=1000;CDecap_Relmove(SUB_zRMOVE,0,300000,20000,INT32_MAX,160000);
 CHECK("relative addition cannot wrap into valid range",moveCount==0);
 reset();xPL.Decapper.SwPosLimit[0]=5000;CDecap_Absmove(SUB_zAMOVE,0,300000,20000,6000,160000);
 CHECK("narrower software limit enforced",moveCount==0);
 reset();CHECK("invalid automatic target fails without motion",!CD_ABSMove(0,300000,20000,170000,160000) && moveCount==0);
 reset();xPL.Decapper.SwPosLimit[0]=1000;position[0]=1001;xSL.xZ_Motor_Run.Motor_Run=1;sPhase=DFSM_AMOVEZ;
 CDecap_Action_Statemachine(ACTION_NONE);
 CHECK("out of range moving axis stops",stopCount>=2 && xSL.isError);
 reset();xSL.isBusy=1;sPhase=DFSM_CAP;CMD_Handle_STOP(&cmd,0);CMD_Handle_RESUME(&cmd,0);CDecap_Action_Statemachine(xAT);
 CHECK("STOP survives later RESUME",stopCount>=2 && !xSL.isBusy);
 reset();gTriggerCount=501;CDecap_CheckTimeout(5000);
 CHECK("timeout has a nonzero error code",errorCode!=0 && xSL.errorCode!=0);
 CDecap_Action_Statemachine(ACTION_NONE);CMD_Handle_HOME(&cmd,0);
 CHECK("fault prevents new motion request",xAT==ACTION_NONE);
 CDecap_Error_Clear();CMD_Handle_HOME(&cmd,0);
 CHECK("explicit idle clear permits new motion",!xSL.isError && xAT==ACTION_HOME && errorCode==0);
 reset();gTriggerCount=501;CDecap_CheckTimeout(5000);CMD_Handle_STOP(&cmd,0);CDecap_Action_Statemachine(xAT);
 CHECK("STOP remains available during fault",xAT==ACTION_NONE && xSL.isError);
 reset();xSL.Decapper.Z_HL_isError=1;CDecap_Action_Statemachine(ACTION_NONE);
 CHECK("deferred sensor interlock remains disabled",!xSL.isError);
 return failures?1:0;
}
'''
# COMPILE_AND_RUN
zig=os.environ.get('ZIG_EXE') or shutil.which('zig')
if not zig: sys.exit('Set ZIG_EXE to a Zig C compiler (tested with Zig 0.13.0).')
with tempfile.TemporaryDirectory(prefix='vial-motion-') as work:
    out=pathlib.Path(work)
    (out/'motion_test.c').write_text(code,encoding='utf-8')
    build=subprocess.run([zig,'cc','-std=c11','-Wno-format','-o',str(out/'motion_test.exe'),str(out/'motion_test.c')])
    result=build.returncode if build.returncode else subprocess.run([str(out/'motion_test.exe')]).returncode
sys.exit(result)
