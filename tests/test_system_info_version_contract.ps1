$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$moduleDef = Get-Content -Raw (Join-Path $repoRoot 'App/System/XSystemInfo_ModuleDef.h')
$versionDef = Get-Content -Raw (Join-Path $repoRoot 'App/Version.h')
$systemInfo = Get-Content -Raw (Join-Path $repoRoot 'App/System/XSystemInfo.c')

function Require-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Require-Match $moduleDef '#define\s+SYSTEM_TYPE[^\r\n]*SYS_Vial_Decapper' 'Default system type must be SYS_Vial_Decapper.'
Require-Match $moduleDef '#define\s+MODEL_TYPE[^\r\n]*MODEL_05' 'Default model must be MODEL_05.'
Require-Match $moduleDef '#define\s+SYS_Vial_Decapper[^\r\n]*\(1\)' 'Vial Decapper system identifier is missing.'
Require-Match $moduleDef '#define\s+MODEL_05[^\r\n]*\(5\)' 'MODEL_05 must have numeric value 5.'
Require-Match $moduleDef '#define\s+MODEL_15[^\r\n]*\(15\)' 'MODEL_15 must have numeric value 15.'
Require-Match $moduleDef '#define\s+MODEL_25[^\r\n]*\(25\)' 'MODEL_25 must have numeric value 25.'
Require-Match $moduleDef '#define\s+MODEL_50[^\r\n]*\(50\)' 'MODEL_50 must have numeric value 50.'
Require-Match $moduleDef '#define\s+SYSTEM_TYPE_STR\s+.*"Vial_Decapper"' 'System type string must be Vial_Decapper.'
Require-Match $versionDef '#define\s+FW_VERSION[^\r\n]*\(10000\)' 'Initial firmware version must be 1.0.0.'
Require-Match $systemInfo '"%d\.%d\.%dA%02u"' 'Version formatter must produce an A-prefixed, two-digit model suffix.'

foreach ($model in 5, 15, 25, 50) {
    $formatted = '1.0.0A{0:D2}' -f $model
    $expected = switch ($model) {
        5  { '1.0.0A05' }
        15 { '1.0.0A15' }
        25 { '1.0.0A25' }
        50 { '1.0.0A50' }
    }
    if ($formatted -ne $expected) {
        throw "Unexpected formatted version for model ${model}: $formatted"
    }
}

Write-Host 'System info version contract passed.'
