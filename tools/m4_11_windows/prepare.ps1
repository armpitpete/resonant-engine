#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$BuildDirectory = "build-m4-11-windows",
    [string]$EvidenceDirectory = "build-m4-11-evidence",
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [string]$DawName = "FL Studio",
    [string]$MidiInput = "Novation Launchkey 25",
    [switch]$SkipTests,
    [switch]$Install,
    [string]$InstallRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallRoot)) {
    $commonProgramFiles = $env:CommonProgramFiles
    if ([string]::IsNullOrWhiteSpace($commonProgramFiles)) {
        if ([string]::IsNullOrWhiteSpace($env:ProgramFiles)) {
            throw "Could not determine Program Files location for the VST3 install root."
        }
        $commonProgramFiles = Join-Path $env:ProgramFiles "Common Files"
    }
    $InstallRoot = Join-Path $commonProgramFiles "VST3"
}

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath exited with code $LASTEXITCODE"
    }
}

function Get-SingleLine {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Command
    )

    $value = & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with code $LASTEXITCODE"
    }
    return (($value | Select-Object -First 1).ToString()).Trim()
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory
} else {
    Join-Path $repoRoot $BuildDirectory
}
$evidencePath = if ([IO.Path]::IsPathRooted($EvidenceDirectory)) {
    $EvidenceDirectory
} else {
    Join-Path $repoRoot $EvidenceDirectory
}

$head = Get-SingleLine { git -C $repoRoot rev-parse HEAD }
$branch = Get-SingleLine { git -C $repoRoot rev-parse --abbrev-ref HEAD }
$porcelain = & git -C $repoRoot status --porcelain
if ($LASTEXITCODE -ne 0) {
    throw "git status failed with code $LASTEXITCODE"
}
if ($porcelain) {
    throw "M4.11 preparation requires a clean Git working tree."
}

New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
New-Item -ItemType Directory -Force -Path $evidencePath | Out-Null

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-DRESONANT_ENGINE_BUILD_VST3=ON",
    "-DRESONANT_ENGINE_BUILD_TESTS=ON",
    "-DRESONANT_ENGINE_BUILD_RENDER=OFF",
    "-DRESONANT_ENGINE_WARNINGS_AS_ERRORS=ON",
    "-DSMTG_CREATE_PLUGIN_LINK=OFF"
)
Invoke-Native -FilePath "cmake" -Arguments $configureArgs

$buildArgs = @(
    "--build", $buildPath,
    "--config", $Configuration,
    "--parallel", "2"
)
Invoke-Native -FilePath "cmake" -Arguments $buildArgs

if (-not $SkipTests) {
    $ctestArgs = @(
        "--test-dir", $buildPath,
        "-C", $Configuration,
        "--output-on-failure"
    )
    Invoke-Native -FilePath "ctest" -Arguments $ctestArgs
}

$bundleMatches = @(
    Get-ChildItem -Path $buildPath -Directory -Recurse -Filter "ResonantEngineBreathPipe.vst3"
)

if ($bundleMatches.Count -eq 0) {
    throw "Could not find ResonantEngineBreathPipe.vst3 under $buildPath"
}

$bundle = $null
if ($bundleMatches.Count -eq 1) {
    $bundle = $bundleMatches[0]
} else {
    $configurationMatches = @(
        $bundleMatches | Where-Object {
            $_.FullName -match [regex]::Escape("\$Configuration\")
        }
    )
    if ($configurationMatches.Count -eq 1) {
        $bundle = $configurationMatches[0]
    } else {
        $paths = ($bundleMatches | ForEach-Object { $_.FullName }) -join [Environment]::NewLine
        throw ("Multiple VST3 bundles were found. Clean the build directory or specify a unique build tree:" +
               [Environment]::NewLine + $paths)
    }
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$manifestPath = Join-Path $evidencePath "plugin-files.sha256"
$bundleRoot = $bundle.FullName.TrimEnd([char[]]"\/")

$manifestLines = @(
    Get-ChildItem -Path $bundleRoot -File -Recurse |
        Sort-Object FullName |
        ForEach-Object {
            $relative = $_.FullName.Substring($bundleRoot.Length).TrimStart([char[]]"\/")
            $relative = $relative.Replace("\", "/")
            $hash = (Get-FileHash -Algorithm SHA256 -Path $_.FullName).Hash.ToLowerInvariant()
            "$hash  $relative"
        }
)

if ($manifestLines.Count -eq 0) {
    throw "The VST3 bundle contains no files."
}

[IO.File]::WriteAllLines($manifestPath, $manifestLines, $utf8NoBom)
$manifestHash = (Get-FileHash -Algorithm SHA256 -Path $manifestPath).Hash.ToLowerInvariant()

$cmakeVersion = Get-SingleLine { cmake --version }
$gitVersion = Get-SingleLine { git --version }

$metadata = [ordered]@{
    schema = "resonant-engine-m4.11-session/v1"
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    commit = $head
    branch = $branch
    working_tree_clean = $true
    os = [Environment]::OSVersion.VersionString
    architecture = $env:PROCESSOR_ARCHITECTURE
    powershell = $PSVersionTable.PSVersion.ToString()
    cmake = $cmakeVersion
    git = $gitVersion
    configuration = $Configuration
    plugin_bundle = $bundle.FullName
    plugin_manifest_file = $manifestPath
    plugin_manifest_sha256 = $manifestHash
    daw = $DawName
    daw_version = ""
    midi_input = $MidiInput
    sample_rate_hz = 48000
    host_buffer_frames = $null
    audio_device_driver = ""
    human_gate_status = "PENDING"
}

$metadataPath = Join-Path $evidencePath "session.json"
[IO.File]::WriteAllText(
    $metadataPath,
    ($metadata | ConvertTo-Json -Depth 5),
    $utf8NoBom
)

$record = @"
# M4.11 Human DAW Acceptance Record

Status: **PENDING HUMAN SESSION**

## Provenance

- Commit: $head
- Branch: $branch
- Working tree clean: yes
- Configuration: $Configuration
- Plugin bundle: $($bundle.FullName)
- Plugin manifest SHA-256: $manifestHash
- Session metadata: $metadataPath
- OS: $([Environment]::OSVersion.VersionString)
- DAW: $DawName
- DAW version: **RECORD DURING SESSION**
- MIDI input: $MidiInput
- Sample rate: **48000 Hz**
- Host buffer: **RECORD DURING SESSION**
- Audio device/driver: **RECORD DURING SESSION**

## Direct human verdicts

- [ ] H01 plugin scan/load PASS
- [ ] H02 accepted Breath Pipe playback PASS
- [ ] H03 velocity response PASS
- [ ] H04 expressive automation PASS
- [ ] H05 overlap/chord/retrigger behaviour PASS
- [ ] H06 project save/reopen state recall PASS
- [ ] H07 offline bounce PASS
- [ ] H08 stop/start/reload recovery PASS
- [ ] H09 M3 identity / no-material-change listening PASS

## H05 monophonic expectation

The current VST3 wrapper is deliberately single-active-note. PASS means bounded,
predictable note takeover/release and no stuck/runaway state. It is not a claim
of four-voice VST3 polyphony.

## Offline bounce evidence

- File:
- Format:
- Sample rate:
- File size:
- SHA-256:
- Audible notes/automation: PASS / FAIL

## Listener statement

Use this exact PASS wording only if it is true:

> The real DAW host does not materially change the accepted M3 Breath Pipe
> identity or expressive behaviour.

Verdict: **PENDING**

## Notes / anomalies

Record any scan warning, audible difference, state mismatch, missing automation,
stuck note, runaway output, bounce defect or recovery issue before changing
source code.
"@

$recordPath = Join-Path $evidencePath "acceptance-record.md"
[IO.File]::WriteAllText($recordPath, $record, $utf8NoBom)

$installedPath = $null
if ($Install) {
    if ([string]::IsNullOrWhiteSpace($InstallRoot)) {
        throw "InstallRoot is empty."
    }

    New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
    $destination = Join-Path $InstallRoot $bundle.Name
    if (Test-Path $destination) {
        throw "Refusing to overwrite existing VST3 bundle: $destination"
    }

    Copy-Item -Path $bundle.FullName -Destination $destination -Recurse
    $installedPath = $destination
}

Write-Host ""
Write-Host "M4.11 preparation complete."
Write-Host "Commit:                 $head"
Write-Host "Plugin bundle:          $($bundle.FullName)"
Write-Host "Manifest SHA-256:       $manifestHash"
Write-Host "Session metadata:       $metadataPath"
Write-Host "Acceptance record:      $recordPath"
if ($installedPath) {
    Write-Host "Installed bundle:       $installedPath"
} else {
    Write-Host "System VST3 directory:  unchanged (use -Install for a fresh copy)"
}
Write-Host ""
Write-Host "Automated preparation is not M4.11 acceptance. Complete H01-H09 in a real DAW."
