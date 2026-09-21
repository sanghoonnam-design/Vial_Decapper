$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repoRoot 'App/Task/_10_XCommand_Module.h')
$source = Get-Content -Raw (Join-Path $repoRoot 'App/Task/_10_XCommand_Module.c')

$indices = @(
    'RPL_SET_DECAP_RUN_CUR_Z', 'RPL_SET_DECAP_RUN_CUR_R',
    'RPL_SET_DECAP_SEL_MAX_CUR_Z', 'RPL_SET_DECAP_SEL_MAX_CUR_R',
    'RPL_SET_DECAP_STOP_CUR_RATE_Z', 'RPL_SET_DECAP_STOP_CUR_RATE_R',
    'RPL_SET_DECAP_STEP_RESOLUTION', 'RPL_SET_DECAP_LIMIT_POS_Z',
    'RPL_SET_DECAP_LIMIT_POS_R', 'RPL_SET_DECAP_SW_NEG_LIMIT_Z',
    'RPL_SET_DECAP_SW_POS_LIMIT_Z', 'RPL_SET_DECAP_SW_NEG_LIMIT_R',
    'RPL_SET_DECAP_SW_POS_LIMIT_R', 'RPL_SET_DECAP_SOFT_LIMIT_ENABLE',
    'RPL_SET_DECAP_Z_ACC', 'RPL_SET_DECAP_Z_VEL',
    'RPL_SET_DECAP_Z_CAP_UP_POS', 'RPL_SET_DECAP_Z_CAP_SIDE_POS',
    'RPL_SET_DECAP_Z_ORIGIN_POS', 'RPL_SET_DECAP_R_ACC',
    'RPL_SET_DECAP_R_VEL', 'RPL_SET_DECAP_R_POS'
)

$setterStart = $source.IndexOf('static int RPL_SetParameter(')
$setterEnd = $source.IndexOf('static void RPL_GetParameter(void)', $setterStart)
$helpStart = $source.IndexOf('static void RPL_PrintHelp(void)')
$helpEnd = $source.IndexOf('void CMD_Handle_Print_PL(', $helpStart)
if ($setterStart -lt 0 -or $setterEnd -le $setterStart -or $helpStart -lt 0 -or $helpEnd -le $helpStart) {
    throw 'Unable to locate RPL setter/help functions.'
}

$setter = $source.Substring($setterStart, $setterEnd - $setterStart)
$help = $source.Substring($helpStart, $helpEnd - $helpStart)

foreach ($index in $indices) {
    if ($header -notmatch [regex]::Escape($index)) {
        throw "Missing Decapper parameter index: $index"
    }
    if ($setter -notmatch ('case\s+' + [regex]::Escape($index) + '\s*:')) {
        throw "Missing setter case for: $index"
    }
    if ($help -notmatch [regex]::Escape($index)) {
        throw "Missing help entry for: $index"
    }
}

if ($help -notmatch '\[ Decapper Settable Parameter \]') {
    throw 'Missing Decapper help section heading.'
}
if ($help -notmatch 'PL %d,20000') {
    throw 'Missing Decapper PL command example.'
}

Write-Host 'RPL Decapper help/setter contract passed.'
