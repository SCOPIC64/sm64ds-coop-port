"""Exercise native locked-door dialogue and the host pause menu on a built game.

Requires the player's own extracted assets. It copies the EXE into a fresh work
folder and uses isolated saves/mods; existing work folders are never overwritten.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True,
                        help='Game-data root containing build/assets/romdata.bin')
    parser.add_argument('--work', type=Path, required=True,
                        help='A new directory for isolated logs, saves and captures')
    args = parser.parse_args()
    source = args.exe.resolve(strict=True)
    assets = args.assets.resolve(strict=True)
    assert (assets/'build/assets/romdata.bin').is_file(), 'Game data is missing'
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    exe = work/'sm64ds coop.exe'
    shutil.copy2(source, exe)
    env = {k:v for k,v in os.environ.items() if not k.startswith('SM64DS_')}
    env.update(SM64DS_ASSET_ROOT=str(assets), SM64DS_LEVEL='6',
               SM64DS_CHARACTER='0', SM64DS_WINDOW_SELFTEST='460',
               SM64DS_NO_AUDIO='1', SM64DS_NO_DIALOG='1', SM64DS_NO_PLAYLOG='1',
               SM64DS_NO_FOCUS='1', SM64DS_SELFTEST_IDLE='1',
               SM64DS_PAUSE_SELFTEST='1', SM64DS_PAUSE_WATCH='1',
               SM64DS_PROBE_MESSAGE='23', SM64DS_PROBE_MESSAGE_CAMERA='2',
               SM64DS_TRACE_STATE='2')
    frames = range(160,440,20)
    cases = [('no-input', {})]
    for name, key in [('enter','0d'), ('escape','1b'), ('jump','20')]:
        cases.append((name, {'SM64DS_HOST_KEY':','.join(f'{key}@{n}-{n+1}' for n in frames)}))
    attack = ','.join(f'{n}-{n+1}:A' for n in frames)
    cases.extend([
        ('attack', {'SM64DS_PROBE_INPUT':attack}),
        ('controller-start', {'SM64DS_HOST_PAD':','.join(f'10@{n}-{n+1}' for n in frames)}),
        ('pause-resume', {'SM64DS_PROBE_MESSAGE':'', 'SM64DS_SELFTEST_IDLE':'',
                          'SM64DS_HOST_KEY':'0d@600-604,0d@640-644',
                          'SM64DS_PROBE_INPUT':attack, 'SM64DS_WINDOW_SELFTEST':'700'})])
    results = []
    for name, overrides in cases:
        folder = work/name
        folder.mkdir()
        current = env | overrides
        current.update(SM64DS_SAVE_PATH=str(folder/'test.sav'), SM64DS_MODS=str(folder/'mods'))
        current = {k:v for k,v in current.items() if v!=''}
        with (folder/'run.log').open('wb') as log:
            process = subprocess.Popen([str(exe)], cwd=folder, env=current, stdout=log, stderr=log)
            try:
                code = process.wait(timeout=90)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise
        text = (folder/'run.log').read_text(errors='replace')
        assert code == 0, (name, code, text[-2000:])
        assert not any(s in text for s in ('caught fault','[quarantine]','FAULT code','unhosted state'))
        states = re.findall(r'\[pause\] f(\d+) .*msg=(\d+)', text)
        if name == 'pause-resume':
            assert '[pause] opened frame=600' in text and '[pause] resumed frame=640' in text
            positions = re.findall(r'\[f(\d+)\] pos=\(([^)]+)\)', text)
            paused = [p for f,p in positions if 601<=int(f)<=639]
            assert len(paused)>=20 and len(set(paused))==1
            assert positions[-1][1]!=paused[-1], 'Movement did not resume'
        else:
            assert any(int(f)>100 and m=='1' for f,m in states), 'Dialogue did not open'
            assert states[-1][1] == ('1' if name=='no-input' else '0'), 'Dialogue did not close'
            for frame in re.findall(r'\[pause\] opened frame=(\d+)', text):
                earlier = [m for f,m in states if int(f)<int(frame)]
                assert earlier and earlier[-1]=='0', 'Pause stole dialogue input'
        results.append({'test':name, 'exit':code})
        (work/'results.json').write_text(json.dumps({
            'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(), 'tests':results}, indent=2))
        print('PASS', name, flush=True)


if __name__ == '__main__':
    main()
