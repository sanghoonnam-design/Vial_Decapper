import pathlib,sys,subprocess,os,shutil,tempfile
b=pathlib.Path(__file__).resolve().parent
ns={'__file__':str(b/'test_motion.py')}
exec((b/'test_motion.py').read_text(encoding='utf-8').split('# COMPILE_AND_RUN')[0],ns)
code=ns['code'].split('static int failures;')[0]
code+=r'''
static int timerStops,saves,httpStatus;
bool gZCapUpPosSavePending; S32 gZCapUpPosPendingValue;
#define XTimer_Stop() (timerStops++)
#define XTimer_Start() ((void)0)
U32 SWRTC_GetTime_YYMMDDHH(void){return 0;}
void EEPROMPL_SaveToEEPROM(void){saves++;}
void SYSPL_FactorySetting(void){saves++;}
typedef struct {void *resp;} http_request_ctx_t;
void http_response_set(void*r,int status,const char*t,const char*b,int n){httpStatus=status;}
'''
read=ns['read'];fn=ns['function']
core=read('App/Task/_10_XCommand_Core.c');http=read('App/HW_Device/httpServer/user/http_post.c')
code+=fn(core,'CMD_BeginMaintenance')+fn(core,'CMD_Handle_SAVEE')
code+=fn(http,'http_begin_maintenance')+fn(http,'http_post_factorySet')
code+=r'''
static int failures;
#define CHECK(test,condition) do{if(!(condition)){printf("FAIL %s\n",test); failures++;}else printf("PASS %s\n",test);}while(0)
int main(void){tsXParsedData cmd={0};http_request_ctx_t ctx={0};
 CDecap_Init();xSL.isBusy=1;CMD_Handle_SAVEE(&cmd,0);
 CHECK("busy SAVEE never stops timer or writes",timerStops==0 && saves==0);
 timerStops=saves=0;xSL.isBusy=0;xAT=ACTION_HOME;CMD_Handle_SAVEE(&cmd,0);
 CHECK("queued motion blocks SAVEE",timerStops==0 && saves==0);
 timerStops=saves=0;xAT=ACTION_NONE;xSL.isBusy=1;http_post_factorySet(&ctx);
 CHECK("busy HTTP factory returns conflict without timer stop",httpStatus==409 && timerStops==0 && saves==0);
 timerStops=saves=0;xSL.isBusy=0;xAT=ACTION_NONE;CMD_Handle_SAVEE(&cmd,0);
 CHECK("idle save succeeds and releases maintenance",saves==1 && !CDecap_IsMaintenanceActive());
 CDecap_BeginMaintenance();
 CHECK("maintenance atomically excludes new motion",!CDecap_TryRequestAction(ACTION_HOME));CDecap_EndMaintenance();
 return failures?1:0;
}
'''
zig=os.environ.get('ZIG_EXE') or shutil.which('zig')
if not zig:sys.exit('Set ZIG_EXE to a Zig C compiler (tested with Zig 0.13.0).')
with tempfile.TemporaryDirectory(prefix='vial-maintenance-') as work:
    p=pathlib.Path(work)/'maintenance_test.c';p.write_text(code,encoding='utf-8');exe=p.with_suffix('.exe')
    r=subprocess.run([zig,'cc','-std=c11','-Wno-format','-o',str(exe),str(p)])
    result=r.returncode if r.returncode else subprocess.run([str(exe)]).returncode
sys.exit(result)
