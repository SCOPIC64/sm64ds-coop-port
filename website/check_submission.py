"""Run only from a trusted PR base checkout. Head files are JSON data, never code."""
import base64
import json
import os
from pathlib import Path
import re
import urllib.request
from validate_catalog import APPROVALS, CATALOG, review_submission

REPO = 'SCOPIC64/sm64ds-coop-port'
event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text())
assert event['repository']['full_name'] == REPO
pr = event['pull_request']
assert pr['base']['repo']['full_name'] == REPO
head = pr['head']['sha']
assert re.fullmatch(r'[0-9a-f]{40}', head)


def api(path):
    request = urllib.request.Request('https://api.github.com/repos/' + REPO + '/' + path,
                                     headers={'Accept': 'application/vnd.github+json',
                                              'Authorization': 'Bearer ' + os.environ['GH_TOKEN']})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def head_json(path):
    value = api('contents/' + path + '?ref=' + head)
    assert value['type'] == 'file' and value['encoding'] == 'base64'
    assert value['size'] <= 128 * 1024, 'Catalogue file is too large'
    data = base64.b64decode(value['content'])
    assert len(data) <= 128 * 1024
    return json.loads(data)


changed = []
for page in range(1, 31):
    files = api('pulls/' + str(pr['number']) + '/files?per_page=100&page=' + str(page))
    for file in files:
        changed.append(file['filename'])
        if file.get('previous_filename'):
            changed.append(file['previous_filename'])
    if len(files) < 100:
        break
else:
    raise AssertionError('Submission has too many changed files')
before = json.loads(Path(CATALOG).read_text())
approvals = json.loads(Path(APPROVALS).read_text())
after = head_json(CATALOG)
proposed = head_json(APPROVALS) if APPROVALS in changed and pr['user']['login'].lower() in {
    user.lower() for user in approvals['maintainers']} else None
review_submission(before, after, approvals, pr['user']['login'], changed, proposed)
print('Trusted-base modder and ownership review PASS')
