import copy
import json
from pathlib import Path
import unittest
from validate_catalog import APPROVALS, CATALOG, review_submission, validate_document


class CataloguePolicy(unittest.TestCase):
    def setUp(self):
        root = Path(__file__).resolve().parent
        self.before = json.loads((root / 'public/catalog.json').read_text())
        self.approvals = json.loads((root / 'approved-modders.json').read_text())
        self.approvals['modders'].append('Creator')
        self.after = copy.deepcopy(self.before)

    def add_creator(self):
        entry = copy.deepcopy(self.before['mods'][0])
        entry.update(id='creator-mod', author='Creator', bundled=False,
                     install='mods/creator-mod', source='https://github.com/Creator/my-mod/blob/main/main.lua',
                     download='https://github.com/Creator/my-mod/releases/tag/v1')
        self.after['mods'].append(entry)

    def test_published_catalogue(self):
        validate_document(self.before, self.approvals)

    def test_approved_creator_can_submit_own_mod(self):
        self.add_creator()
        review_submission(self.before, self.after, self.approvals, 'Creator', [CATALOG])

    def test_unapproved_creator_cannot_approve_themself(self):
        self.add_creator()
        trusted = copy.deepcopy(self.approvals)
        trusted['modders'].remove('Creator')
        with self.assertRaises(AssertionError):
            review_submission(self.before, self.after, trusted, 'Creator', [CATALOG, APPROVALS], self.approvals)

    def test_approved_creator_cannot_edit_rules_or_site_code(self):
        for path in (APPROVALS, 'website/validate_catalog.py', '.github/workflows/mod-catalog-review.yml'):
            with self.subTest(path=path), self.assertRaises(AssertionError):
                review_submission(self.before, self.after, self.approvals, 'Creator', [path])

    def test_creator_cannot_edit_or_delete_someone_elses_mod(self):
        for remove in (False, True):
            after = copy.deepcopy(self.before)
            if remove:
                after['mods'].clear()
            else:
                after['mods'][0]['description'] = 'Changed by someone else'
            with self.subTest(remove=remove), self.assertRaises(AssertionError):
                review_submission(self.before, after, self.approvals, 'Creator', [CATALOG])

    def test_maintainer_can_approve_a_new_creator(self):
        self.add_creator()
        base = copy.deepcopy(self.approvals)
        base['modders'].remove('Creator')
        review_submission(self.before, self.after, base, 'SCOPIC64', [CATALOG, APPROVALS], self.approvals)

    def test_unsafe_urls_and_folders_are_rejected(self):
        for field, value in (('download', 'javascript:alert(1)'), ('source', 'https://github.com.evil.test/SCOPIC64/mod'),
                             ('source', 'https://other@github.com/SCOPIC64/mod'), ('install', '../outside'),
                             ('install', 'mods/resource-packs/sm64-movement')):
            after = copy.deepcopy(self.before)
            after['mods'][0][field] = value
            with self.subTest(field=field, value=value), self.assertRaises(AssertionError):
                validate_document(after, self.approvals)

    def test_duplicates_and_unapproved_metadata_are_rejected(self):
        self.after['mods'].append(copy.deepcopy(self.after['mods'][0]))
        with self.assertRaises(AssertionError):
            validate_document(self.after, self.approvals)
        self.after['mods'].pop()
        self.after['mods'][0]['author'] = 'Visitor'
        with self.assertRaises(AssertionError):
            validate_document(self.after, self.approvals)


if __name__ == '__main__':
    unittest.main()
