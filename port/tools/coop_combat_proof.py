# Two isolated minimized clients. Existing assets and exported startup plans are read-only.
import argparse, json, os, re, shutil, sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mp2_proof as M

p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--run',required=True)
p.add_argument('--assets',required=True);p.add_argument('--plans',required=True);p.add_argument('--output',required=True)
p.add_argument('--combat',action='store_true');p.add_argument('--frames',type=int,default=1200)
p.add_argument('--baseline',action='store_true');p.add_argument('--analyze',action='store_true');args=p.parse_args()
assert re.fullmatch(r'[A-Za-z0-9_-]+',args.run), 'Use a simple new run folder name'
root=Path(args.output).resolve()/args.run
if not args.analyze:root.mkdir(parents=True,exist_ok=False)
assets=Path(args.assets).resolve(strict=True)
plans=json.loads(Path(args.plans).read_text())
code=('B' if args.baseline else 'C')+str(os.getpid())[-6:];clients=[]
started=time.monotonic()
for slot,role in ([] if args.analyze else enumerate(('parent','child'))):
    d=root/role;d.mkdir();(d/'appdata').mkdir();(d/'mods').mkdir();(d/'mods/enabled.txt').write_text('')
    shutil.copy2(args.exe,d/'game.exe')
    (d/'settings.json').write_text(json.dumps({'CameraMode':'ds','MovementMod':'off','VoiceEnabled':0,'Volume':0,'FrameRate':0}))
    env=M.env_base(str(assets),str(d),'combat'+str(slot));env.update(plans[role])
    env.update(APPDATA=str(d/'appdata'),SM64DS_COMMS_CODE=code,SM64DS_COMMS_PORT='52741',
        SM64DS_SAVE_PATH=str(d/'isolated.sav'),SM64DS_WINDOW_SELFTEST=str(args.frames),
        SM64DS_PACE_SELFTEST='1',SM64DS_TRACE_PACE='1',SM64DS_NO_PLAYLOG='1',SM64DS_NO_AUDIO='1',
        SM64DS_MODS=str(d/'mods'),SM64DS_SYNC_COMBAT_PROBE='1' if args.combat else '',
        SM64DS_COMMS_INJECT='key=0',SM64DS_SYNC_REPORT='0',SM64DS_SYNC_PERF_PROBE='1',SM64DS_COMMS_DELAY_MARGIN='0')
    if not args.combat:env.pop('SM64DS_SYNC_COMBAT_PROBE',None)
    if slot:time.sleep(2)
    proc=M.spawn(str(d/'game.exe'),str(d),env,str(d/'run.log'));clients.append((slot,d,proc));print('START',role,proc.pid,flush=True)
deadline=time.monotonic()+150
while any(proc.poll() is None for _,_,proc in clients) and time.monotonic()<deadline:time.sleep(.2)
checks=[]
def check(name,pass_,detail=None):
    checks.append({'name':name,'passed':bool(pass_),'detail':detail});print('PASS' if pass_ else 'FAIL',name,detail or '',flush=True)
logs={}
if args.analyze:clients=[(slot,root/role,None) for slot,role in enumerate(('parent','child'))]
for slot,d,proc in clients:
    if proc and proc.poll() is None:proc.terminate();proc.wait()
    text=(d/'run.log').read_text(errors='replace');logs[slot]=text
    check(f'{slot}: exited',proc.poll()==0 if proc else 'selftest: '+str(args.frames)+' frames' in text,proc.poll() if proc else 'recorded log')
    clocks=[tuple(map(int,r)) for r in re.findall(r'\[sync-perf\] frame=(\d+) t=(\d+)',text)]
    rates=[(b[0]-a[0])*1000/(b[1]-a[1]) for a,b in zip(clocks,clocks[1:]) if b[1]>a[1] and a[0]>=90]
    if args.baseline:rates=[float(n)/2 for n in re.findall(r'= ([\d.]+) fps \(divider',text)]
    check(f'{slot}: online tick rate',bool(rates) and (args.combat or min(rates[-3:])>=28.0),rates)
    check(f'{slot}: no fault','EXCEPTION' not in text and '[fault]' not in text)
    if args.combat:
        hits=re.findall(r'\[combat\] owner=(\d+) attacker=(\d+) seq=(\d+) result=(-?\d+) health=(\d+)->(\d+)',text)
        landed=[h for h in hits if int(h[5])<int(h[4])]
        check(f'{slot}: receives a full damaging/death cycle',len(landed)>=3,hits)
        expected=[(0x880,0x580),(0x580,0x280),(0x280,0)]
        check(f'{slot}: damage sequence per life',all((int(h[4]),int(h[5]))==expected[n%3] for n,h in enumerate(landed)))
        check(f'{slot}: retries do not duplicate damage',len(hits)==len(set((h[1],h[2]) for h in hits)),len(hits))
        row_re=r'\[combat-test\] f=(\d+) me=(\d+) slot=(\d+) health=(\d+) state=(\d+) anim=(\d+) damage=(\d+) noctl=(\d+) inv=(\d+) t=(\d+)'
        rows=[tuple(map(int,m)) for m in re.findall(row_re,text)];logs[f'rows{slot}']=rows
if args.combat:
    for owner in (0,1):
        own=[r for r in logs[f'rows{owner}'] if r[2]==owner]
        peer=[r for r in logs[f'rows{1-owner}'] if r[2]==owner]
        for hp in (0x580,0x280,0):
            own_hp=[r for r in own if r[3]==hp];peer_hp=[r for r in peer if r[3]==hp]
            check(f'owner {owner}: health {hp} replicated',bool(own_hp and peer_hp))
        own_dead=[r for r in own if r[3]==0];peer_dead=[r for r in peer if r[3]==0]
        check(f'owner {owner}: death state/animation agree',bool(own_dead and peer_dead and
            set((r[4],r[5]) for r in own_dead)&set((r[4],r[5]) for r in peer_dead)))
        if own_dead and peer_dead:
            check(f'owner {owner}: respawn health/control replicated',
                any(r[0]>own_dead[0][0] and r[3]==0x880 and r[7]==0 for r in own) and
                any(r[0]>peer_dead[0][0] and r[3]==0x880 and r[7]==0 for r in peer))
result={'passed':all(c['passed'] for c in checks),'checks':checks,'seconds':round(time.monotonic()-started,2)}
(root/'results.json').write_text(json.dumps(result,indent=2));print('RESULT',result['passed'],root,flush=True)
sys.exit(0 if result['passed'] else 1)


