/*******************************************************************************
 * XDebug.c
 *
 *  Created on: 2026.01.02
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#include "XDebug_User.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "_10_XCommand_Core.h"
#include "_04_XDiagnose.h"
#include "XDebug.h"
#include "XParser.h"
#include "XEEPROMParam.h"
// #include "_04_XDiagnose_Def.h"
#include "_04_XDiagnose.h"

/***************************************************************************** */
#ifdef STAGE_DEVELOPMENT
/**/ U08 isConnectedUSB = YES;
#else // 양산 버전
/**/ U08 isConnectedUSB = NO;
#endif
/***************************************************************************** */
//=============================================================================

/** @note USER CODE - START */

__attribute__((unused)) static void CLI_PrintSystemSummary(void)
{
    LOG_MSG_SEND("[F1] Pressed.");

    //    xprintf(ANSI_ST_BOLD "\t[%s(%d),  %s(%d)]" ANSI_ST_RESET " CPU : %s%.2f°C%s",
    //            GetSystemTypeString(), GetSystemType(),
    //            GetModelTypeString(), GetModelType(),
    //            ANSI_TX_LightGreen,
    //            xSystemInfo.CD_Get_CpuTemperature(),
    //            ANSI_TX_ORG);
}

__attribute__((unused)) void CLI_HandleESCKey(void)
{
    xprintf("[Pressed ESC key.]");

    //    if (DB_isDiagnosisEnable)
    //        DB_isDiagnosisEnable = NO;
    //    if (DB_isPrintParsingDataEnabled)
    //        DB_isPrintParsingDataEnabled = NO;
    //    if (DB_isCANRxFrameDisplayEnabled)
    //        DB_isCANRxFrameDisplayEnabled = NO;
    //    if (DB_isCANTxFrameDisplayEnabled)
    //        DB_isCANTxFrameDisplayEnabled = NO;
}

__attribute__((unused)) void CLI_HandleSpecialKeys(char c)
{
    switch (c)
    {
    case 'A': // Up Arrow, command history 관리
    {
        const char *prevCmd = CLI_GetPrevHistory();
        if (prevCmd)
        {
            CLI_ClearCurrentInput();

            strncpy(inputBuffer, prevCmd, __CLI_MAX_CMD_LENGTH - 1);
            inputBuffer[__CLI_MAX_CMD_LENGTH - 1] = '\0';
            inputIndex = strlen(inputBuffer);
        }
        else
        {
            CLI_ClearCurrentInput();
        }
        break;
    }
    case 'B': // Down Arrow, command history 관리
    {
        const char *nextCmd = CLI_GetNextHistory();
        if (nextCmd)
        {
            CLI_ClearCurrentInput();

            strncpy(inputBuffer, nextCmd, __CLI_MAX_CMD_LENGTH - 1);
            inputBuffer[__CLI_MAX_CMD_LENGTH - 1] = '\0';
            inputIndex = strlen(inputBuffer);
        }
        else
        {
            CLI_ClearCurrentInput();
        }
        break;
    }
    case 'C': // -> Right Arrow, // 마지막 명령 재 실행
    {
        //---------------------------------------------------------------------------------
        if (!strcmp(xParsedData_USB.Command, "LED") && xParsedData_USB.ParamCount == 2) // ex)
        {
            /** @note USER CODE - START */

            //            xParsedData_USB.Params[0].value._int = ON;
            //            xParsedData_USB.Params[1].value._int++;
            //
            //            if (xParsedData_USB.Params[1].value._int == LED_COROL_COUNT)
            //                xParsedData_USB.Params[1].value._int = LED_COLOR_RED;

            //            Handle_command(&xParsedData_USB, COMM_USB);

            /** @note USER CODE - END */
        }
        // ... 계속 추가
        //---------------------------------------------------------------------------------
        else
        {
            if (xParsedData_USB.Command[0] == '\0')
            {
                LOG_MSG_SEND(ANSI_TX_LightRed "Oops...No command entered.!~" ANSI_TX_ORG);
            }
            else
            {
                Handle_command(&xParsedData_USB, COMM_USB);
            }
        }
        break;
    }
    case 'D': // <- 화살표, TODO
    {
        //        static int temp_rpm = 0;

        if (xParsedData_USB.Params[1].value._int == '?')
        {
            Handle_command(&xParsedData_USB, COMM_USB);
            return;
        }

        /** @note USER CODE - START */
        /** @note USER CODE - END */

        break;
    }

    /* ============================================================
     *  Function Keys (F1~F12)
     * ============================================================ */
    case KEY_F1:
        xprintf("\r\n[F1] Pressed.");
        break;
    case KEY_F2:
        xprintf("\r\n[F2] Pressed.");
        break;
    case KEY_F3:
        xprintf("\r\n[F2] Pressed.");
        break;
    case KEY_F4:
        xprintf("\r\n[F4] Pressed.");
        break;
    case KEY_F5:
        xprintf("\r\n[F5] Pressed.");
        break;
    case KEY_F6:
        xprintf("\r\n[F6] Pressed.");
        break;
    case KEY_F7:
        xprintf("\r\n[F7] Pressed.");
        break;
    case KEY_F8:
        xprintf("\r\n[F8] Pressed.");
        break;
    case KEY_F9:
        xprintf("\r\n[F9] Pressed.");
        break;
    case KEY_F10:
        xprintf("\r\n[F10] Pressed.");
        break;
    case KEY_F11:
        xprintf("\r\n[F11] Pressed.");
        break;
    case KEY_F12:
        xprintf("\r\n[F12] Pressed.");
        break;

    default: //---------------------------------------------------------------------------------
        LOG_MSG_SEND("Unhandled Special Key: 0x%02X\r\n", c);
        break;
    }
}

/** @brief Ctrl + Pressed 키 입력 처리 */
__attribute__((unused)) void CLI_HandleCtrl_PressedKeys(char c)
{
    switch (c)
    {
    case 'A':
        XTimer_Stop();
        xprintf("\r\n[Ctrl+A] Pressed.");
        XTimer_Start();
        break;
    case 'B':
        XTimer_Stop();
        xprintf("\r\n[Ctrl+B] Pressed.");
        XTimer_Start();
        break;
    case 'S':
        if (!gWait_SaveConfirm)
        {
            gWait_SaveConfirm = true;
            gSaveConfirmTick = xTaskGetTickCount();
            xcprintf_t("Save to EEPROM? (Y/N): ");
        }
        break;
    case 'L':
        XTimer_Stop();
        xcprintf_t("[Ctrl+L] Pressed.");
        // Handle_command_by_string("LOADE", COMM_USB);
        XTimer_Start();
        break;
    default:
        xcprintf("\r\nUnhandled Ctrl+Key: Ctrl+%c (0x%02X)\r\n", c, c);
        break;
    }
}

__attribute__((unused)) void CLI_HandleBacktickKey(void)
{
    //    int i = 1;
    //
    //    clearConsole();
    //
    //    LOG_MSG_SEND("== [%s(%d), %s(%d)] ==",
    //                 GetSystemTypeString(), GetSystemType(),
    //                 GetModelTypeString(), GetModelType());
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Temp]",
    //                     "%.2f, %.2f, %.2f",
    //                     "Temp(°C), Humi(%%), CO2(%%)",
    //                     xCD.Humidity.Temperature_f,
    //                     xCD.Humidity.Humidity_f,
    //                     xCD.CO2.CO2_percent_f);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Fan]",
    //                     "%4d  %4d  %4d  %4d",
    //                     "rpm",
    //                     xCD.Fan.SpeedRpm_f[0],
    //                     xCD.Fan.SpeedRpm_f[1],
    //                     xCD.Fan.SpeedRpm_f[2],
    //                     xCD.Fan.SpeedRpm_f[3]);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Humidity]",
    //                     "%.2f  %4d  %4d",
    //                     "Humi(%), Pump, Valve",
    //                     xCD.Humidity.Humidity_f,
    //                     xCD.Humidity.Pump,
    //                     xCD.Humidity.SolValve_Nozzle);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[CO2]",
    //                     "%.2f  %4d  %4d",
    //                     "CO2(%), SOL_1, SOL_2",
    //                     xCD.CO2.CO2_percent_f,
    //                     xCD.CO2.SolValve[0],
    //                     xCD.CO2.SolValve[1]);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Robot-Door]",
    //                     "%s %s(%d)[%d/%d], %s(%d)[%d/%d]",
    //                     "Pressure, Upper[c/o sensor], Lower[c/o sensor]",
    //                     RobotDoor_GetString_PressureState(),
    //                     RobotDoor_GetString_State(eUPPER), xSL.RobotDoor.State[eUPPER], xCD.RobotDoor.DoorSensor_db[eUPPER][eCLOSE], xCD.RobotDoor.DoorSensor_db[eUPPER][eOPEN],
    //                     RobotDoor_GetString_State(eLOWER), xSL.RobotDoor.State[eLOWER], xCD.RobotDoor.DoorSensor_db[eLOWER][eCLOSE], xCD.RobotDoor.DoorSensor_db[eLOWER][eOPEN]);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Door]",
    //                     "%s(%d)-%d, %s(%d)-%d, %s(%d)-%d",
    //                     "Stocker, Inner-glass, Outer [stateString(state)-raw]",
    //                     StockerDoor_GetString_State(), xSL.StockerDoor.State, xCD.StockerDoor.HighLow,
    //                     UserDoor_GetString_State(eINNER), xSL.UserDoor.State[eINNER], xCD.UserDoor.HighLow[eINNER],
    //                     UserDoor_GetString_State(eOUTER), xSL.UserDoor.State[eOUTER], xCD.UserDoor.HighLow[eOUTER]);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[WaterLevel]",
    //                     "%s(%d), %s(%d, UV)",
    //                     "Tank state1 (raw), Tank state2(raw-UV pos)",
    //                     WaterLevel_GetString_State(0), xCD.WaterLevel.LevelData_db[0],
    //                     WaterLevel_GetString_State(1), xCD.WaterLevel.LevelData_db[1]);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[UV Lamp]",
    //                     "%s, %s [%.2fA/%.2fA]",
    //                     "State, Malfunc/Normal, [Current/Criteria]",
    //                     UVLamp_GetString_State(),
    //                     UVLamp_GetString_MalfuncState(),
    //                     xCD.UVLamp.Sensor_A,
    //                     xPL.UVLamp.DG_SensorReference_A);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[SCARA Detection]",
    //                     "%s(%d)",
    //                     "State (raw)",
    //                     SCARADetectionSensor_GetString_State(),
    //                     xCD.SCARADetectionSensor.HighLow);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[Buzzer]",
    //                     "%s, %s(%d)",
    //                     "state, UserAlarmState (raw)",
    //                     Buzzer_GetString_State(),
    //                     Buzzer_GetString_UserAlarmState(),
    //                     xCD.Buzzer.HighLow_UserAlarm);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[DAC]",
    //                     "%.2f, %.2f, %.2f",
    //                     "DAC#1,2-Temperature, DAC#3-Humidity, DAC#4-CO2 [mA]",
    //                     xCD.Humidity.Temperature_DAC_mA,
    //                     xCD.Humidity.Humidity_DAC_mA,
    //                     xCD.CO2.CO2_percent_2_DAC_mA);
    //
    //    CLI_PrintAligned(COL_VALUE_WIDTH, i++,
    //                     "[CPU Temperature]",
    //                     "%.2f°C",
    //                     "°C",
    //                     xCD.CPU_Temperature);
}

__attribute__((unused)) bool Debug_IsDebuggingFlagTrue(void)
{
    return 0;
    //    return (!DB_isDiagnosisEnable) ||
    //           DB_isPrintParsingDataEnabled;
}

/** @note USER CODE - END */
