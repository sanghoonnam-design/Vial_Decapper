/*******************************************************************************
 * XFilter.c
 *
 *  Created on: 2024.10.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XFilter.h"
#include "string.h"
#include <math.h>

/* 3. Butterworth Filter  : 2차 시스템 설계 */
void ButterworthFilter_Init(tsFilterState *f, float cutoff_frequency, float sampling_frequency)
{
    /** @brief frequency 기준 필터 상수 계산
     * \note omega : normalized 된 주파수: 차단 주파수를 샘플링 주파수에 대해 정규화
     * \note alpha : 댐핑 계수(damping factor) ==> 필터의 폭(Q)
     */
    float omega = 2.0f * 3.141592653589793f * cutoff_frequency / sampling_frequency;
    float alpha = sin(omega) / (2.0f * 0.7071067811865476f); // Assuming Q = 1/sqrt(2)

    // filter coefficients 계산
    f->b[0] = (1 - cos(omega)) / 2;
    f->b[1] = 1 - cos(omega);
    f->b[2] = (1 - cos(omega)) / 2;
    f->a[1] = -2 * cos(omega);
    f->a[2] = 1 - alpha;

    // Normalize
    for (int i = 0; i < 3; i++)
    {
        f->b[i] /= (1 + alpha);
        f->a[i] /= (1 + alpha);
    }

    // 변수 초기화
    for (int i = 0; i < 3; i++)
    {
        f->x[i] = 0.0f;
        f->y[i] = 0.0f;
    }
}

float ButterworthFilter_LowPass(tsFilterState *f, float input)
{ // 2nd order system
    f->x[0] = input;
    f->y[0] = f->b[0] * f->x[0] + f->b[1] * f->x[1] + f->b[2] * f->x[2] - f->a[1] * f->y[1] - f->a[2] * f->y[2];

    // Update states
    f->x[2] = f->x[1];
    f->x[1] = f->x[0];

    f->y[2] = f->y[1];
    f->y[1] = f->y[0];

    return f->y[0];
}

/** @brief 4. Kanlman Filter
 *  \note   4-1. 1차원 상태에 대한 코드
 *              - 반드시 R>0 */
void Kalman_init(tsKalmanFilter *kf, float initial_x, float initial_P, float Q, float R)
{
    kf->x = initial_x; // 초기 상태
    kf->P = initial_P; // 초기 공분산
    kf->Q = Q;         // 프로세스 노이즈
    kf->R = R;         // 측정 노이즈
}

// dt 가 일정 할때 사용
float Kalman_update(tsKalmanFilter *kf, float measurement)
{
    // 1. 예측 단계 : (P)공분산 예측
    kf->P += kf->Q; // 이전 공분산에 프로세스 노이즈를 더함.

    // 2. 업데이트 단계
    kf->K = kf->P / (kf->P + kf->R);        // 칼만 이득 계산, R=측정노이즈 covariance
    kf->x += kf->K * (measurement - kf->x); // 상태 업데이트(예측값 추정)
    kf->P *= (1 - kf->K);                   // 공분산 업데이트

#if 0 // TBD
    // 프로세스 노이즈 동적 조정 (상태가 변할 때 사용)
    if (measurement > 1.0) {  // 임의의 조건에 따라
        kf->Q *= 1.1; // 불확실성이 증가
    } else {
        kf->Q *= 0.9; // 불확실성이 감소
    }
#endif

    return kf->x;
}

// dt 가 일정 하지 않을때 사용
float Kalman_update2(tsKalmanFilter *kf, float measurement, float dt)
{
    // 1. 예측 단계 : (P)공분산 예측
    kf->P += kf->Q * dt; // 이전 공분산에 프로세스 노이즈를 더함.
    // kf->P += kf->Q * dt * dt; // 속도 변화가 클 때 사용

    // 2. 업데이트 단계
    kf->K = kf->P / (kf->P + kf->R);        // 칼만 이득 계산, R=측정노이즈 covariance
    kf->x += kf->K * (measurement - kf->x); // 상태 업데이트(예측값 추정)
    kf->P *= (1 - kf->K);                   // 공분산 업데이트

    return kf->x;
}

#if 0 // ex)
void example() {
    KalmanFilter kf;
    kalman_init(&kf, 0.0, 1.0, 0.1, 0.1);  // 초기화

    float measurements[] = {1.0, 2.0, 3.0, 2.5, 3.5}; // 센서 측정값
    int n = sizeof(measurements) / sizeof(measurements[0]);

    for (int i = 0; i < n; i++) {
        kalman_update(&kf, measurements[i]);
        printf("Filtered value: %f\n", kf.x); // 필터링된 값 출력
    }
}
#endif

/*****************************************************************************
 * 5. Moving Average Filter
 ******************************************************************************/
void MovingAverageFilter_Init(tsMovingAverageFilter *f, int windowSize)
{
    if (windowSize <= 0)
        windowSize = 1;

    if (windowSize > MA_MAX_WINDOW_SIZE)
        windowSize = MA_MAX_WINDOW_SIZE;

    f->windowSize = windowSize;
    f->index = 0;
    f->count = 0;
    f->sum = 0.0f;

    for (int i = 0; i < f->windowSize; i++)
        f->buffer[i] = 0.0f;
}

#if 0
float MovingAverageFilter_Update(tsMovingAverageFilter *f, float input)
{
    // 기존 값 제거
    f->sum -= f->buffer[f->index];

    // 새 값 저장
    f->buffer[f->index] = input;
    f->sum += input;

    // index 증가 (원형 버퍼)
    f->index++;
    if (f->index >= f->windowSize)
        f->index = 0;

    // count 증가 (초기 구간 보호)
    if (f->count < f->windowSize)
        f->count++;

    // 평균 반환
    return f->sum / f->count;
}
#else
/*------------------------------------------------------------------------------
 * MovingAverageFilter_Update
 *
 * 기능:
 *  - 이동평균 필터에 새 입력값(input)을 넣고,
 *    현재 평균값을 반환합니다.
 *
 * 동작:
 *  1) 필터 포인터가 NULL이면 0.0f 반환
 *  2) 윈도우 크기가 0 이하이면 필터링 없이 input 그대로 반환
 *  3) 입력값이 NaN/INF이면
 *     - 기존 유효 데이터가 있으면 현재 평균 반환
 *     - 없으면 0.0f 반환
 *  4) 내부 sum 값이 깨졌다면 필터 전체 초기화
 *  5) 현재 index 위치의 기존 샘플을 sum에서 빼고,
 *     새 input을 buffer에 저장한 뒤 sum에 더함
 *  6) index를 다음 위치로 이동 (ring buffer)
 *  7) count가 windowSize보다 작으면 증가
 *  8) 현재 평균(sum / count) 반환
 *----------------------------------------------------------------------------*/
float MovingAverageFilter_Update(tsMovingAverageFilter *f, float input)
{
    /* 필터 객체가 없으면 안전하게 0 반환 */
    if (f == NULL)
        return 0.0f;

    /* 윈도우 크기가 0 이하이면 필터를 적용하지 않고 원본값 반환 */
    if (f->windowSize <= 0)
        return input;

    /* 입력값이 NaN 또는 INF 이면 기존 평균값 유지 */
    if (!isfinite(input))
        return (f->count > 0) ? (f->sum / f->count) : 0.0f;

    /* 내부 누적합(sum)이 NaN/INF로 깨졌으면 필터 상태 초기화 */
    if (!isfinite(f->sum))
    {
        f->sum   = 0.0f;
        f->count = 0;
        f->index = 0;
        memset(f->buffer, 0, sizeof(f->buffer));
    }

    /* 현재 index 위치의 버퍼값이 비정상이면 0으로 보정 */
    if (!isfinite(f->buffer[f->index]))
        f->buffer[f->index] = 0.0f;

    /* 기존 샘플을 합계에서 제거 */
    f->sum -= f->buffer[f->index];

    /* 새 입력값을 버퍼에 저장 */
    f->buffer[f->index] = input;

    /* 새 입력값을 합계에 반영 */
    f->sum += input;

    /* 다음 버퍼 위치로 이동 */
    f->index++;

    /* ring buffer 처리 */
    if (f->index >= f->windowSize)
        f->index = 0;

    /* 아직 버퍼가 다 차지 않았으면 count 증가 */
    if (f->count < f->windowSize)
        f->count++;

    /* 현재 평균 반환 */
    return (f->count > 0) ? (f->sum / f->count) : input;
}
#endif
