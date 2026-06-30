param(
    [string]$Source = "$PSScriptRoot\..\ExternalAssets\ToImport",
    [string]$Dest = "/Game/Assets/Fab/Auto",
    [switch]$DryRun,
    [switch]$Replace,
    [switch]$NoMaterials,
    [switch]$NoNanite,
    [switch]$NoScale,
    [double]$TargetSize = 100.0,
    [ValidateSet("auto", "box", "sphere", "capsule", "convex", "none")]
    [string]$Collision = "auto",
    [ValidateSet("none", "center", "bottom")]
    [string]$Pivot = "none",
    [string]$UnrealEditor = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$ProjectFile = Join-Path $ProjectRoot "ProjectFT.uproject"
$PythonScript = Join-Path $ProjectRoot "Content\Python\auto_import_assets.py"
$ProjectFileArg = $ProjectFile.Replace("\", "/")
$PythonScriptArg = $PythonScript.Replace("\", "/")
$SourceArg = $Source.Replace("\", "/")
$CollisionArg = $Collision.ToLowerInvariant()
$PivotArg = $Pivot.ToLowerInvariant()

$scriptArgs = @(
    "`"$PythonScriptArg`"",
    "--source", "`"$SourceArg`"",
    "--dest", "`"$Dest`"",
    "--target-size", $TargetSize.ToString([System.Globalization.CultureInfo]::InvariantCulture),
    "--collision", $CollisionArg,
    "--pivot", $PivotArg
)

if ($DryRun) { $scriptArgs += "--dry-run" }
if ($Replace) { $scriptArgs += "--replace" }
if ($NoMaterials) { $scriptArgs += "--no-materials" }
if ($NoNanite) { $scriptArgs += "--no-nanite" }
if ($NoScale) { $scriptArgs += "--no-scale" }

$executePython = $scriptArgs -join " "

& $UnrealEditor $ProjectFileArg "-ExecutePythonScript=$executePython"
