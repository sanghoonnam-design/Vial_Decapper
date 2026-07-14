/*******************************************************************************
 * XFilter.c
 *
 *  Created on: 2024.10.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XFilter.h"
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
 * */
void kalman_init(tsKalmanFilter *kf, float initial_x, float initial_P, float Q, float R)
{
    kf->x = initial_x; // 초기 상태
    kf->P = initial_P; // 초기 공분산
    kf->Q = Q;         // 프로세스 노이즈
    kf->R = R;         // 측정 노이즈
}

void kalman_update(tsKalmanFilter *kf, float measurement)
{
    // 1. 예측 단계 : (P)공분산 예측
    kf->P += kf->Q; // 이전 공분산에 프로세스 노이즈를 더함.

    // 2. 업데이트 단계
    kf->K = kf->P / (kf->P + kf->R);        // 칼만 이득 계산, R=측정노이즈 covariance
    kf->x += kf->K * (measurement - kf->x); // 상태 업데이트(예측값 추정)
    kf->P *= (1 - kf->K);                   // 공분산 업데이트

#if 0
    // 프로세스 노이즈 동적 조정 (상태가 변할 때 사용)
    if (measurement > 1.0) {  // 임의의 조건에 따라
        kf->Q *= 1.1; // 불확실성이 증가
    } else {
        kf->Q *= 0.9; // 불확실성이 감소
    }
#endif
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