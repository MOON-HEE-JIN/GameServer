
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
$dockerfile = @"
FROM mcr.microsoft.com/windows/servercore:ltsc2019

WORKDIR C:/app

COPY . C:/app/

ENTRYPOINT ["C:/app/$exeName"]
"@

$dockerfilePath = Join-Path $outputDir "Dockerfile"

Set-Content -LiteralPath $dockerfilePath -Value $dockerfile -Encoding ASCII

Write-Host "Docker files generated: $outputDir"
