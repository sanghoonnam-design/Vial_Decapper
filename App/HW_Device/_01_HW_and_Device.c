/*******************************************************************************
 * _01_HW_and_Device.c
 *
 *  Created on: 2025.06.23
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "_01_HW_and_Device.h"
#include "project.h"

void SystemClock_Config(void);
void MPU_Config(void);

/** ****************************************************************************
 * @brief 하드웨어 & 시스템 장치 초기화
 * @note  1. HW_Init() 부분은 모든 프로젝트에 동일하게 적용
 *        2. Dev_Init() 부분은 프로젝트에 맞게 수정해서 사용
 * *****************************************************************************/
void HWDev_Initialize(void)
{
    HW_Init();

    DEV_Init();
}

void HW_Init(void)
{
    HAL_Init();           /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    MPU_Config();         /* MPU Configuration */
    SystemClock_Config(); /* Configure the system clock */

    /* CPU 내장 시리얼 통신 초기화: USB 1ch, RS485, TS232 */
    Serial_t usb, rs485, ts232;
    usb.baudrate = BAUDRATE_115200; // usb
    usb.parity = PARITY_NONE;       //
    usb.stopbit = UART_STOPBITS_1;  //
    // rs485.baudrate = BAUDRATE_115200; // RS485
    rs485.baudrate = BAUDRATE_38400;  // RS485
    rs485.parity = PARITY_NONE;       //
    rs485.stopbit = UART_STOPBITS_1;  //
    ts232.baudrate = BAUDRATE_9600;   // TS232 TTL level
    ts232.parity = PARITY_NONE;       //
    ts232.stopbit = UART_STOPBITS_1;  //
    IUART_Init(&usb, &rs485, &ts232); //

    ITIMER_Init(); // Timer init.
    IPWM_Init();   // PWM port init.
    IADC_Init();   // ADC init.
    TEMP_Init();   // Temperature init.
    LED_Init();    // Status-LED init.
    IOEXP_Init();  // Digital IO init.
    SW_Init();     // Switch init.
    EEPROM_Init(); // EEPROM init.
    SWRTC_Init();
    Drive_Init(); // Step motor driver init.

    Can_t c0;
    c0.baudrate = BAUDRATE_1M;
    c0.id = 0;
    c0.mask = 0;
    ICAN_Init(&c0); // CAN init.

    RS485_Init();

    /*[]. 디버깅용  */
    Init_TriggerPort(); // init trigger port
}

void DEV_Init(void)
{
    Initialize_RobotDoor();
    Initialize_ServoA6();
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 216;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Activate the Over-Drive mode
     */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_RCC_CLK48_CONFIG(RCC_CLK48SOURCE_PLL);
    __HAL_RCC_SDMMC1_CONFIG(RCC_SDMMC1CLKSOURCE_CLK48);

    { // Backup SRAM
        __HAL_RCC_BKPSRAM_CLK_ENABLE();
        HAL_PWR_EnableBkUpAccess();
    }
}

/* MPU Configuration */
void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    /* Disables the MPU */
    HAL_MPU_Disable();

    /** Initializes and configures the Region and the memory to be protected
     */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x60000000;
    MPU_InitStruct.Size = ARM_MPU_REGION_SIZE_256MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
