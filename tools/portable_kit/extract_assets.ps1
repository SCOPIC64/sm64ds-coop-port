<#
.SYNOPSIS
    Unpack the game data out of your own Super Mario 64 DS cartridge dump.

.DESCRIPTION
    Reads a .nds file you supply and writes the game's internal filesystem next
    to this script, in the layout the port expects:

        extracted\dsd\files\<path>     every file, byte for byte as the card has it
        build\assets\files.tsv         file id -> path index, rebuilt from the dump
        build\assets\handles.tsv       game handle -> file id, read out of overlay 0
        build\assets\romdata.bin       code-side data tables, rebuilt from the dump
                                       (the kit's romdata.recipe.tsv says where each
                                       piece lives; the file is hash-checked before
                                       it is written)

    Nothing is downloaded and nothing is installed. Windows PowerShell 5.1 is
    enough; there is no Python, no ndspy and no other dependency.

    The extraction is a plain copy: a file's bytes on disk are exactly the bytes
    the cartridge holds for it. Compressed files are left compressed -- Super
    Mario 64 DS stores those with a four-byte ASCII "LZ77" tag in front of the
    usual type-0x10 header, and the game's own loader looks for that tag, so
    stripping or decoding anything here would break it.

.PARAMETER Rom
    Path to your .nds dump. Optional and taken first when supplied (the launcher
    passes the file it found this way). If you leave it out, the script looks in
    the drop folder next to it ("PLACE YOUR ROM HERE", or the older
    "PLACE EU ROM HERE"), then falls back to a single .nds sitting next to it.

.PARAMETER Destination
    Where to write extracted\ and build\. Defaults to this script's folder.

.EXAMPLE
    .\extract_assets.ps1
    .\extract_assets.ps1 -Rom D:\dumps\sm64ds.nds
#>
[CmdletBinding()]
param(
    [string] $Rom,
    [string] $Destination
)

$ErrorActionPreference = 'Stop'

function Write-Step($text) { Write-Host $text }
function Write-Warn($text) { Write-Host $text -ForegroundColor Yellow }

# Machine-readable progress for a host that is parsing stdout (the launcher).
# One line, '##PROGRESS <pct> <short phase text>'. Percentages are honest against
# the known phase weights below; the file dump dominates, so it owns the widest
# band. A human running the script directly just sees these as extra lines; the
# Write-Step narration is unchanged. Kept on its own line and flushed so the
# launcher's line-reader gets it promptly.
function Write-Prog([int]$pct, [string]$phase) {
    if ($pct -lt 0) { $pct = 0 } elseif ($pct -gt 100) { $pct = 100 }
    Write-Host ("##PROGRESS {0} {1}" -f $pct, $phase)
}

function Stop-Politely($text) {
    Write-Host ""
    Write-Host $text -ForegroundColor Red
    Write-Host ""
    Write-Host "This kit contains no game data of its own, by design. It needs a dump" -ForegroundColor Red
    Write-Host "of a Super Mario 64 DS cartridge that you own. See README.txt." -ForegroundColor Red
    exit 1
}

if (-not $Destination) {
    $Destination = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
}

# ---------------------------------------------------------------- find the rom
# An explicit -Rom wins (the launcher passes the file it validated). Otherwise
# the named drop folder first -- try both the current bundle folder name and the
# older one -- then next to the script, so a dump placed the old way still works.
if (-not $Rom) {
    # The bundle ships the folder named 'PLACE YOUR ROM HERE'. The earlier kit
    # used 'PLACE EU ROM HERE'; accept both so neither layout breaks. Report the
    # one that actually exists (prefer the current name) in the no-ROM message.
    $dropNames = @('PLACE YOUR ROM HERE', 'PLACE EU ROM HERE')
    $dropDir = $null
    $found = @()
    foreach ($name in $dropNames) {
        $candidate = Join-Path $Destination $name
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            if (-not $dropDir) { $dropDir = $candidate }
            $hits = @(Get-ChildItem -Path $candidate -Filter *.nds -File -ErrorAction SilentlyContinue)
            if ($hits.Count -gt 0) { $found = $hits; break }
        }
    }
    if (-not $dropDir) { $dropDir = Join-Path $Destination $dropNames[0] }
    if ($found.Count -eq 0) {
        $found = @(Get-ChildItem -Path $Destination -Filter *.nds -File -ErrorAction SilentlyContinue)
    }
    if ($found.Count -eq 0) {
        Stop-Politely "No .nds file found in '$dropDir' or next to this script."
    }
    if ($found.Count -gt 1) {
        Write-Host ""
        Write-Host "More than one .nds file is sitting here:" -ForegroundColor Red
        $found | ForEach-Object { Write-Host "    $($_.Name)" }
        Stop-Politely "Leave only the Super Mario 64 DS one, or name it: .\extract_assets.ps1 -Rom <file>"
    }
    $Rom = $found[0].FullName
}
if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) {
    Stop-Politely "No such file: $Rom"
}
$Rom = (Resolve-Path -LiteralPath $Rom).Path

Write-Step "Reading $([IO.Path]::GetFileName($Rom))"
$romBytes = [IO.File]::ReadAllBytes($Rom)

# ------------------------------------------------------- check it is the game
# Cartridge header: 12-byte internal title, 4-byte game code, then the table
# offsets. A wrong file trips one of these long before it can write nonsense.
if ($romBytes.Length -lt 0x4000) {
    Stop-Politely "That file is only $($romBytes.Length) bytes -- too small to be a DS cartridge dump."
}
$title = [Text.Encoding]::ASCII.GetString($romBytes, 0x00, 12).TrimEnd([char]0)
$code  = [Text.Encoding]::ASCII.GetString($romBytes, 0x0C, 4)
if ($title -ne 'S.MARIO64DS' -or $code -notlike 'ASM?') {
    Stop-Politely "That is not a Super Mario 64 DS dump (it says title '$title', game code '$code')."
}

$regions = @{ 'E' = 'North America'; 'P' = 'Europe'; 'J' = 'Japan';
              'K' = 'Korea'; 'C' = 'China'; 'U' = 'Australia' }
$regionLetter = $code.Substring(3, 1)
$region = if ($regions.ContainsKey($regionLetter)) { $regions[$regionLetter] } else { "region '$regionLetter'" }

# EU-only gate. The port and this kit's romdata.recipe.tsv are built from the
# European ROM (game code ASMP). A USA/JP/... dump is genuine Super Mario 64 DS,
# passes every header check above, and would otherwise grind all the way to the
# romdata.bin checksum before failing with a confusing "not the revision this
# build was made from" message. Reject it here, up front, by game code, with an
# explicit reason so the player knows exactly which ROM to use.
if ($code -ne 'ASMP') {
    Stop-Politely ("This is the $region ROM (game code $code). This port is built from the " +
                   "European ROM (ASMP), so its data will not come out of a $region cartridge.")
}

$fntOffset = [BitConverter]::ToUInt32($romBytes, 0x40)
$fntSize   = [BitConverter]::ToUInt32($romBytes, 0x44)
$fatOffset = [BitConverter]::ToUInt32($romBytes, 0x48)
$fatSize   = [BitConverter]::ToUInt32($romBytes, 0x4C)
$ovtOffset = [BitConverter]::ToUInt32($romBytes, 0x50)
$ovtSize   = [BitConverter]::ToUInt32($romBytes, 0x54)
$usedSize  = [BitConverter]::ToUInt32($romBytes, 0x80)

foreach ($span in @(@('file name table', $fntOffset, $fntSize),
                    @('file allocation table', $fatOffset, $fatSize),
                    @('overlay table', $ovtOffset, $ovtSize))) {
    if ($span[1] -eq 0 -or ($span[1] + $span[2]) -gt $romBytes.Length) {
        Stop-Politely ("This dump is truncated: its $($span[0]) runs past the end of the file. " +
                       "Re-dump the cartridge.")
    }
}
if ($usedSize -gt $romBytes.Length) {
    Stop-Politely ("This dump is incomplete: the header says the cartridge holds $usedSize bytes " +
                   "but the file is only $($romBytes.Length). Re-dump the cartridge.")
}
Write-Step "Super Mario 64 DS ($code, $region), $([math]::Round($romBytes.Length / 1MB)) MB"
# Phase weights (honest, against wall-clock on a normal run): the file dump
# dominates (~2..70), then the handle table (70..80), then romdata.bin's
# decompress+assemble+hash (80..99). Header/table parsing is instant, so we open
# at 2 once the dump is confirmed good.
Write-Prog 2 "Reading the file tables"

# ------------------------------------------------------------- the file tables
# FAT: one 8-byte {start, end} pair per file id.
$fatCount = [int]($fatSize / 8)
$fatStart = New-Object 'uint32[]' $fatCount
$fatEnd   = New-Object 'uint32[]' $fatCount
for ($i = 0; $i -lt $fatCount; $i++) {
    $fatStart[$i] = [BitConverter]::ToUInt32($romBytes, $fatOffset + $i * 8)
    $fatEnd[$i]   = [BitConverter]::ToUInt32($romBytes, $fatOffset + $i * 8 + 4)
}

# FNT: a directory table of 8-byte entries, each pointing at a name subtable.
# Walk it from the root so a directory is always named before its children.
# Names with the high bit set are subdirectories and carry a trailing id.
$paths = New-Object 'System.Collections.Generic.SortedDictionary[int,string]'
$queue = New-Object 'System.Collections.Generic.Queue[object]'
$queue.Enqueue(@(0, ''))
$visited = New-Object 'System.Collections.Generic.HashSet[int]'
[void]$visited.Add(0)
while ($queue.Count -gt 0) {
    $item = $queue.Dequeue()
    $dirId = [int]$item[0]
    $base  = [string]$item[1]
    $entry = $fntOffset + $dirId * 8
    if (($entry + 8) -gt $romBytes.Length) {
        Stop-Politely "This dump's file name table is damaged. Re-dump the cartridge."
    }
    $o     = $fntOffset + [BitConverter]::ToUInt32($romBytes, $entry)
    $fileId = [int][BitConverter]::ToUInt16($romBytes, $entry + 4)
    while ($true) {
        if ($o -ge $romBytes.Length) {
            Stop-Politely "This dump's file name table is damaged. Re-dump the cartridge."
        }
        $type = $romBytes[$o]; $o++
        if ($type -eq 0) { break }
        $len = $type -band 0x7F
        $name = [Text.Encoding]::ASCII.GetString($romBytes, $o, $len); $o += $len
        if ($type -band 0x80) {
            $sub = [int]([BitConverter]::ToUInt16($romBytes, $o) -band 0x0FFF); $o += 2
            if ($visited.Add($sub)) {
                $queue.Enqueue(@($sub, $(if ($base) { "$base/$name" } else { $name })))
            }
        } else {
            $paths[$fileId] = $(if ($base) { "$base/$name" } else { $name })
            $fileId++
        }
    }
}
if ($paths.Count -eq 0) {
    Stop-Politely "This dump has no named files in it. Re-dump the cartridge."
}

# --------------------------------------------------------------- write them out
$filesRoot = Join-Path $Destination 'extracted\dsd\files'
Write-Step "Writing $($paths.Count) files to $filesRoot"
$madeDirs = New-Object 'System.Collections.Generic.HashSet[string]'
$written = 0
$bytes = [long]0
$total = $paths.Count
foreach ($pair in $paths.GetEnumerator()) {
    $id = $pair.Key
    $relative = $pair.Value.Replace('/', '\')
    $target = Join-Path $filesRoot $relative
    $dir = [IO.Path]::GetDirectoryName($target)
    if ($madeDirs.Add($dir)) { [void][IO.Directory]::CreateDirectory($dir) }
    $length = [int]($fatEnd[$id] - $fatStart[$id])
    $buffer = New-Object 'byte[]' $length
    if ($length -gt 0) { [Array]::Copy($romBytes, [int]$fatStart[$id], $buffer, 0, $length) }
    [IO.File]::WriteAllBytes($target, $buffer)
    $written++
    $bytes += $length
    if (($written % 500) -eq 0) {
        Write-Step "    $written of $total"
        # File dump spans 2..70. There are a few thousand files, so every 500
        # keeps the bar moving well under 5s apart.
        Write-Prog (2 + [int](68 * $written / $total)) "Unpacking file $written of $total"
    }
}
Write-Step "    $written files, $([math]::Round($bytes / 1MB, 1)) MB"
Write-Prog 70 "Unpacked $written files"

# ------------------------------------------------------------ overlay 0 for the
# handle table. The game asks for assets by handle, not by file id; overlay 0
# carries the handle -> path-string table that translates them. Overlays are
# stored back-to-front LZ compressed ("BLZ"), so it has to be decoded first.
function Expand-Blz([byte[]] $data) {
    if ($data.Length -lt 8) { return $data }
    $footer = [BitConverter]::ToUInt32($data, $data.Length - 8)
    $extraSize = [BitConverter]::ToUInt32($data, $data.Length - 4)
    if ($extraSize -eq 0) { return $data }        # flagged, but stored plain
    $headerLen = [int]($footer -shr 24)
    $compressedLen = [int]($footer -band 0xFFFFFF)
    if ($headerLen -lt 8 -or $headerLen -gt $data.Length -or $compressedLen -gt $data.Length) {
        throw "overlay 0 has an unreadable compression header"
    }
    if ($compressedLen -ge $data.Length) { $compressedLen = $data.Length }

    # Anything before the compressed run is already plain and stays put.
    $passthrough = $data.Length - $compressedLen
    $compLen = $compressedLen - $headerLen
    $out = New-Object 'byte[]' ($data.Length + $extraSize - $passthrough)
    $outLen = $out.Length

    # Both streams run backwards: index -1-n from the end of each.
    $read = 0; $done = 0; $flags = 0; $mask = 1
    while ($done -lt $outLen) {
        if ($mask -eq 1) {
            if ($read -ge $compressedLen) { throw "overlay 0 ended mid-stream" }
            $flags = $data[$passthrough + $compLen - 1 - $read]; $read++
            $mask = 0x80
        } else {
            $mask = $mask -shr 1
        }
        if ($flags -band $mask) {
            $b1 = $data[$passthrough + $compLen - 1 - $read]; $read++
            $b2 = $data[$passthrough + $compLen - 1 - $read]; $read++
            $length = ($b1 -shr 4) + 3
            $disp = ((($b1 -band 0x0F) -shl 8) -bor $b2) + 3
            if ($disp -gt $done) {
                if ($done -lt 2) { throw "overlay 0 back-references data that is not there yet" }
                $disp = 2
            }
            $from = $done - $disp
            for ($i = 0; $i -lt $length; $i++) {
                $out[$outLen - 1 - $done] = $out[$outLen - 1 - $from]
                $from++; $done++
            }
        } else {
            $out[$outLen - 1 - $done] = $data[$passthrough + $compLen - 1 - $read]
            $read++; $done++
        }
    }
    $result = New-Object 'byte[]' ($passthrough + $outLen)
    if ($passthrough -gt 0) { [Array]::Copy($data, 0, $result, 0, $passthrough) }
    [Array]::Copy($out, 0, $result, $passthrough, $outLen)
    return $result
}

# Overlay table entries are 32 bytes: id, ram address, size, bss, two static
# initialiser bounds, file id, then (compressed size | flags << 24).
if ($ovtSize -lt 32) { Stop-Politely "This dump has no ARM9 overlays. Re-dump the cartridge." }
$ov0Ram    = [BitConverter]::ToUInt32($romBytes, $ovtOffset + 4)
$ov0FileId = [int][BitConverter]::ToUInt32($romBytes, $ovtOffset + 24)
$ov0Flags  = [int]([BitConverter]::ToUInt32($romBytes, $ovtOffset + 28) -shr 24)
$ov0Length = [int]($fatEnd[$ov0FileId] - $fatStart[$ov0FileId])
$ov0 = New-Object 'byte[]' $ov0Length
[Array]::Copy($romBytes, [int]$fatStart[$ov0FileId], $ov0, 0, $ov0Length)
# The BLZ decode below runs silently and is not instant, so announce it before
# starting -- this is the step that looked hung in testing.
Write-Prog 71 "Decompressing overlay 0"
if ($ov0Flags -band 1) {
    try { $ov0 = Expand-Blz $ov0 }
    catch { Stop-Politely "Could not read overlay 0 of this dump ($_). Re-dump the cartridge." }
}

# The table's address and length are properties of the game, not of the dump:
# the matched initialiser func_ov000_020aa420 walks exactly this many entries.
$handleTableAddress = 0x020BD4B8
$handleCount = 0x80A
$tableOffset = $handleTableAddress - $ov0Ram
if ($tableOffset -lt 0 -or ($tableOffset + $handleCount * 4) -gt $ov0.Length) {
    Stop-Politely ("This dump is Super Mario 64 DS but not a revision this build knows: " +
                   "its overlay 0 has no asset-handle table where one is expected.")
}

$idByPath = @{}
foreach ($pair in $paths.GetEnumerator()) { $idByPath[$pair.Value] = $pair.Key }

$kinds = @{ '.bca' = 'animation'; '.bmd' = 'model'; '.btp' = 'texture-sequence';
            '.kcl' = 'collision'; '.narc' = 'archive'; '.sdat' = 'sound-archive';
            '.bin' = 'data' }
function Get-Kind($path) {
    $suffix = [IO.Path]::GetExtension($path).ToLowerInvariant()
    if ($kinds.ContainsKey($suffix)) { $kinds[$suffix] } else { 'file' }
}

Write-Step "Reading the asset-handle table out of overlay 0"
Write-Prog 72 "Reading the asset-handle table"
$handleRows = New-Object Text.StringBuilder
[void]$handleRows.Append("handle`thex_handle`tfile_id`thex_file_id`tpath`tkind`tsize`n")
for ($h = 0; $h -lt $handleCount; $h++) {
    # Handle table spans 72..80.
    if (($h % 400) -eq 0 -and $h -gt 0) {
        Write-Prog (72 + [int](8 * $h / $handleCount)) "Cataloguing asset handle $h of $handleCount"
    }
    $pointer = [BitConverter]::ToUInt32($ov0, $tableOffset + $h * 4)
    $stringOffset = [int]($pointer - $ov0Ram)
    if ($stringOffset -lt 0 -or $stringOffset -ge $ov0.Length) {
        Stop-Politely ("This dump is Super Mario 64 DS but not a revision this build knows: " +
                       "asset handle $h points outside overlay 0.")
    }
    $end = $stringOffset
    while ($end -lt $ov0.Length -and $ov0[$end] -ne 0) { $end++ }
    $assetPath = [Text.Encoding]::ASCII.GetString($ov0, $stringOffset, $end - $stringOffset)
    if (-not $idByPath.ContainsKey($assetPath)) {
        Stop-Politely ("This dump is Super Mario 64 DS but not a revision this build knows: " +
                       "asset handle $h names '$assetPath', which its filesystem does not have.")
    }
    $id = $idByPath[$assetPath]
    $size = [int]($fatEnd[$id] - $fatStart[$id])
    [void]$handleRows.Append(("{0}`t0x{0:x4}`t{1}`t0x{1:x4}`t{2}`t{3}`t{4}`n" -f
                              $h, $id, $assetPath, (Get-Kind $assetPath), $size))
}

$fileRows = New-Object Text.StringBuilder
[void]$fileRows.Append("file_id`thex_id`tpath`tkind`tsize`n")
foreach ($pair in $paths.GetEnumerator()) {
    $id = $pair.Key
    $size = [int]($fatEnd[$id] - $fatStart[$id])
    [void]$fileRows.Append(("{0}`t0x{0:x4}`t{1}`t{2}`t{3}`n" -f
                            $id, $pair.Value, (Get-Kind $pair.Value), $size))
}

$assetsDir = Join-Path $Destination 'build\assets'
[void][IO.Directory]::CreateDirectory($assetsDir)
$utf8 = New-Object Text.UTF8Encoding $false
[IO.File]::WriteAllText((Join-Path $assetsDir 'files.tsv'), $fileRows.ToString(), $utf8)
[IO.File]::WriteAllText((Join-Path $assetsDir 'handles.tsv'), $handleRows.ToString(), $utf8)
Write-Step "    $($paths.Count) files and $handleCount handles catalogued"
Write-Prog 80 "Rebuilding romdata.bin"

# ------------------------------------------------------------ romdata.bin
# ROM-CLEAN: the game's executable ships with every code-side data table
# zeroed and fills them at boot from build\assets\romdata.bin. The kit ships
# romdata.recipe.tsv -- offsets and hashes only, no game bytes -- and this
# stage rebuilds romdata.bin from the dump: decompress the arm9 program and
# the overlays the recipe names, copy each range, hash-check the result.
# Ranges that reach past a decompressed image are runtime-zero (bss) and stay
# zero, exactly as the recipe's hashes expect.
$recipePath = Join-Path $Destination 'romdata.recipe.tsv'
if (-not (Test-Path -LiteralPath $recipePath)) {
    $recipePath = Join-Path $Destination 'build\assets\romdata.recipe.tsv'
}
if (-not (Test-Path -LiteralPath $recipePath)) {
    Write-Warn "No romdata.recipe.tsv next to this script -- skipping romdata.bin."
    Write-Warn "The game will refuse to start without it; re-download the kit."
} else {
    $recipeLines = [IO.File]::ReadAllLines($recipePath)
    if ($recipeLines.Count -lt 2 -or $recipeLines[0] -notmatch '^# romdata-recipe v1 ([0-9a-f]{64}) (\d+)$') {
        Stop-Politely "romdata.recipe.tsv is damaged. Re-download the kit."
    }
    $wantSha = $Matches[1]
    $blobTotal = [int]$Matches[2]

    # Which images the recipe needs, and where the overlays sit in the dump.
    $needed = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($line in $recipeLines) {
        if ($line.StartsWith('#')) { continue }
        [void]$needed.Add($line.Split("`t")[2])
    }
    $ovByName = @{}
    for ($i = 0; $i -lt [int]($ovtSize / 32); $i++) {
        $e = $ovtOffset + $i * 32
        $ovId = [BitConverter]::ToUInt32($romBytes, $e)
        $ovByName[('ov{0:d3}' -f $ovId)] = @(
            [int][BitConverter]::ToUInt32($romBytes, $e + 24),          # file id
            [int]([BitConverter]::ToUInt32($romBytes, $e + 28) -shr 24) # flags
        )
    }

    Write-Step "Rebuilding romdata.bin ($($needed.Count) images to decompress -- the slow part, please wait)"
    $images = @{}
    # Image decompression spans 80..97. Announce each image before decompressing
    # it (the decompress itself is silent and can be a second or two), so the bar
    # never sits still through this phase.
    $imgDone = 0
    $imgTotal = $needed.Count
    foreach ($src in $needed) {
        Write-Prog (80 + [int](17 * $imgDone / $imgTotal)) "Decompressing $src ($($imgDone + 1) of $imgTotal)"
        if ($src -eq 'arm9') {
            # Program header: rom offset at 0x20, ram address 0x28, size 0x2C.
            # The payload is stored with the same back-to-front compression as
            # the overlays.
            $a9Off  = [int][BitConverter]::ToUInt32($romBytes, 0x20)
            $a9Size = [int][BitConverter]::ToUInt32($romBytes, 0x2C)
            $a9 = New-Object 'byte[]' $a9Size
            [Array]::Copy($romBytes, $a9Off, $a9, 0, $a9Size)
            try { $images[$src] = Expand-Blz $a9 }
            catch { Stop-Politely "Could not read this dump's program data ($_). Re-dump the cartridge." }
        } else {
            if (-not $ovByName.ContainsKey($src)) {
                Stop-Politely ("This dump is Super Mario 64 DS but not a revision this build knows: " +
                               "it has no overlay '$src'.")
            }
            $fileId = $ovByName[$src][0]
            $length = [int]($fatEnd[$fileId] - $fatStart[$fileId])
            $raw = New-Object 'byte[]' $length
            [Array]::Copy($romBytes, [int]$fatStart[$fileId], $raw, 0, $length)
            if ($ovByName[$src][1] -band 1) {
                try { $raw = Expand-Blz $raw }
                catch { Stop-Politely "Could not read overlay '$src' of this dump ($_). Re-dump the cartridge." }
            }
            $images[$src] = $raw
        }
        Write-Step "    $src ($([math]::Round($images[$src].Length / 1KB)) KB)"
        $imgDone++
    }

    # Assemble. The buffer starts zeroed, so a range past its image's end
    # simply keeps its zeros.
    Write-Prog 98 "Assembling and checking romdata.bin"
    $blob = New-Object 'byte[]' $blobTotal
    foreach ($line in $recipeLines) {
        if ($line.StartsWith('#')) { continue }
        $f = $line.Split("`t")
        $off = [int]$f[0]; $size = [int]$f[1]; $img = $images[$f[2]]; $srcOff = [int]$f[3]
        $have = [Math]::Min($size, [Math]::Max(0, $img.Length - $srcOff))
        if ($have -gt 0) { [Array]::Copy($img, $srcOff, $blob, $off, $have) }
    }

    $sha = [BitConverter]::ToString(
        [Security.Cryptography.SHA256]::Create().ComputeHash($blob)).Replace('-', '').ToLowerInvariant()
    if ($sha -ne $wantSha) {
        Stop-Politely ("The rebuilt romdata.bin does not match its checksum -- this dump is " +
                       "Super Mario 64 DS but not the revision this build was made from ($code).")
    }
    [IO.File]::WriteAllBytes((Join-Path $assetsDir 'romdata.bin'), $blob)
    Write-Step "    romdata.bin verified ($blobTotal bytes, checksum OK)"

    # The game verifies romdata.bin at boot against romdata.manifest, which the
    # kit ships next to this script; put it where the game looks.
    $manifestSrc = Join-Path $Destination 'romdata.manifest'
    if (Test-Path -LiteralPath $manifestSrc) {
        Copy-Item -LiteralPath $manifestSrc (Join-Path $assetsDir 'romdata.manifest') -Force
    } elseif (-not (Test-Path -LiteralPath (Join-Path $assetsDir 'romdata.manifest'))) {
        Write-Warn "No romdata.manifest in the kit -- the game will refuse to start; re-download the kit."
    }
}

Write-Prog 100 "Done"
Write-Host ""
Write-Host "Done. The game data is ready in $Destination" -ForegroundColor Green
exit 0
