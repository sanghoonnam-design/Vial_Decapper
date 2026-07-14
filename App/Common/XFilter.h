/*******************************************************************************
 * XFilter.h
 *
 * Created on: 2024.10.20
 * Author    : RND. Kang PilSoon.
 *
 *  - Butterworth 필터와, 칼만 필터 관련 파일을 제공함.
 *
 ******************************************************************************/
#ifndef XFILTER_H_
#define XFILTER_H_
#include <math.h>

/*****************************************************************************
 * 1. saturation function 관련
 ******************************************************************************/
#define Xsat_sigmoidlike(_x_, _delta_) (_x_ / (fabsf(_x_) + _delta_))

/*****************************************************************************
 * 2. 1차 저역통과 Filter 관련
 ******************************************************************************/
/* 1차 저역통과 필터로서, 필터링된 신호를 시간에 따라 부드럽게 변화시키는 데 사용 */
// [INPUT]
//  _input_    : 현재 입력 값. 필터를 통해서 처리될 원본 값.
//  _preOutput_: 이전 출력 값. LPF의 이전 상태를 나타내며, 현재 필터링된 출력을 계산하는 데 사용됨.
//  _f_        : 필터의 컷오프 주파수(Hz). 값이 클수록 빠르게 반응하고, 값이 작을수록 더 부드러운 필터링이 됨.
//  _dt_       : 샘플링 시간 간격으로, 타이머 주기나 루프 주기 등을 통해 얻어진 시간 값임.
// [설명]
//             :  2.0f * 3.141592f * _f_ * _dt_: 필터의 반응 정도를 나타내는 상수를 계산.
//                여기서 2πf는 필터의 컷오프 주파수에 기반한 이득(Gain)
#define calcLPF(_input_, _preOutput_, _f_, _dt_) (_preOutput_ + 2.0f * 3.141592f * _f_ * _dt_ * (_input_ - _preOutput_))

/*****************************************************************************
 * 3. ButterWorth Low-Pass Filter 관련 : 2dn 시스템으로 설계, Canonical Form II
 ******************************************************************************/
typedef struct
{
    float   a[3];
    float b[3];
    float x[3]; // Raw data
    float y[3]; // Filtered data
} tsFilterState;

void ButterworthFilter_Init(tsFilterState *filter, float cutoff_frequency, float sampling_frequency);
float ButterworthFilter_LowPass(tsFilterState *filter, float input);

/*****************************************************************************
 * 4. 칼만 Filter 관련
 ******************************************************************************/
/* 4-1. 1차원 상태처리 로직 */
typedef struct
{
    float x; // 현재 상태 (예: 온도, 습도, CO2)
    float P; // 공분산(시스템의 불확실 정도)
    float Q; // 프로세스 노이즈 공분산
    float R; // 측정 노이즈 공분산
    float K; // 칼만 이득
} tsKalmanFilter;

void kalman_init(tsKalmanFilter *kf, float initial_x, float initial_P, float Q, float R);
void kalman_update(tsKalmanFilter *kf, float measurement);

/* 2-2. 2차원 상태 처리 로직 */
// ⚠️TODO

#endif /* XFILTER_H_ */