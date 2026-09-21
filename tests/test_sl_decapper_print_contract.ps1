$ErrorActionPreference = 'Stop'

$sourcePath = Join-Path $PSScriptRoot '..\App\Task\_10_XCommand_Module.c'
$source = Get-Content -LiteralPath $sourcePath -Raw

$start = $source.IndexOf('void CMD_Handle_Print_SL')
$end = $source.IndexOf('void CMD_Handle_Print_CD', $start)

if ($start -lt 0 -or $end -lt 0) {
    throw 'Could not isolate CMD_Handle_Print_SL().'
}

$function = $source.Substring($start, $end - $start)
$requiredOutput = @(
    '[ Common ]',
    'xSL.Time',
    'xSL.isBusy',
    'xSL.isError',
    'xSL.isEnable',
    'xSL.isHomed',
    'xSL.errorCode',
    'sizeof(tsXStateList)',
    '[ xSL.Decapper ]',
    'xSL.Decapper.Cap_is',
    'xSL.Decapper.Body_is',
    'xSL.Decapper.Z_HL_isError',
    'xSL.Decapper.Y_HL_isError',
    'xSL.Decapper.CT_Cap_Grip_isError',
    'xSL.Decapper.CT_Body_Grip_isError',
    '[ xSL.CDecapping_Sensor ]',
    'xSL.CDecapping_Sensor.Z_H_Limit_Sensor',
    'xSL.CDecapping_Sensor.Z_L_Limit_Sensor',
    'xSL.CDecapping_Sensor.Y_H_Limit_Sensor',
    'xSL.CDecapping_Sensor.Y_L_Limit_Sensor',
    'xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Open',
    'xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Close',
    'xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Open',
    'xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Close',
    'xSL.CDecapping_Sensor.CT_Detect_Sensor',
    '[ Motor Run ]',
    'xSL.xZ_Motor_Run.Motor_Run',
    'xSL.xR_Motor_Run.Motor_Run'
)

foreach ($text in $requiredOutput) {
    if (-not $function.Contains($text)) {
        throw "CMD_Handle_Print_SL() is missing output for: $text"
    }
}

Write-Output 'PASS: CMD_Handle_Print_SL prints every Decapper state-list field.'
