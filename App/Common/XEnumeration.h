/*******************************************************************************
 * XEnumeration.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/

#ifndef ENUMERATION_H_
#define ENUMERATION_H_

//[]. 시간에 대한 Enumerator
enum
{
    BEFORE,
    AFTER
};

typedef enum
{
    eASCII = 0,
    eBINARY = 1,
    eHEXA = 2,
} teCharMode;

//[]. 위치에 대한 Enumerator
enum index_or_position
{
    eCLOSE = 0,
    eOPEN
};

enum
{
    eINNER,
    eOUTER
};

enum
{
    eUPPER,
    eLOWER,
    eALL
};

enum
{
    eLEFT,
    eRIGHT
};

//[]. 통신 메시지 타입
typedef enum
{
    MSG_TX,
    MSG_RX
} teComm_MSG_Type;


// //
// // ─────────────────────────────────────────────────────────────
// // ② Flow Sensor 라인 구분 (총 9종류)
// // ─────────────────────────────────────────────────────────────
// //
// // 핵산합성기 Reagent 종류 (전장팀 자료 기반)
// //
// // 순번   Process        Reagent 이름
// // -----------------------------------------------
// // 1      Deblocking     Trichloroacetic Acid
// // 2      Activation     0.25M ETT
// // 3      Coupling       Amidite (dA~rU) ×12
// // 4      Washing        Acetonitrile (공용)
// // 5      Capping A      Acetic Anhydride
// // 6      Capping B      N-methylimidazole
// // 7      Oxidation      THF/pyridine or lutidine/H₂O
// // 8      Sulfurization  (옵션)
// // 9      Column Drain   (Waste/Drain 용도)
// // -----------------------------------------------
// // ⚠️ Amidite (Coupling)는 Flow Sensor 없이 정량 주입 (보통 Spectrometer 또는 압력 제어 사용)

// typedef enum
// {
//     FLOW_SENSOR_DEBLOCKING = 0,     // Trichloroacetic Acid
//     FLOW_SENSOR_ACTIVATION,         // 0.25M ETT
//     FLOW_SENSOR_WASH,               // Acetonitrile
//     FLOW_SENSOR_CAP_A,              // Acetic Anhydride
//     FLOW_SENSOR_CAP_B,              // N-methylimidazole
//     FLOW_SENSOR_OXIDATION,          // THF/pyridine or lutidine/H₂O
//     FLOW_SENSOR_SULFURIZATION,      // (옵션)
//     FLOW_SENSOR_COLUMN_DRAIN,       // Waste drain
//     FLOW_SENSOR_COMMON_GAS,         // 공용 N₂ 유량 체크 (option)

//     FLOW_SENSOR_TOTAL_COUNT
// } teReagentFlowSensor;

// //
// // ─────────────────────────────────────────────────────────────
// // ③ Reagent 종류 구분 (논리적 분류)
// // ─────────────────────────────────────────────────────────────
// // Flow Sensor 여부와 무관하게 전체 시약 타입 분류
// // 시약 포트 테이블 구성, 타입 기반 분주 루틴 등에서 사용
// //
// typedef enum
// {
//     REAGENT_TYPE_DEBLOCKING = 0,
//     REAGENT_TYPE_ACTIVATION,
//     REAGENT_TYPE_COUPLING,      // Amidite dA~rU
//     REAGENT_TYPE_WASH,
//     REAGENT_TYPE_CAPPING_A,
//     REAGENT_TYPE_CAPPING_B,
//     REAGENT_TYPE_OXIDATION,
//     REAGENT_TYPE_SULFURIZATION,
//     REAGENT_TYPE_DRAIN,

//     REAGENT_TYPE_TOTAL_COUNT
// } teReagentType;

// //
// // ─────────────────────────────────────────────────────────────
// // ④ 합성 사이클 단계 정의
// // ─────────────────────────────────────────────────────────────
// // 상태머신, 순차 제어 루틴에서 사용
// //
// typedef enum
// {
//     SYNTHESIS_STEP_IDLE = 0,
//     SYNTHESIS_STEP_DEBLOCKING,
//     SYNTHESIS_STEP_ACTIVATION,
//     SYNTHESIS_STEP_COUPLING,
//     SYNTHESIS_STEP_CAPPING_A,
//     SYNTHESIS_STEP_CAPPING_B,
//     SYNTHESIS_STEP_OXIDATION,
//     SYNTHESIS_STEP_WASHING,
//     SYNTHESIS_STEP_SULFURIZATION,

//     SYNTHESIS_STEP_FINISHED,
//     SYNTHESIS_STEP_ERROR
// } teSynthesisStep;

// //
// // ─────────────────────────────────────────────────────────────
// // ⑤ 센서 종류 구분
// // ─────────────────────────────────────────────────────────────
// // 센서 상태 테이블 구성 시, 다형성 접근을 위한 인덱스
// //
// typedef enum
// {
//     SENSOR_PRESSURE = 0,
//     SENSOR_LEAK,
//     SENSOR_TEMP,
//     SENSOR_HUMI,
//     SENSOR_FLOW,
//     SENSOR_UV,
//     SENSOR_DOOR,

//     SENSOR_TOTAL_COUNT
// } teSensorType;

// //
// // ─────────────────────────────────────────────────────────────
// // ⑥ 가스 라인 구분 (N₂ 공급 라인 분기)
// // ─────────────────────────────────────────────────────────────
// // N₂ 제어 밸브를 라인별로 구분할 때 사용
// //
// typedef enum
// {
//     GAS_LINE_COUPLING = 0,
//     GAS_LINE_SOLVENT,
//     GAS_LINE_PURGE,
//     GAS_LINE_DRAIN,

//     GAS_LINE_TOTAL_COUNT
// } teGasLine;

#endif //@end: #ifndef ENUMERATION_H_
