"""Publish a locally verified ROM-clean artifact; never create fake OS builds."""
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import zipfile


def api(path):
    return json.loads(subprocess.check_output(['gh', 'api', path], text=True))


def validate_archive(data, manifest, movement_script=None):
    required = {'sm64ds coop.exe', 'README.txt', 'LICENSE', 'THIRD-PARTY-NOTICES.txt'}
    allowed = required | {'mods/README.txt', 'mods/sm64-movement/main.lua',
                          'mods/resource-packs/README.txt', 'texture-work/capture/README.txt',
                          'JOIN-STEPS.txt', 'server-profile.json'}
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        members = archive.namelist()
        assert len(members) == len(set(members)), 'Duplicate archive entry'
        assert sorted(members) == sorted(manifest['archive_files'])
        assert required <= set(members) <= allowed, 'Unexpected archive entry'
        assert archive.testzip() is None, 'Corrupted archive entry'
        exe = archive.read('sm64ds coop.exe')
        assert exe[:2] == b'MZ'
        assert hashlib.sha256(exe).hexdigest() == manifest['exe_sha256']
        if 'server-profile.json' in members:
            profile = json.loads(archive.read('server-profile.json'))
            assert set(profile) <= {'lobby_url', 'relay_addr', 'discovery_key'}, 'Private profile field'
            assert profile.get('relay_addr') == 'cairo-pot.tun.ply.gg:41702', 'Unexpected public relay'
            if 'lobby_url' in profile:
                assert profile['lobby_url'].startswith('https://'), 'Invalid public lobby URL'
            key = profile.get('discovery_key')
            assert isinstance(key, str) and len(key) == 368, 'Missing public discovery key'
            key = base64.b64decode(key, validate=True)
            # CryptoAPI PUBLICKEYBLOB: RSA1, 2048 bits. PRIVATEKEYBLOB uses
            # a different type/magic and is never a distribution profile.
            assert len(key) == 276 and key[:4] == b'\x06\x02\x00\x00', 'Private signing material'
            assert key[8:12] == b'RSA1' and int.from_bytes(key[12:16], 'little') == 2048, 'Invalid public key'
            assert 'JOIN-STEPS.txt' in members, 'Online archive needs joining instructions'
        if 'mods/sm64-movement/main.lua' in members:
            script = archive.read('mods/sm64-movement/main.lua')
            assert script == movement_script, 'Bundled movement differs from source'
            assert hashlib.sha256(script).hexdigest() == manifest['bundled_mod_sha256']


def validate_menu_pack(data, manifest):
    prefix = 'ds-classic-menu/'
    allowed = {prefix + name for name in ('panel.png', 'button.png', 'selected.png',
                                         'font.png', 'pack.lua', 'README.txt', 'provenance.json')}
    assert len(data) == manifest['bytes'] and len(data) <= 2 * 1024 * 1024
    assert hashlib.sha256(data).hexdigest() == manifest['sha256']
    assert manifest['name'] == 'DS-Classic-Menu-Texture-Pack.zip'
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        members = archive.namelist()
        assert len(members) == len(set(members)) and set(members) == allowed
        assert sum(item.file_size for item in archive.infolist()) <= 2 * 1024 * 1024
        assert archive.testzip() is None
        for name in ('panel.png', 'button.png', 'selected.png', 'font.png'):
            png = archive.read(prefix + name)
            assert len(png) >= 24 and png[:8] == b'\x89PNG\r\n\x1a\n'
            width, height = int.from_bytes(png[16:20], 'big'), int.from_bytes(png[20:24], 'big')
            assert 0 < width <= 2048 and 0 < height <= 2048
            if name == 'font.png': assert (width, height) == (288, 108)
        script = archive.read(prefix + 'pack.lua').decode('utf-8')
        assert script == 'sm64ds.menu_theme {\n panel="panel.png", button="button.png", selected="selected.png",\n font="font.png", border=4\n}\n'


def main():
    repo = os.environ['GH_REPO']
    assert repo == 'SCOPIC64/sm64ds-coop-port'
    if os.environ.get('GITHUB_EVENT_NAME') == 'pull_request':
        event=json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text())
        pr=event['pull_request']
        assert event['number']==5 and pr['head']['repo']['full_name']==repo
        assert pr['head']['ref']=='codex/coop-menu-single-exe'
    else:
        assert os.environ.get('GITHUB_REF') == 'refs/heads/codex/coop-menu-single-exe'
    manifest = json.loads(Path('tools/portable_kit/menu-preview-release.json').read_text())
    tag, source = manifest['tag'], manifest['source_commit']
    assert re.fullmatch(r'v[0-9.]+-menu-preview\.[0-9]+', tag)
    assert re.fullmatch(r'[a-f0-9]{40}', source)
    assert manifest['platforms'] == ['windows-x86']
    assert manifest['local_validation']['passed'] is True
    # Portable tests and the actual Windows host compilation must both pass.
    runs = api(f'repos/{repo}/actions/runs?head_sha={source}&per_page=100')['workflow_runs']
    checks = [r for r in runs if r['name'] == 'Portable front-end checks (not game builds)']
    assert checks and checks[0]['status'] == 'completed' and checks[0]['conclusion'] == 'success'
    blob = manifest['artifact_blob']
    assert re.fullmatch(r'[a-f0-9]{40}', blob)
    response = api(f'repos/{repo}/git/blobs/{blob}')
    assert response['encoding'] == 'base64'
    data = base64.b64decode(response['content'])
    assert hashlib.sha256(data).hexdigest() == manifest['artifact_sha256']
    # Only the executable, documentation and the reviewed default mod are allowed.
    movement_script = None
    if 'mods/sm64-movement/main.lua' in manifest['archive_files']:
        response = api(f'repos/{repo}/contents/port/mods/sm64-movement/main.lua?ref={source}')
        assert response['encoding'] == 'base64'
        movement_script = base64.b64decode(response['content'])
    validate_archive(data, manifest, movement_script)
    pack = manifest.get('optional_texture_pack')
    pack_data = None
    if pack:
        assert re.fullmatch(r'[a-f0-9]{40}', pack['blob'])
        response = api(f'repos/{repo}/git/blobs/{pack["blob"]}')
        assert response['encoding'] == 'base64'
        pack_data = base64.b64decode(response['content'])
        validate_menu_pack(pack_data, pack)
    # A repeat event may verify the identical release, but must never replace it.
    releases = api(f'repos/{repo}/releases?per_page=100')
    matches=[r for r in releases if r['tag_name']==tag]
    filename='SM64DS-Coop-' + tag[1:] + '-Windows-x86.zip'
    if matches:
        existing=matches[0]
        assert not existing['draft'] and existing['prerelease']
        assert existing['target_commitish']==source, 'Existing release has different source'
        assets=existing['assets']
        expected={filename: manifest['artifact_sha256']}
        if pack: expected[pack['name']]=pack['sha256']
        assert len(assets)==len(expected) and {a['name'] for a in assets}==set(expected)
        for asset in assets:
            assert asset['state']=='uploaded' and asset['digest']=='sha256:'+expected[asset['name']], 'Existing asset differs'
        ref=api(f'repos/{repo}/git/ref/tags/{tag}')
        assert ref['object']['type']=='commit' and ref['object']['sha']==source
        print('Verified existing release:',existing['html_url'])
        return
    refs = api(f'repos/{repo}/git/matching-refs/tags/{tag}')
    assert not any(r['ref'] == 'refs/tags/' + tag for r in refs), 'Tag already exists'
    artifact = Path(filename)
    artifact.write_bytes(data)
    artifacts=[str(artifact)]
    if pack:
        pack_artifact=Path(pack['name']);pack_artifact.write_bytes(pack_data)
        artifacts.append(str(pack_artifact))
    notes = Path('menu-preview-notes.md')
    notes.write_text(manifest['notes'], encoding='utf-8')
    subprocess.run(['gh', 'release', 'create', tag, *artifacts, '--repo', repo,
                    '--target', source, '--title', manifest['title'], '--prerelease',
                    '--latest=false', '--notes-file', str(notes)], check=True)


if __name__ == '__main__':
    main()
