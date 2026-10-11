/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include "pid.h"
#include "encoder.h"
#include <string.h>
#include "i2c_soft.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/*速度环*/
#define SIM_KP  2.0f
#define SIM_KI  0.2f
#define SIM_KAW 0.5f
#define ENCODER_SAMPLE_TIME 0.01f
/*位置环*/
#define POS_KP  1.67f
#define POS_KI  0.0f
#define POS_KAW 0.0f
#define POS_OUT_MAX 25.0f


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
PID_t speed_pi;              /* 速度环 PID 控制器实例（唯一） */
float real_speed;            // 编码器实测转速（rpm）

PID_t angle_p;              /* 角度环 PID 控制器实例（唯一） */
float target_angle = 90.0f;      // 目标角度，后期可改为角度/闭环输入
float real_angle;            // 编码器实测角度（度）
extern volatile int flag_pid;         // 由 TIM4 中断置位，表示新的采样点就绪
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Vofa_SendJustFloat(float *ch,uint8_t num)
{
  uint8_t frame[8 * 4 + 4];
  uint8_t idx = 0;
  for(uint8_t i=0;i<num;i++)
  {
    memcpy(frame + idx, (uint8_t *)(ch+i), 4); 
    idx += 4;
  }
  frame[idx++] = 0x00;frame[idx++] = 0x00;
  frame[idx++] = 0x80;frame[idx++] = 0x7F;
  HAL_UART_Transmit(&huart1, frame, idx, HAL_MAX_DELAY);
}

void __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
} 
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  i2c_soft_init();
  DWT_Delay_Init();

  /* USER CODE BEGIN 2 */
  PID_Init(&speed_pi, SIM_KP, SIM_KI, SIM_KAW, -100.0f, 100.0f); /* 初始化转速实例 */
  PID_Init(&angle_p, POS_KP, POS_KI, POS_KAW, -POS_OUT_MAX, POS_OUT_MAX); /* 初始化角度实例 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim4);   /* 启动 TIM4 定时中断(10ms) */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0); /* 初始占空比 0% */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /*占空比统一由 PID 输出驱动*/
    if(flag_pid)
    {
      flag_pid = 0;   /* 复位标志*/

      Encoder_Update();   /*更新编码器计数*/
      real_angle = Encoder_GetAngle();
      real_speed = Encoder_Getspeed();

      float target_speed = PID_Calc(&angle_p, target_angle, real_angle, ENCODER_SAMPLE_TIME);
      float pwm_out = PID_Calc(&speed_pi, target_speed, real_speed, ENCODER_SAMPLE_TIME);
      float duty;
      if (pwm_out >= 0.0f )
      {
          HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
          duty = pwm_out;
      } else {
          HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_SET);
          duty = -pwm_out;
      }
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
      float frame[ 4 ] = {target_angle, real_angle, target_speed, pwm_out};
      Vofa_SendJustFloat(frame, 4);
    }
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
