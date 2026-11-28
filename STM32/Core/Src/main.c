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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ICM_20948.h"
#include "imu_orientation.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
ICM20948_t icm;
ICM20948_e status;

/* A orientacao inicial e mantida pelo modulo imu_orientation.c. */
uint8_t imu_initialized = 0;
volatile uint32_t imu_read_errors = 0;
float imu_calib_acc_norm = 0.0f;
uint32_t imu_last_read = 0;
uint32_t imu_read_counter = 0;

#define GPS_DMA_SIZE 512
#define GPS_NMEA_SIZE 128

// Buffer escrito automaticamente pelo DMA
uint8_t gps_dma_buffer[GPS_DMA_SIZE];

// Posição de leitura do software
uint16_t gps_dma_read_pos = 0;

// Buffer para montar uma sentença NMEA
uint8_t gps_nmea_buffer[GPS_NMEA_SIZE];
uint16_t gps_nmea_index = 0;

// Indica se estamos recebendo uma sentença
uint8_t gps_nmea_active = 0;

// Estatísticas
uint32_t gps_rx_bytes = 0;
uint32_t gps_nmea_received = 0;
uint32_t gps_nmea_discarded = 0;

#define TX_QUEUE_SIZE 8
#define TX_MSG_SIZE   200

typedef struct
{
    uint8_t data[TX_MSG_SIZE];
    uint16_t length;
} TX_Message_t;

static TX_Message_t tx_queue[TX_QUEUE_SIZE];

static uint8_t tx_head = 0;
static uint8_t tx_tail = 0;
static uint8_t tx_count = 0;

static uint8_t tx_active = 0;
static volatile uint8_t tx_done = 0;

volatile uint32_t tx_sent = 0;
volatile uint32_t tx_dropped = 0;
volatile uint32_t tx_start_errors = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t TX_Enqueue(const uint8_t *data, uint16_t length)
{
    if (data == NULL ||
        length == 0 ||
        length > TX_MSG_SIZE)
    {
        tx_dropped++;
        return 0;
    }

    if (tx_count >= TX_QUEUE_SIZE)
    {
        tx_dropped++;
        return 0;
    }

    TX_Message_t *msg = &tx_queue[tx_head];

    memcpy(msg->data, data, length);
    msg->length = length;

    tx_head = (tx_head + 1U) % TX_QUEUE_SIZE;
    tx_count++;

    return 1;
}

void TX_Service(void)
{
    // Finaliza a mensagem anterior
    if (tx_done)
    {
        tx_done = 0;

        if (tx_active)
        {
            tx_active = 0;

            tx_tail = (tx_tail + 1U) % TX_QUEUE_SIZE;
            tx_count--;

            tx_sent++;
        }
    }

    // Se DMA ocupado ou fila vazia, retorna
    if (tx_active || tx_count == 0)
    {
        return;
    }

    TX_Message_t *msg = &tx_queue[tx_tail];

    HAL_StatusTypeDef result = HAL_UART_Transmit_DMA(
        &huart2,
        msg->data,
        msg->length
    );

    if (result == HAL_OK)
    {
        tx_active = 1;
    }
    else if (result == HAL_ERROR)
    {
        tx_start_errors++;
    }
}

void GPS_ProcessByte(uint8_t byte)
{
    gps_rx_bytes++;

    // Início de uma nova sentença NMEA
    if (byte == '$')
    {
        gps_nmea_index = 0;
        gps_nmea_active = 1;
    }

    // Ignora bytes fora de uma sentença
    if (!gps_nmea_active)
    {
        return;
    }

    // Proteção contra estouro do buffer
    if (gps_nmea_index >= GPS_NMEA_SIZE)
    {
        gps_nmea_index = 0;
        gps_nmea_active = 0;
        gps_nmea_discarded++;
        return;
    }

    // Armazena o byte recebido
    gps_nmea_buffer[gps_nmea_index++] = byte;

    // Sentença completa
    if (byte == '\n')
    {
        if (gps_nmea_index >= 7)
        {
            if (
                memcmp(gps_nmea_buffer, "$GNGGA,", 7) == 0 ||
                memcmp(gps_nmea_buffer, "$GNRMC,", 7) == 0
            )
            {
            	if (TX_Enqueue(gps_nmea_buffer, gps_nmea_index))
            	{
            	    gps_nmea_received++;
            	}
            	else
            	{
            	    gps_nmea_discarded++;
            	}
            }
        }

        gps_nmea_index = 0;
        gps_nmea_active = 0;
    }
}

void GPS_ProcessDMA(void)
{
    if (huart1.hdmarx == NULL ||
        huart1.RxState != HAL_UART_STATE_BUSY_RX)
    {
        return;
    }

    uint16_t write_pos =
        GPS_DMA_SIZE -
        __HAL_DMA_GET_COUNTER(huart1.hdmarx);

    if (write_pos >= GPS_DMA_SIZE)
    {
        write_pos = 0;
    }

    while (gps_dma_read_pos != write_pos)
    {
        uint8_t byte = gps_dma_buffer[gps_dma_read_pos];

        gps_dma_read_pos++;

        if (gps_dma_read_pos >= GPS_DMA_SIZE)
        {
            gps_dma_read_pos = 0;
        }

        GPS_ProcessByte(byte);
    }
}
/* Filtro a 100 Hz; transmissao compacta de atitude a 25 Hz. */
void IMU_ReadAndPrint(float dt)
{
    if (!imu_initialized) return;

    if (ICM20948_Read(&icm) != ICM20948_OK)
    {
        imu_read_errors++;
        return;
    }
    imu_read_counter++;

    /* O acelerometro entra BRUTO (g): nao retirar offsets de montagem.
       O giroscopio entra com o bias de repouso descontado (graus/s). */
    IMU_Attitude atitude;
    if (!IMU_OrientationUpdate(
            icm.accel.x, icm.accel.y, icm.accel.z,
            icm.gyro.x - icm.gyro_offset.x,
            icm.gyro.y - icm.gyro_offset.y,
            icm.gyro.z - icm.gyro_offset.z,
            dt, &atitude))
    {
        return;
    }

    static uint32_t ultimo_envio = 0;
    uint32_t agora = HAL_GetTick();
    if ((uint32_t)(agora - ultimo_envio) >= 40U)
    {
        ultimo_envio = agora;
        char msg[96];
        int len = snprintf(msg, sizeof(msg),
                           "$ATT,%.2f,%.2f,%.2f\r\n",
                           (double)atitude.roll_deg,
                           (double)atitude.pitch_deg,
                           (double)atitude.yaw_deg);
        if (len > 0 && len < (int)sizeof(msg))
        {
            TX_Enqueue((const uint8_t *)msg, (uint16_t)len);
        }
    }
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1000);

  // Configuracao da ICM-20948
  icm.i2c = &hi2c1;
  icm.address = ADDRESS_I2C_AD0_FROM_GND;

  // Inicializacao
  status = ICM20948_Init(&icm);

  if (status == ICM20948_OK)
  {
      /* Confere a escala REAL do CI antes de confiar nos valores em g e dps. */
      uint8_t accel_config_reg = 0;
      uint8_t gyro_config_reg = 0;
      ICM20948_e cfg_acc_status = ICM20948_SelectBank(&icm, BANK_2);
      ICM20948_e cfg_gyro_status = ICM20948_FAIL;
      if (cfg_acc_status == ICM20948_OK)
      {
          cfg_acc_status = ICM20948_Reg_Read(&icm, B2_ACCEL_CONFIG,
                                            &accel_config_reg, 1);
          cfg_gyro_status = ICM20948_Reg_Read(&icm, B2_GYRO_CONFIG,
                                             &gyro_config_reg, 1);
      }
      ICM20948_SelectBank(&icm, BANK_0);

      char cfg_msg[100];
      int cfg_len = snprintf(cfg_msg, sizeof(cfg_msg),
                             "ICM CFG ACC:0x%02X GYRO:0x%02X OK:%d\r\n",
                             accel_config_reg, gyro_config_reg,
                             (cfg_acc_status == ICM20948_OK &&
                              cfg_gyro_status == ICM20948_OK) ? 1 : 0);
      if (cfg_len > 0 && cfg_len < (int)sizeof(cfg_msg))
          HAL_UART_Transmit(&huart2, (uint8_t *)cfg_msg,
                            (uint16_t)cfg_len, 100);

      const char msg[] = "ICM20948 inicializado\r\n";

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)msg,
          sizeof(msg) - 1,
          100
      );

      // Calibra antes de iniciar o DMA do GPS
      status = ICM20948_Calibrate(&icm, 1000, 5);

      if (status == ICM20948_OK)
      {
          /* As medias do accelerometro foram armazenadas em acel_offset.
             O Z recebeu -1 g na biblioteca, portanto restauramos +1 g. */
          float ax0 = icm.acel_offset.x;
          float ay0 = icm.acel_offset.y;
          float az0 = icm.acel_offset.z + 1.0f;
          imu_calib_acc_norm = sqrtf(ax0*ax0 + ay0*ay0 + az0*az0);

          imu_initialized = IMU_OrientationBegin(ax0, ay0, az0) ? 1U : 0U;

          char calib_msg[120];
          int calib_len = snprintf(calib_msg, sizeof(calib_msg),
                                   "ICM calib: |ACC|=%.3f g, fusao=%s\r\n",
                                   (double)imu_calib_acc_norm,
                                   imu_initialized ? "OK" : "ERRO_ESCALA");
          if (calib_len > 0 && calib_len < (int)sizeof(calib_msg))
          {
              HAL_UART_Transmit(&huart2, (uint8_t *)calib_msg,
                                (uint16_t)calib_len, 100);
          }
          /* Se |ACC| for ~7.6 g em repouso, a fusao NAO inicia.
             Verifique ACCEL_CONFIG e o divisor no driver ICM_20948.c. */
      }
      else
      {
          char msg[80];

          int len = snprintf(
              msg,
              sizeof(msg),
              "ERRO CALIBRACAO IMU: %d\r\n",
              (int)status
          );

          if (len > 0 && len < (int)sizeof(msg))
          {
              HAL_UART_Transmit(
                  &huart2,
                  (uint8_t *)msg,
                  (uint16_t)len,
                  100
              );
          }
      }
  }
  else
  {
      char msg[80];

      int len = snprintf(
          msg,
          sizeof(msg),
          "ERRO INICIALIZACAO IMU: %d\r\n",
          (int)status
      );

      if (len > 0 && len < (int)sizeof(msg))
      {
          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)msg,
              (uint16_t)len,
              100
          );
      }
  }



  char msg[100];

  HAL_StatusTypeDef gps_dma_status;

  if (huart1.hdmarx == NULL)
  {
      snprintf(
          msg,
          sizeof(msg),
          "ERRO: DMA USART1 NAO VINCULADO\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)msg,
          strlen(msg),
          100
      );
  }
  else
  {
	  const char inicio[] = "\r\nFIRMWARE DMA V2\r\n";

	  HAL_UART_Transmit(
	      &huart2,
	      (uint8_t *)inicio,
	      sizeof(inicio) - 1,
	      100
	  );
      gps_dma_status = HAL_UART_Receive_DMA(
          &huart1,
          gps_dma_buffer,
          GPS_DMA_SIZE
      );

      snprintf(
          msg,
          sizeof(msg),
          "GPS DMA STATUS: %d\r\n",
          (int)gps_dma_status
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)msg,
          strlen(msg),
          100
      );
  }

  imu_last_read = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	    // Processa bytes recebidos do GPS
	    GPS_ProcessDMA();

	    // Avanca as transmissões da USART2
	    TX_Service();

	    uint32_t now = HAL_GetTick();

    // Leitura da IMU + fusao a cada ~10 ms (100 Hz)
    if ((uint32_t)(now - imu_last_read) >= 10U)
    {
        float dt = (float)((uint32_t)(now - imu_last_read)) * 0.001f;
        imu_last_read = now;
        IMU_ReadAndPrint(dt);
    }

	    // Inicia nova transmissão, se disponível
	    TX_Service();

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

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        tx_done = 1;
    }
}

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
