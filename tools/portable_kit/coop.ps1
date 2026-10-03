[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('host', 'join')]
    [string] $Mode,
    [string] $Address,
    [ValidateRange(1024, 65534)]
    [int] $Port = 51765
)

$ErrorActionPreference = 'Stop'
if ($Mode -eq 'join') {
    if (-not $Address) { $Address = Read-Host 'Host computer IPv4 address' }
    $parsed = $null
    if (-not [Net.IPAddress]::TryParse($Address, [ref]$parsed) -or
        $parsed.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
        throw 'Enter a valid IPv4 address, for example 192.168.1.20.'
    }
    $Address = $parsed.ToString()
}

# Start each session with an explicit configuration, not inherited test knobs.
Get-ChildItem Env:SM64DS_* | ForEach-Object { Remove-Item -LiteralPath $_.PSPath }
$env:SM64DS_COMMS_PORT = "$Port"
$env:SM64DS_COMMS_FANOUT = '1'
$env:SM64DS_NETMODE = 'rollback'
$env:SM64DS_PARTY = '1'
$env:SM64DS_LEVEL = '1'
$env:SM64DS_SAVE_PATH = Join-Path $PSScriptRoot "coop-$Mode.sav"
if ($Mode -eq 'host') {
    $env:SM64DS_COMMS_ROLE = 'parent'
    $env:SM64DS_COMMS_BIND_ANY = '1'
    Write-Host "Hosting a trusted-LAN co-op session on UDP $Port."
} else {
    $env:SM64DS_COMMS_ROLE = 'child'
    $env:SM64DS_COMMS_SLOT = '1'
    $env:SM64DS_COMMS_HOST = "${Address}:$Port"
    Write-Host "Joining $Address on UDP $Port."
}
& (Join-Path $PSScriptRoot 'play.bat')
exit $LASTEXITCODE
