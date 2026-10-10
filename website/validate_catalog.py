"""Validate public metadata and trusted-base modder permissions; execute no mods."""
import json
from pathlib import Path
import re
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parent
CATALOG = 'website/public/catalog.json'
APPROVALS = 'website/approved-modders.json'


def validate_approvals(value):
    assert value.get('schema') == 1
    for role in ('maintainers', 'modders'):
        users = value[role]
        assert isinstance(users, list) and users
        assert all(isinstance(user, str) and re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9-]{0,38}', user) for user in users)
        assert len({user.lower() for user in users}) == len(users)
    assert set(user.lower() for user in value['maintainers']) <= set(user.lower() for user in value['modders'])


def validate_document(document, approvals):
    validate_approvals(approvals)
    assert set(document) == {'schema', 'mods'} and document['schema'] == 1
    assert isinstance(document['mods'], list) and len(document['mods']) <= 500
    approved = {user.lower() for user in approvals['modders']}
    ids = set()
    fields = {'id', 'name', 'kind', 'author', 'version', 'api', 'description',
              'tags', 'install', 'source', 'download', 'bundled'}
    for mod in document['mods']:
        assert set(mod) == fields, 'Unexpected or missing catalogue field'
        assert isinstance(mod['id'], str) and re.fullmatch(r'[a-z][a-z0-9-]{2,48}', mod['id'])
        assert mod['id'] not in ids, 'Duplicate mod ID'
        ids.add(mod['id'])
        for field, limit in (('name', 90), ('description', 400), ('version', 30)):
            assert isinstance(mod[field], str) and 0 < len(mod[field]) <= limit
            assert all(ord(char) >= 32 for char in mod[field]), 'Control character in metadata'
        assert isinstance(mod['author'], str) and mod['author'].lower() in approved, 'Author is not an approved modder'
        assert mod['kind'] in ('gameplay', 'texture')
        expected_api = 'ds-lua-1' if mod['kind'] == 'gameplay' else 'resource-pack'
        expected_folder = 'mods/' if mod['kind'] == 'gameplay' else 'mods/resource-packs/'
        assert mod['api'] == expected_api and mod['install'] == expected_folder + mod['id']
        assert isinstance(mod['tags'], list) and len(mod['tags']) <= 6
        assert all(isinstance(tag, str) and 0 < len(tag) <= 30 and all(ord(char) >= 32 for char in tag) for tag in mod['tags'])
        assert isinstance(mod['bundled'], bool)
        if mod['bundled']:
            assert mod['id'] == 'sm64-movement' and mod['author'].lower() == 'scopic64'
        for field in ('source', 'download'):
            assert isinstance(mod[field], str) and len(mod[field]) <= 400
            url = urlsplit(mod[field])
            assert url.scheme == 'https' and url.hostname == 'github.com' and not url.username and not url.password
            assert url.port in (None, 443) and not url.fragment
            parts = url.path.strip('/').split('/')
            assert len(parts) >= 2 and parts[0].lower() in (mod['author'].lower(), 'scopic64')
            assert all(part and part not in ('.', '..') for part in parts)


def review_submission(before, after, approvals, author, changed, proposed_approvals=None):
    validate_approvals(approvals)
    maintainer = author.lower() in {user.lower() for user in approvals['maintainers']}
    if maintainer:
        validate_document(after, proposed_approvals or approvals)
        return
    assert author.lower() in {user.lower() for user in approvals['modders']}, 'Submission author is not approved'
    protected = [path for path in changed if path.startswith('website/') or path in
                 ('.github/workflows/project-site.yml', '.github/workflows/mod-catalog-review.yml')]
    assert set(protected) <= {CATALOG}, 'Only maintainers can edit approval rules or site code'
    validate_document(after, approvals)
    old = {mod['id']: mod for mod in before['mods']}
    new = {mod['id']: mod for mod in after['mods']}
    for mod_id in old.keys() | new.keys():
        if old.get(mod_id) == new.get(mod_id):
            continue
        for entry in (old.get(mod_id), new.get(mod_id)):
            assert entry is None or entry['author'].lower() == author.lower(), 'Cannot change another creator\'s entry'


if __name__ == '__main__':
    validate_document(json.loads((ROOT / 'public/catalog.json').read_text()),
                      json.loads((ROOT / 'approved-modders.json').read_text()))
    print('Approved catalogue metadata PASS')
