"""Isolated native castle test with induced internet latency, jitter and loss."""
import argparse, hashlib, json, re, shutil, sys, time
from pathlib import Path
import mp2_proof as M

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--exe',type=Path,required=True)
p.add_argument('--assets',type=Path,required=True)
p.add_argument('--plans',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--baseline',action='store_true')
a=p.parse_args(); root=a.output.resolve(); root.mkdir(parents=True,exist_ok=False)
plans=json.loads(a.plans.read_text()); clients=[]; code='I'+str(__import__('os').getpid())[-6:]
for slot,role in enumerate(('parent','child')):
    d=root/role; d.mkdir(); (d/'appdata').mkdir(); (d/'mods').mkdir(); (d/'mods/enabled.txt').write_text('')
    shutil.copy2(a.exe,d/'game.exe')
    (d/'settings.json').write_text(json.dumps({'CameraMode':'ds','MovementMod':'off','VoiceEnabled':0,'Volume':0}))
    env=M.env_base(str(a.assets.resolve()),str(d),'internet'+str(slot)); env.update(plans[role])
    env.update(APPDATA=str(d/'appdata'),SM64DS_SAVE_PATH=str(d/'isolated.sav'),
        SM64DS_COMMS_CODE=code,SM64DS_COMMS_PORT='52943',SM64DS_COMMS_RELAY='127.0.0.1:41234',
        SM64DS_COMMS_DELAY_MS='180',SM64DS_COMMS_JITTER_MS='60',SM64DS_COMMS_DROP='2',
        SM64DS_OWNER_SYNC='0' if a.baseline else '1', SM64DS_LEVEL='2',SM64DS_ENTRANCE='4',
        SM64DS_WINDOW_SELFTEST='720',SM64DS_PACE_SELFTEST='1',SM64DS_NO_AUDIO='1',SM64DS_NO_PLAYLOG='1',
        SM64DS_MODS=str(d/'mods'),SM64DS_COMMS_INJECT='key=0',SM64DS_LC_WATCH='1',
        SM64DS_SYNC_PERF_PROBE='1',SM64DS_SYNC_REPORT='0',
        SM64DS_WARP_SEQ='' if a.baseline else '1/12@240,2/4@420')
    env.pop('SM64DS_COMMS_REPORT',None)
    if slot: time.sleep(2)
    proc=M.spawn(str(d/'game.exe'),str(d),env,str(d/'run.log'));clients.append((d,proc))
    print('START',role,proc.pid,flush=True)
deadline=time.monotonic()+140; checks=[]
while any(q.poll() is None for _,q in clients) and time.monotonic()<deadline:time.sleep(.2)
for d,q in clients:
    if q.poll() is None:q.terminate();q.wait()
    log=(d/'run.log').read_text(errors='replace')
    clocks=[tuple(map(int,m)) for m in re.findall(r'\[sync-perf\] frame=(\d+) t=(\d+)',log)]
    intervals=[(y[0]-x[0],y[1]-x[1]) for x,y in zip(clocks,clocks[1:]) if y[1]>x[1] and x[0]>=90 and y[0]>x[0]][-6:]
    rate=sum(i[0] for i in intervals)*1000/sum(i[1] for i in intervals) if intervals else 0
    rows=[tuple(map(int,m)) for m in re.findall(r'\[lcwatch\] f(\d+) f20c=(\d+) f2d4=(\d+)',log)]
    checks.extend([{'name':d.name+': clean exit','passed':q.returncode==0},
        {'name':d.name+': no false results menu','passed':bool(rows) and not any(r[1] for r in rows),'detail':rows},
        {'name':d.name+': steady tick rate under delayed/lossy packets','passed':len(intervals)==6 and rate>=28,'detail':{'ticks_per_second':rate,'intervals':intervals}},
        {'name':d.name+': zero buffered local frames','passed':'indelay=0 starved=0' in log}])
    if not a.baseline:checks.append({'name':d.name+': returned to main floor','passed':'level 1 -> 2, entrance 4' in log})
result={'passed':all(c['passed'] for c in checks),'exe_sha256':hashlib.sha256(a.exe.read_bytes()).hexdigest(),
    'network':{'one_way_delay_ms':180,'jitter_ms':60,'drop_percent':2},'checks':checks}
(root/'results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2),flush=True)
sys.exit(0 if result['passed'] else 1)
