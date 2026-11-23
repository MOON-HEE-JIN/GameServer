
param(
    [string]$TargetPath,
    [string]$ProjectPath,
    [string]$Configuration,
    [string]$Platform,
    [string]$Solution
)

$ErrorActionPreference = "Stop"

$ProjectPath = Split-Path -Path $ProjectPath -Parent
$TargetDir = Split-Path -Path $TargetPath -Parent
$Solution = Split-Path -Path $Solution -Parent

# 1. ÎπåÎìú Í≤∞Í≥ºÎ¨??ïÏù∏
if (!(Test-Path -LiteralPath $TargetPath)) {
    throw "Build output not found: $TargetPath"
}

# 2. Docker ÎπåÎìú ?îÎ†â?∞Î¶¨
$outputDir = Join-Path $Solution "DockerBuild"
$outputDir = Join-Path $outputDir $Platform
$outputDir = Join-Path $outputDir $Configuration

Write-Host "ProjectPath : $ProjectPath"
Write-Host "Configuration : $Configuration"
Write-Host "Platform : $Platform"
Write-Host "Target : $TargetPath"
Write-Host "OutputDir : $outputDir"
Write-Host "Solution : $Solution"

New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

# 3. EXE Î≥µÏÇ¨
$exeName = Split-Path $TargetPath -Leaf

Copy-Item -LiteralPath $TargetPath -Destination $outputDir -Force

# EXE?Ä ?ôÏùº???îÎ†â?∞Î¶¨???àÎäî DLL Î≥µÏÇ¨
$targetDir = Split-Path $TargetPath -Parent

Write-Host "TargetDir : $TargetDir"
#with dll
Get-ChildItem -LiteralPath $TargetDir -File |
    Where-Object { $_.Extension -in ".dll", ".pdb" } |
    Copy-Item -Destination $outputDir -Force

# 4. Dockerfile ?ùÏÑ±
# Deploy release CRT DLLs beside the EXE; no container installer is needed.
if ($Configuration -notmatch 'Debug') {
    $redistRoot = $env:VCToolsRedistDir
    if (!$redistRoot) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (!(Test-Path -LiteralPath $vswhere)) {
            throw 'Visual Studio locator not found. Set VCToolsRedistDir to the MSVC redistributable directory.'
        }
        $vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (!$vsInstall) { throw 'Visual Studio C++ tools installation not found.' }
        $redistVersions = Join-Path $vsInstall 'VC\Redist\MSVC'
        $redistRoot = Get-ChildItem -LiteralPath $redistVersions -Directory |
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
            Sort-Object { [version]$_.Name } -Descending |
            Select-Object -First 1 -ExpandProperty FullName
    }
    if (!$redistRoot) { throw 'MSVC redistributable directory not found.' }
    $crtDirectory = Get-ChildItem -LiteralPath (Join-Path $redistRoot $Platform) -Directory |
        Where-Object { $_.Name -match '^Microsoft\.VC\d+\.CRT$' } |
        Select-Object -First 1
    if (!$crtDirectory) { throw "Release CRT directory not found: $redistRoot ($Platform)" }
    foreach ($requiredDll in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
        if (!(Test-Path -LiteralPath (Join-Path $crtDirectory.FullName $requiredDll))) {
            throw "Required CRT DLL not found: $requiredDll"
        }
    }
    Get-ChildItem -LiteralPath $crtDirectory.FullName -Filter '*.dll' -File |
        Copy-Item -Destination $outputDir -Force
    Write-Host "CRT runtime copied from: $($crtDirectory.FullName)"
}

$dockerfile = @"
FROM mcr.microsoft.com/windows/servercore:ltsc2019

WORKDIR C:/app

COPY . C:/app/

ENTRYPOINT ["C:/app/$exeName"]
"@

$dockerfilePath = Join-Path $outputDir "Dockerfile"

Set-Content -LiteralPath $dockerfilePath -Value $dockerfile -Encoding ASCII

Write-Host "Docker files generated: $outputDir"
