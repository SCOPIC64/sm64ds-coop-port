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
    # The archive contains only the executable and redistributable documentation.
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        assert sorted(archive.namelist()) == sorted(manifest['archive_files'])
        assert set(archive.namelist()) <= {'sm64ds coop.exe', 'README.txt', 'LICENSE', 'THIRD-PARTY-NOTICES.txt'}
        exe = archive.read('sm64ds coop.exe')
        assert exe[:2] == b'MZ'
        assert hashlib.sha256(exe).hexdigest() == manifest['exe_sha256']
    # Refuse to replace a release/tag that already exists.
    releases = api(f'repos/{repo}/releases?per_page=100')
    assert not any(r['tag_name'] == tag for r in releases), 'Release already exists'
    refs = api(f'repos/{repo}/git/matching-refs/tags/{tag}')
    assert not any(r['ref'] == 'refs/tags/' + tag for r in refs), 'Tag already exists'
    artifact = Path('SM64DS-Coop-' + tag[1:] + '-Windows-x86.zip')
    artifact.write_bytes(data)
    notes = Path('menu-preview-notes.md')
    notes.write_text(manifest['notes'], encoding='utf-8')
    subprocess.run(['gh', 'release', 'create', tag, str(artifact), '--repo', repo,
                    '--target', source, '--title', manifest['title'], '--prerelease',
                    '--latest=false', '--notes-file', str(notes)], check=True)


if __name__ == '__main__':
    main()
