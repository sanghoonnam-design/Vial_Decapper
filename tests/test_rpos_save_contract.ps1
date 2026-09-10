$modulePath = Join-Path $PSScriptRoot "..\App\Task\_10_XCommand_Module.c"
$corePath = Join-Path $PSScriptRoot "..\App\Task\_10_XCommand_Core.c"
$headerPath = Join-Path $PSScriptRoot "..\App\Task\_10_XCommand_Module.h"

$module = Get-Content -Raw $modulePath
$core = Get-Content -Raw $corePath
$header = Get-Content -Raw $headerPath

if ($module -notmatch 'bool\s+gZCapUpPosSavePending') {
    throw 'RPOS capture-pending state is missing.'
}

if ($module -notmatch 'S32\s+gZCapUpPosPendingValue') {
    throw 'RPOS captured-position state is missing.'
}

if ($header -notmatch 'extern\s+bool\s+gZCapUpPosSavePending') {
    throw 'SAVEE cannot access the RPOS capture-pending state.'
}

if ($header -notmatch 'extern\s+S32\s+gZCapUpPosPendingValue') {
    throw 'SAVEE cannot access the RPOS captured position.'
}

if ($module -notmatch 'gZCapUpPosSavePending\s*=\s*true') {
    throw 'RPOS does not mark the ZCap_UpPos value for saving.'
}

if ($module -notmatch 'gZCapUpPosPendingValue\s*=\s*CDecap_GetZPosition\s*\(') {
    throw 'RPOS does not retain its sampled Z position.'
}

if ($core -notmatch 'xPL\.Decapper\.ZCap_UpPos\s*=\s*gZCapUpPosPendingValue') {
    throw 'SAVEE does not copy the captured Z position into ZCap_UpPos.'
}

if ($core -notmatch 'gZCapUpPosSavePending') {
    throw 'SAVEE is not guarded by the RPOS capture-pending state.'
}
