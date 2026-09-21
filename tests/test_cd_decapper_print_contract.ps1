$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw (Join-Path $repoRoot 'App/Task/_10_XCommand_Module.c')

$start = $source.IndexOf('void CMD_Handle_Print_CD(')
$end = $source.IndexOf('static int RPL_SetParameter(', $start)
if ($start -lt 0 -or $end -le $start) {
    throw 'CMD_Handle_Print_CD function was not found.'
}
$body = $source.Substring($start, $end - $start)

$requiredFields = @(
    'xCD.Decapper.Motor_CurPos', 'xCD.Decapper.Motor_TargetPos',
    'xCD.Decapper.phaseDecapCap', 'xCD.Decapper.LongRunCount',
    'xCD.Decapper.SpeedPercent', 'xCD.Decapper.SystemInfo.isSWLimit',
    'xCD.Decapper.debug', 'xCD.Decapper.chMotor',
    'xCD.CT.Body', 'xCD.CT.Cap'
)

foreach ($field in $requiredFields) {
    if ($body -notmatch [regex]::Escape($field)) {
        throw "CMD_Handle_Print_CD does not print $field."
    }
}

if ($body -match [regex]::Escape('xCD.xCDecap')) {
    throw 'CMD_Handle_Print_CD must not print xCD.xCDecap function pointers.'
}

Write-Host 'CD Decapper print contract passed.'
