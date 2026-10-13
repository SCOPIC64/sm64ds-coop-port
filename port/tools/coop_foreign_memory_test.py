import ctypes as C,json,msvcrt,os,shutil,subprocess,sys,time,_winapi
from ctypes import wintypes as W
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import mp2_proof as M
k=C.WinDLL('kernel32',use_last_error=True)
k.VirtualAllocEx.argtypes=[W.HANDLE,C.c_void_p,C.c_size_t,W.DWORD,W.DWORD];k.VirtualAllocEx.restype=C.c_void_p
k.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)];k.ReadProcessMemory.restype=W.BOOL
k.WriteProcessMemory.argtypes=k.ReadProcessMemory.argtypes;k.WriteProcessMemory.restype=W.BOOL
k.ResumeThread.argtypes=[W.HANDLE];k.ResumeThread.restype=W.DWORD
import argparse
parser=argparse.ArgumentParser(description='Launch an isolated game with a foreign constructor-table allocation established before TLS.')
parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--assets',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--baseline',action='store_true')
a=parser.parse_args();mode='baseline' if a.baseline else 'protected';exe=a.exe.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
shutil.copy2(exe,out/'game.exe');(out/'appdata').mkdir();(out/'mods').mkdir();(out/'mods/enabled.txt').write_text('speed-coordinates\n');(out/'settings.json').write_text('{"Volume":0,"MenuSounds":false}')
env=M.env_base(str(a.assets.resolve()),str(out),mode)
env.update(APPDATA=str(out/'appdata'),SM64DS_SAVE_PATH=str(out/'isolated.sav'),SM64DS_MODS=str(out/'mods'),SM64DS_WINDOW_SELFTEST='120',SM64DS_PACE_SELFTEST='1',SM64DS_NO_AUDIO='1',SM64DS_NO_PLAYLOG='1')
si=subprocess.STARTUPINFO();si.dwFlags=0x101;si.wShowWindow=7
logfile=(out/'run.log').open('wb');null=open(os.devnull,'rb');loghandle=msvcrt.get_osfhandle(logfile.fileno());nullhandle=msvcrt.get_osfhandle(null.fileno())
os.set_handle_inheritable(loghandle,True);os.set_handle_inheritable(nullhandle,True)
si.hStdInput=nullhandle;si.hStdOutput=loghandle;si.hStdError=loghandle
hp,ht,pid,tid=_winapi.CreateProcess(str(out/'game.exe'),subprocess.list2cmdline([str(out/'game.exe')]),None,None,True,M.NO_CONSOLE|4,env,str(out),si)
os.set_handle_inheritable(loghandle,False);os.set_handle_inheritable(nullhandle,False)
allocated=k.VirtualAllocEx(hp,C.c_void_p(0x02080000),0x10000,0x3000,4);marker=b'\x5a'*96;got=C.c_size_t();buf=C.create_string_buffer(marker)
if allocated!=0x02080000 or not k.WriteProcessMemory(hp,C.c_void_p(0x02086b60),buf,96,C.byref(got)):
 _winapi.TerminateProcess(hp,1);raise RuntimeError('Forced collision could not be established before loader startup')
k.ResumeThread(ht);_winapi.CloseHandle(ht);print('Forced foreign allocation established',mode,pid,flush=True)
witness=None;end=time.monotonic()+80
while _winapi.WaitForSingleObject(hp,0)==258 and time.monotonic()<end:
 log=(out/'run.log').read_text(errors='replace')
 if '[ctor] func_02072f94' in log:
  read=C.create_string_buffer(96)
  if k.ReadProcessMemory(hp,C.c_void_p(0x02086b60),read,96,C.byref(got)) and got.value==96:witness=read.raw==marker
 time.sleep(.05)
if _winapi.WaitForSingleObject(hp,0)==258:_winapi.TerminateProcess(hp,1);_winapi.WaitForSingleObject(hp,5000)
rc=_winapi.GetExitCodeProcess(hp);_winapi.CloseHandle(hp);logfile.close();null.close();log=(out/'run.log').read_text(errors='replace')
result={'mode':mode,'exit':rc,'completed':'selftest: 120 frames' in log,'collisionEstablished':True,'foreignConstructorBytesUntouched':witness,'fallback':'not owned by the DS backend' in log,'fault':'FAULT code' in log or '[fault]' in log}
result['passed']=rc==0 and result['completed'] and not result['fault'] and witness==(not a.baseline) and result['fallback']==(not a.baseline)
(out/'results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result),flush=True)
sys.exit(0 if result['passed'] else 1)
