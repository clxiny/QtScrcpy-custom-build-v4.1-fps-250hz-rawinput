param(
    [string]$SourceDir = ""
)

$ErrorActionPreference = "Stop"

$PatchDir = $PSScriptRoot
$RepoRoot = (Resolve-Path (Join-Path $PatchDir "..")).Path
$PatchFile = Join-Path $PatchDir "0001-add-server-side-video-pause.patch"
$ServerTarget = Join-Path $RepoRoot "QtScrcpy\QtScrcpyCore\src\third_party\scrcpy-server"

if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $SourceDir = Join-Path $RepoRoot ".server-build\scrcpy-v4.1"
}

if (-not (Test-Path (Join-Path $SourceDir ".git"))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $SourceDir) | Out-Null
    git clone --branch v4.1 --depth 1 https://github.com/Genymobile/scrcpy.git $SourceDir
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to clone scrcpy v4.1"
    }
}

git -C $SourceDir apply --check $PatchFile 2>$null
if ($LASTEXITCODE -eq 0) {
    git -C $SourceDir apply $PatchFile
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to apply the video-pause server patch"
    }
} else {
    git -C $SourceDir apply --reverse --check $PatchFile 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "The scrcpy source tree is neither clean nor already patched"
    }
}

$GradleHome = Join-Path $RepoRoot ".server-build\gradle-home"
$env:GRADLE_USER_HOME = $GradleHome
& (Join-Path $SourceDir "gradlew.bat") -p $SourceDir :server:assembleRelease --no-daemon
if ($LASTEXITCODE -ne 0) {
    throw "scrcpy server build failed"
}

$Apk = Get-ChildItem (Join-Path $SourceDir "server\build\outputs\apk\release") -Filter "*.apk" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $Apk) {
    throw "Built server APK was not found"
}

$Backup = "$ServerTarget.official-v4.1"
if ((Test-Path $ServerTarget) -and -not (Test-Path $Backup)) {
    Copy-Item $ServerTarget $Backup
}
Copy-Item $Apk.FullName $ServerTarget -Force

Write-Host "Custom server installed: $ServerTarget"
Write-Host "Original server backup: $Backup"
