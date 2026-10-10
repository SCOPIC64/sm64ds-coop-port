# Project website and approved mod catalogue

The static site lives in `website/public`. It has Windows release downloads,
a searchable gameplay/texture catalogue, installation guides and honest platform
status. It uses no account service, tracking, remote fonts or public upload form.

The home page uses a dark project-site layout. The mod directory uses compact
blue list rows and category navigation inspired by the CoopDX community site.
`mods.html`, `guides.html` and `lobbies.html` are separate static pages. Category
counts come from the real catalogue. All artwork is text/CSS, without copied
game or CoopDX assets.

## Lobby hosting

Pages serves static files and cannot run the Python room service or UDP relay.
The existing tools are under `port/tools/lobby` and `port/tools/relay`; they need
a separate host. The new menu currently uses direct connections. The lobby page
explains what works today and does not publish private room codes or present a
fake public directory. Public listing, native menu integration and deployment
must be completed before a site-to-game lobby flow can be enabled.

## Publish on GitHub Pages

Open repository **Settings → Pages → Build and deployment → Source → GitHub
Actions**. Then rerun the **Project website** workflow on `codex/project-website`.
The workflow validates the catalogue and uploads the static site. Deployment
stays skipped while Pages is disabled; no password or token is stored in the site.
After merging, pushes to `coop-port` also publish it. If the `github-pages`
environment restricts deployment branches, include the publishing branch.

The expected Pages address is `https://scopic64.github.io/sm64ds-coop-port/`.
That address becomes live only after GitHub reports a successful deployment.

For a local preview: `python -m http.server 8765 --bind 127.0.0.1 --directory website/public`.
Validate with `python website/validate_catalog.py`, then
`python -m unittest discover -s website -p 'test_*.py'`.

## Modder approval and review

Only repository maintainers can change `website/approved-modders.json`. Add a
creator's GitHub username to `modders` after approving them. Approval grants
submission eligibility; it does not grant repository write or automatic publishing.

Approved creators host their mod's source and release on GitHub, fork this
repository, add an entry to `website/public/catalog.json`, then open a pull
request against the publishing branch. Copy an existing entry and use:

- A unique lowercase ID, a name, version, description and up to six short tags.
- `kind: "gameplay"`, `api: "ds-lua-1"`, and `install: "mods/<id>"` for Lua mods.
- `kind: "texture"`, `api: "resource-pack"`, and
  `install: "mods/resource-packs/<id>"` for texture packs.
- Your GitHub username as `author` and HTTPS GitHub source/download links in
  your account or the project owner's account. Set `bundled: false`.

The trusted-base review workflow reads approval rules from the PR's base commit,
so a submission cannot approve itself by editing its own allowlist. Approved
modders can add or edit their own entries; changes to other creators' entries or
approval rules require a maintainer. Review does not execute submitted Lua,
download submitted archives or run code from the pull request. The maintainer
reviews compatibility, artwork ownership and download contents before merging.

Catalogue changes become visible only after a maintainer merges them into the
publishing branch and the Pages build passes. GitHub repository access remains
the authority for merging; ordinary visitors have no upload endpoint.

Gameplay scripts use the DS Lua API, not CoopDX's structures. The current Windows
preview runs gameplay hooks only in solo adventure; resource packs load separately.
Do not submit original cartridge data, songs or unedited texture captures.
