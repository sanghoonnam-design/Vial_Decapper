"""Run the real Decapper FSM against a deterministic hardware model; never operates a device."""
import pathlib,sys,os,subprocess,tempfile,json,hashlib
base=pathlib.Path(__file__).resolve().parent
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else base.parents[1]
helper=root/'tests/safety/test_motion.py'
ns={'__file__':str(helper)}
exec(helper.read_text(encoding='utf-8').split('# COMPILE_AND_RUN')[0],ns)
code=ns['code'].split('int main(void)')[0]
code=code.replace('void IOEXP_WriteIObit(U8 a,U8 b){}', 'void model_io(U8 a,U8 b); void IOEXP_WriteIObit(U8 a,U8 b){model_io(a,b);}')
code=code.replace('void Drive_AbsMove(sMotionCommand_t*p){lastMove=*p;moveCount++;}', 'void model_abs(sMotionCommand_t*p); void Drive_AbsMove(sMotionCommand_t*p){lastMove=*p;moveCount++;model_abs(p);}')
code=code.replace('void Drive_Stop(sMotionCommand_t*p){stopCount++;}', 'void model_stop(U8 ch); void Drive_Stop(sMotionCommand_t*p){stopCount++;model_stop(p->Axis);}')
code+=ns['function'](ns['module'],'CMD_Handle_LongRun')
code+=ns['function'](ns['read']('App/System/XSystem_DB.c'),'SystemDB_PL_Init_Factory')
code+=r'''
typedef struct {S32 target; int speed,delay; bool active;} Axis;
static Axis axes[2];
static int mode, startDelay, overwritten, prematureGrip, io[16], rMoves;
static S32 rDistance[8];
static int reached[2][32];
void model_io(U8 ch,U8 value){
 if(ch==AIR_CT_CAP_GRIP_PIN && io[ch]!=value && (axes[0].active||axes[1].active)) prematureGrip++;
 io[ch]=value;
}
void model_abs(sMotionCommand_t*p){
 Axis *a=&axes[p->Axis];
 if(a->active && a->target!=p->Pos) overwritten++;
 if(p->Axis==1 && rMoves<8)rDistance[rMoves++]=p->Pos-position[1];
 a->target=p->Pos; a->speed=abs(p->Vel); a->delay=startDelay; a->active=(a->target!=position[p->Axis]);
}
void model_stop(U8 ch){axes[ch].active=false;}
static void tick(void){
 gTriggerCount++;
 for(int ch=0;ch<2;ch++){
  Axis *a=&axes[ch];
  if(a->active && a->delay>0) a->delay--;
  else if(a->active && mode==0){
   S32 amount=a->speed/100;if(amount<1)amount=1;
   int64_t diff=(int64_t)a->target-position[ch];
   if(llabs(diff)<=amount){position[ch]=a->target;a->active=false;}
   else position[ch]+=diff>0?amount:-amount;
  }
  U8 busy=a->active && a->delay==0 && mode!=2;
  if(ch==0)xSL.xZ_Motor_Run.Motor_Run=busy;else xSL.xR_Motor_Run.Motor_Run=busy;
 }
 CDecap_Action_Statemachine(xAT);
 if(sPhase==DFSM_LONGRUN && CDecapping_Step<32) reached[xCD.Decapper.phaseDecapCap][CDecapping_Step]++;
}
static void setup(void){
 memset(axes,0,sizeof axes);memset(io,0,sizeof io);memset(reached,0,sizeof reached);
 mode=startDelay=overwritten=prematureGrip=rMoves=0;reset();
 xPL.Decapper.ZDecapAcc=300000;xPL.Decapper.ZDecapVel=20000;
 xPL.Decapper.RDecapAcc=300000;xPL.Decapper.RDecapVel=20000;
 xPL.Decapper.ZCap_SidePos=4000;xPL.Decapper.ZCap_UpPos=2000;
 xPL.Decapper.ZCap_Origin_Position=0;xPL.Decapper.RDecapPos=1000;
 xSL.isHomed=true;xSL.CDecapping_Sensor.CT_Detect_Sensor=1;
}
static void start(void){tsXParsedData cmd={0};CMD_Handle_LongRun(&cmd,0);}
static void run(unsigned count,unsigned ticks){for(unsigned n=0;n<ticks && !xSL.isError && xCD.Decapper.LongRunCount<count;n++)tick();}
static void result(const char*name,bool ok){printf("%s | %s | count=%lu error=%d Z=%ld R=%ld overwrite=%d earlyGrip=%d\n",ok?"PASS":"FAIL",name,(unsigned long)xCD.Decapper.LongRunCount,errorCode,(long)position[0],(long)position[1],overwritten,prematureGrip);}

static void begin_unit(teXActionType action){CDecap_TryRequestAction(action);}
static void finish_unit(void){for(int n=0;n<3000 && !xSL.isError;n++){tick();if(sPhase==DFSM_IDLE && xAT==ACTION_NONE)break;}}
int main(void){
 setbuf(stdout,NULL);
 setup();position[1]=2500;start();run(1000,1000000);
 CHECK("1000 cycles return R to nonzero starting coordinate",xCD.Decapper.LongRunCount==1000 && !xSL.isError && position[1]==2500);
 bool equal=true;for(int i=0;i<rMoves;i++)if(rDistance[i]!=(i%2?1000:-1000))equal=false;
 CHECK("every DECAP/CAP rotates the configured amount",equal && rMoves==8);
 setup();mode=2;start();run(3,3000);
 CHECK("stopped feedback without arrival times out before grip/count",xCD.Decapper.LongRunCount==0 && errorCode==ERROR_CODE_DECAP_TIMEOUT && io[AIR_CT_CAP_GRIP_PIN]==0 && !overwritten);
 setup();startDelay=3;start();run(10,10000);
 CHECK("delayed start cannot overwrite target or change grip early",xCD.Decapper.LongRunCount==10 && !xSL.isError && !overwritten && !prematureGrip && position[1]==0);
 setup();mode=1;start();run(1,1000);
 CHECK("running feedback stuck faults with no count",errorCode==ERROR_CODE_DECAP_TIMEOUT && xCD.Decapper.LongRunCount==0);
 setup();xPL.Decapper.ZCap_SidePos=xPL.Decapper.ZCap_UpPos=0;xPL.Decapper.RDecapPos=0;start();run(5,3000);
 CHECK("already at target completes without observing motor ON",xCD.Decapper.LongRunCount==5 && !xSL.isError);
 setup();position[1]=4000;begin_unit(ACTION_UCAP);finish_unit();begin_unit(ACTION_UCAP);finish_unit();
 CHECK("standalone CAP adds same amount on every request",position[1]==6000 && !xSL.isError);
 begin_unit(ACTION_UDECAP);finish_unit();begin_unit(ACTION_UDECAP);finish_unit();
 CHECK("standalone DECAP subtracts same amount on every request",position[1]==4000 && !xSL.isError);
 setup();position[1]=99500;begin_unit(ACTION_UCAP);finish_unit();
 CHECK("relative R endpoint beyond limit faults",errorCode==ERROR_CODE_DECAP_LIMIT && rMoves==0);
 setup();xPL.Decapper.RDecapPos=INT32_MIN;begin_unit(ACTION_UDECAP);finish_unit();
 CHECK("R delta arithmetic cannot overflow into valid goal",errorCode==ERROR_CODE_DECAP_LIMIT && rMoves==0);
 setup();mode=2;xCD.Decapper.Motor_TargetPos[0]=500;begin_unit(ACTION_AMOVEZ);finish_unit();
 CHECK("manual absolute move needs arrival and times out",errorCode==ERROR_CODE_DECAP_TIMEOUT);
 setup();mode=2;xCD.Decapper.Motor_TargetPos[0]=500;begin_unit(ACTION_RMOVEZ);finish_unit();
 CHECK("manual relative move needs arrival and times out",errorCode==ERROR_CODE_DECAP_TIMEOUT);
 setup();startDelay=3;position[1]=4000;begin_unit(ACTION_DECAP);finish_unit();
 CHECK("automatic DECAP uses same arrival checks and fixed R amount",!xSL.isError && position[1]==3000 && !overwritten && !prematureGrip);
 begin_unit(ACTION_CAP);finish_unit();
 CHECK("automatic CAP restores original R with fixed amount",!xSL.isError && position[1]==4000 && !overwritten && !prematureGrip);
 setup();startDelay=3;xCD.Decapper.Motor_TargetPos[0]=500;begin_unit(ACTION_AMOVEZ);finish_unit();
 CHECK("manual absolute valid move completes",!xSL.isError && position[0]==500 && sPhase==DFSM_IDLE);
 xCD.Decapper.Motor_TargetPos[0]=-200;begin_unit(ACTION_RMOVEZ);finish_unit();
 CHECK("manual relative valid move completes at computed goal",!xSL.isError && position[0]==300 && sPhase==DFSM_IDLE);
 setup();mode=1;xCD.Decapper.Motor_TargetPos[0]=500;begin_unit(ACTION_AMOVEZ);tick();position[0]=500;finish_unit();
 CHECK("position alone is insufficient while running feedback stays ON",errorCode==ERROR_CODE_DECAP_TIMEOUT);
 int cases=0,failed=0;
 for(int delay=0;delay<=3;delay+=3)for(int ph=0;ph<2;ph++)for(int st=0;st<20;st++){
  setup();startDelay=delay;start();bool found=false;
  for(int n=0;n<3000 && xCD.Decapper.LongRunCount<2;n++){tick();if(sPhase==DFSM_LONGRUN && xCD.Decapper.phaseDecapCap==ph && CDecapping_Step==st){found=true;break;}}
  if(!found)continue;
  cases++;tsXParsedData cmd={0};CMD_Handle_PAUSE(&cmd,0);tick();
  int beforeMoves=moveCount;for(int n=0;n<1000;n++)tick();
  bool quiet=(moveCount==beforeMoves && !xSL.isError && PauseContext.isPaused);
  CMD_Handle_RESUME(&cmd,0);tick();run(3,5000);
  if(!(quiet && xCD.Decapper.LongRunCount==3 && !xSL.isError && !overwritten && !prematureGrip && position[1]==0)){
   failed++;printf("DETAIL pause failure delay=%d phase=%d step=%d count=%lu error=%d R=%ld\n",delay,ph,st,(unsigned long)xCD.Decapper.LongRunCount,errorCode,(long)position[1]);
  }
 }
 CHECK("PAUSE/RESUME in 28 phase/step/delay cases preserves remaining motion",cases==28 && failed==0);
 setup();gTriggerCount=UINT32_MAX-80;start();run(3,5000);
 CHECK("tick wrap preserves target tracking",xCD.Decapper.LongRunCount==3 && !xSL.isError);
 setup();start();for(int n=0;n<150;n++)tick();tsXParsedData cmd={0};CMD_Handle_STOP(&cmd,0);CMD_Handle_RESUME(&cmd,0);tick();
 int beforeMoves=moveCount;for(int n=0;n<100;n++)tick();
 CHECK("STOP cannot be undone by RESUME",sPhase==DFSM_IDLE && moveCount==beforeMoves && !xSL.isBusy);
 return failures?1:0;
}
'''
code=code.replace('void Drive_RelMove(sMotionCommand_t*p){lastMove=*p;moveCount++;}', 'void model_abs(sMotionCommand_t*p); void Drive_RelMove(sMotionCommand_t*p){lastMove=*p;moveCount++;sMotionCommand_t a=*p;a.Pos=position[p->Axis]+p->Pos;model_abs(&a);}')
with tempfile.TemporaryDirectory(prefix='vial-longrun-regression-') as work:
    source=pathlib.Path(work)/'test.c';exe=pathlib.Path(work)/'test.exe'
    source.write_text(code,encoding='utf-8')
    subprocess.run([os.environ['ZIG_EXE'],'cc','-std=c11','-Wno-format',str(source),'-o',str(exe)],check=True)
    sys.exit(subprocess.run([str(exe)]).returncode)
