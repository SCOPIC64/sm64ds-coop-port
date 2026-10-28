# Character Select preview49 — unpublished release notes

Status: prepared notes only. No GitHub Release or downloadable asset is published by this document.

This incremental Windows x86 preview adds a mod-owned character selection menu and a separate optional [CS] Waluigi add-on. It is not a completed Character Select or DS Versus release.

## Included in the tested preview

- Character Select uses a character grid and an independent animated native 3D preview.
- D-pad Down or `/cs` opens the menu; analog-stick Down does not open it.
- D-pad, stick or arrows navigate. A/Enter confirms; B/Escape cancels. LB/RB changes pages; X/Tab changes sorting.
- Highlighting Waluigi changes the preview only. His gameplay model, voice, HUD portrait and minimap portrait activate after confirmation.
- The separate Waluigi add-on requires Character Select. Without it, the add-on reports `[CS]Waluigi requires character select` and stays inactive.
- Native DS File Select music plays while the selection menu is open and releases when it closes.
- Single-player mod selection, loaded-mod command help, lobby-asset handling and menu/F11 scaling repairs are included.

## Validation and limits

The exact preview executable passed the original three build guards and focused native menu/model/input, avatar/voice/minimap restoration and same-window Stage Disconnect checks. All three installed executable paths passed startup checks. The friend archive passed ZIP CRC and installed-file hash verification; personal save and preference preservation was checked.

These checks do not establish physical controller/cursor/speaker behavior, zero stutter, multiplayer custom-avatar replication, every cutscene, or a finished multiplayer mode.

| Artifact | SHA-256 |
| --- | --- |
| Tested executable | `3fe332c07e93c2c74df14073b2b94afd8cef7c53f9f0ea6f5258cdbeaf95decf` |
| Tested friend ZIP | `22db4af3483476c783eb36bf262dd5e2cf762e1a4d4a9ee7f14d3ca828189aa7` |

These hashes identify the tested local artifacts. They are not links to published downloads.

## Still in development

Casino Luigi, the Warpspeed Power Flower with land/swim speed changes and purple trails, multiplayer custom-character appearance/voice replication, additional movesets, persistent built-in player selection, richer chat and expanded settings are not part of this preview's completed scope. Later local work must not be inferred to be present in these artifacts.

The full DS Versus shared-star/PvP/scoring/seven-player release validation and playable native Linux/SteamOS/macOS packages remain unfinished.

## Credits

- Original SM64DS decompilation and port foundation: [tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp) and its credited contributors.
- Waluigi model: CrossReport, [Waluigi for Super Mario 64 DS](https://skfb.ly/oXX6P), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Adaptations include rigging, conversion, proportions and native animation integration. Attribution does not imply endorsement.
- Waluigi icon art: [g-norm-us](https://www.deviantart.com/g-norm-us/gallery), from the supplied sprite sheet. No additional redistribution license is asserted here.
- Wah-Tacular Waluigi Voice: Greenstranger500 and Toxicking1129; performance by Toxicking1129; original resource thanks to SMS Alfredo.
- Original game, characters and native assets: Nintendo. Users supply their own supported ROM dump.

## Publication requirements

Recover and verify the exact corresponding source snapshot before tagging this preview. Audit the distributable contents and asset attribution, attach the tested package and checksums, and publish as a prerelease with these limitations intact. Do not point an older source tag at newer binaries or describe unfinished features as released.
