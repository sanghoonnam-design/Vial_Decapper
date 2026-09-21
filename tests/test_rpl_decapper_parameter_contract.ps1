$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw (Join-Path $repoRoot 'App/Task/_10_XCommand_Module.c')

$start = $source.IndexOf('static void RPL_GetParameter(void)')
$end = $source.IndexOf('static void RPL_PrintHelp(void)', $start)
if ($start -lt 0 -or $end -le $start) {
    throw 'RPL_GetParameter function was not found.'
}
$body = $source.Substring($start, $end - $start)

$requiredFields = @(
    'RunCur', 'SelMaxCur', 'StopCurRate', 'StepResolution',
    'Limit_PosZ', 'Limit_PosR', 'SwNegLimit', 'SwPosLimit',
    'SoftLimitEnable', 'ZDecapAcc', 'ZDecapVel', 'ZCap_UpPos',
    'ZCap_SidePos', 'ZCap_Origin_Position', 'RDecapAcc',
    'RDecapVel', 'RDecapPos'
)

foreach ($field in $requiredFields) {
    if ($body -notmatch [regex]::Escape("xPL.Decapper.$field")) {
        throw "RPL_GetParameter does not print xPL.Decapper.$field."
    }
}

if ($body -match '"xPL\.Header\.Time"[\s\S]*"xPL\.Header\.Time"') {
    throw 'RPL_GetParameter still prints xPL.Header.Time in the Decapper section.'
}

Write-Host 'RPL Decapper parameter contract passed.'
