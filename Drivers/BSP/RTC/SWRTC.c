#define __SWRTC_C__
    #include "SWRTC.h"
#undef  __SWRTC_C__

#include "project.h"

static uint8_t days_in_month(uint8_t year, uint8_t month);
static void rtc_increment_day(void);
static void SWRTC_SetFromCompileTime(SW_DateTime_t *dt);

static volatile SW_DateTime_t rtc;

void SWRTC_Init(void)
{
    SWRTC_SetFromCompileTime((SW_DateTime_t *)&rtc);
}

void SWRTC_SetParam(SW_DateTime_t *dt)
{
    __disable_irq();

    rtc = *dt;

    __enable_irq();
}

void SWRTC_GetParam(SW_DateTime_t *dt)
{
    __disable_irq();

    *dt = rtc;

    __enable_irq();
}

static void SWRTC_SetFromCompileTime(SW_DateTime_t *dt)
{
    //// __DATE__ → "Jul 14 2025"
    //// __TIME__ → "10:45:32"

    const char *build_date = __DATE__;
    const char *build_time = __TIME__;

    if (strncmp(build_date, "Jan", 3) == 0)
        dt->month = 1;
    else if (strncmp(build_date, "Feb", 3) == 0)
        dt->month = 2;
    else if (strncmp(build_date, "Mar", 3) == 0)
        dt->month = 3;
    else if (strncmp(build_date, "Apr", 3) == 0)
        dt->month = 4;
    else if (strncmp(build_date, "May", 3) == 0)
        dt->month = 5;
    else if (strncmp(build_date, "Jun", 3) == 0)
        dt->month = 6;
    else if (strncmp(build_date, "Jul", 3) == 0)
        dt->month = 7;
    else if (strncmp(build_date, "Aug", 3) == 0)
        dt->month = 8;
    else if (strncmp(build_date, "Sep", 3) == 0)
        dt->month = 9;
    else if (strncmp(build_date, "Oct", 3) == 0)
        dt->month = 10;
    else if (strncmp(build_date, "Nov", 3) == 0)
        dt->month = 11;
    else if (strncmp(build_date, "Dec", 3) == 0)
        dt->month = 12;
    else
        dt->month = 1;

    dt->day = (uint8_t)atoi(&build_date[4]);
    dt->year = (uint8_t)(atoi(&build_date[7]) % 100); // 2025 → 25

    dt->hour = (uint8_t)atoi(&build_time[0]);
    dt->min = (uint8_t)atoi(&build_time[3]);
    dt->sec = (uint8_t)atoi(&build_time[6]);
    dt->ms = 0;
}

void SWRTC_TickISR(void)
{
    rtc.ms++;

    if(rtc.ms >= 1000)
    {
        rtc.ms = 0;
        rtc.sec++;

        if(rtc.sec >= 60)
        {
            rtc.sec = 0;
            rtc.min++;

            if(rtc.min >= 60)
            {
                rtc.min = 0;
                rtc.hour++;

                if(rtc.hour >= 24)
                {
                    rtc.hour = 0;
                    rtc_increment_day();
                }
            }
        }
    }
}

static uint8_t days_in_month(uint8_t year, uint8_t month)
{
    static const uint8_t days[12] =
    {
        31,28,31,30,31,30,31,31,30,31,30,31
    };

    uint8_t d = days[month-1];

    if(month == 2 && (year % 4) == 0)
        d = 29;

    return d;
}

static void rtc_increment_day(void)
{
    rtc.day++;

    if(rtc.day > days_in_month(rtc.year, rtc.month))
    {
        rtc.day = 1;
        rtc.month++;

        if(rtc.month > 12)
        {
            rtc.month = 1;
            rtc.year++;
        }
    }
}
