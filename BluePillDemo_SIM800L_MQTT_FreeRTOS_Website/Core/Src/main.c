/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include "task.h"
#include "modem.h"
#include "buffered_serial.h"
#include "mqtt.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ACCESS_POINT_NAME		"everywhere"
#define USER_NAME				"eesecure"
#define PASSWORD				"secure"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
osThreadId_t mainHandle;
uint32_t mainBuffer[ 250 ];
osStaticThreadDef_t mainControlBlock;
osThreadId_t modemHandle;
uint32_t modemBuffer[ 350 ];
osStaticThreadDef_t modemControlBlock;
osMessageQueueId_t commandQueueHandle;
uint8_t commandQueueBuffer[ 1 * sizeof( AtCommandPacket_t ) ];
osStaticMessageQDef_t commandQueueControlBlock;
osMessageQueueId_t responseQueueHandle;
uint8_t responseQueueBuffer[ 1 * sizeof( AtResponsePacket_t ) ];
osStaticMessageQDef_t responseQueueControlBlock;
osMutexId_t modemMutexHandle;
osStaticMutexDef_t modemMutexControlBlock;
/* USER CODE BEGIN PV */
osEventFlagsId_t modemTaskStartedEventHandle;
static bool subscribeResponseReceived;
static uint16_t subscribeResponsePacketIdentifier;
volatile static bool adcConversionComplete;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void mainTask(void *argument);
void modemTask(void *argument);

/* USER CODE BEGIN PFP */
void DebugPrint(const char *text);
void ModemDebugPrintStatus(const char *text, ModemStatus_t modemStatus);
void MqttDebugPrintStatus(const char *text, MqttStatus_t mqttStatus);
void PublishCallback(const char *topic, uint8_t topicLength, const uint8_t *payload, uint32_t payloadLength);
void PingResponseCallback(void);
void SubscribeResponseCallback(uint16_t packetIdentifier, bool success);
void UnsubscribeResponseCallback(uint16_t packetIdentifier);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void PublishCallback(const char *topic, uint8_t topicLength, const uint8_t *payload, uint32_t payloadLength)
{
  DebugPrint("Publish callback: ");
  HAL_UART_Transmit(&huart2, (uint8_t *)topic, topicLength, 100U);
  DebugPrint(",");
  HAL_UART_Transmit(&huart2, (uint8_t *)payload, payloadLength, 100U);
  DebugPrint("\r\n");

  if (topicLength == 18U && memcmp(topic, "BluePillDemo/test3", topicLength) == 0)
  {
	  if (payloadLength == 1UL)
	  {
		  if (*payload == '0')
		  {
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		  }
		  else if (*payload == '1')
		  {
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		  }
	  }
  }
}

void PingResponseCallback(void)
{
}

void SubscribeResponseCallback(uint16_t packetIdentifier, bool success)
{
  subscribeResponseReceived = success;
  subscribeResponsePacketIdentifier = packetIdentifier;
  char numberBuffer[10];

  DebugPrint("Subscribe response callback: ");
  if (success)
  {
	DebugPrint("success,");
  }
  else
  {
	DebugPrint("fail,");
  }
  itoa((int)packetIdentifier, numberBuffer, 10);
  DebugPrint(numberBuffer);
  DebugPrint("\r\n");
}

void UnsubscribeResponseCallback(uint16_t packetIdentifier)
{
}

void MqttDebugPrintStatus(const char *text, MqttStatus_t mqttStatus)
{
  DebugPrint(text);
  DebugPrint(": ");
  DebugPrint(MqttStatusToText(mqttStatus));
  DebugPrint("\r\n");
}

void DebugPrint(const char *text)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)text, strlen(text), 100U);
}

void ModemDebugPrintStatus(const char *text, ModemStatus_t modemStatus)
{
  DebugPrint(text);
  DebugPrint(": ");
  DebugPrint(ModemStatusToText(modemStatus));
  DebugPrint("\r\n");
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	adcConversionComplete = true;
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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  serial_init();
  HAL_UART_Transmit(&huart2, (uint8_t *)"MQTT Test\r\n", 11U, 100UL);

  /* USER CODE END 2 */

  osKernelInitialize();

  /* Create the recursive mutex(es) */
  /* definition and creation of modemMutex */
  const osMutexAttr_t modemMutex_attributes = {
    .name = "modemMutex",
    .attr_bits = osMutexRecursive,
    .cb_mem = &modemMutexControlBlock,
    .cb_size = sizeof(modemMutexControlBlock),
  };
  modemMutexHandle = osMutexNew(&modemMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  modemTaskStartedEventHandle = osEventFlagsNew(NULL);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of commandQueue */
  const osMessageQueueAttr_t commandQueue_attributes = {
    .name = "commandQueue",
    .cb_mem = &commandQueueControlBlock,
    .cb_size = sizeof(commandQueueControlBlock),
    .mq_mem = &commandQueueBuffer,
    .mq_size = sizeof(commandQueueBuffer)
  };
  commandQueueHandle = osMessageQueueNew (1, sizeof(AtCommandPacket_t), &commandQueue_attributes);

  /* definition and creation of responseQueue */
  const osMessageQueueAttr_t responseQueue_attributes = {
    .name = "responseQueue",
    .cb_mem = &responseQueueControlBlock,
    .cb_size = sizeof(responseQueueControlBlock),
    .mq_mem = &responseQueueBuffer,
    .mq_size = sizeof(responseQueueBuffer)
  };
  responseQueueHandle = osMessageQueueNew (1, sizeof(AtResponsePacket_t), &responseQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of main */
  const osThreadAttr_t main_attributes = {
    .name = "main",
    .stack_mem = &mainBuffer[0],
    .stack_size = sizeof(mainBuffer),
    .cb_mem = &mainControlBlock,
    .cb_size = sizeof(mainControlBlock),
    .priority = (osPriority_t) osPriorityNormal,
  };
  mainHandle = osThreadNew(mainTask, NULL, &main_attributes);

  /* definition and creation of modem */
  const osThreadAttr_t modem_attributes = {
    .name = "modem",
    .stack_mem = &modemBuffer[0],
    .stack_size = sizeof(modemBuffer),
    .cb_mem = &modemControlBlock,
    .cb_size = sizeof(modemControlBlock),
    .priority = (osPriority_t) osPriorityLow,
  };
  modemHandle = osThreadNew(modemTask, NULL, &modem_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
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
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config 
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel 
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  huart1.Init.BaudRate = 115200;
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
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_mainTask */
/**
  * @brief  Function implementing the main thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_mainTask */
void mainTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  ModemStatus_t modemStatus;
  MqttStatus_t mqttStatus;
  bool registrationStatus;
  uint32_t newScaledAdcValue = 0UL;
  uint32_t previousScaledAdcValue;
  GPIO_PinState newPinState = GPIO_PIN_SET;
  GPIO_PinState previousPinState;
  char ipAddress[MODEM_MAX_IP_ADDRESS_LENGTH];
  char buf[10];
  uint32_t startTime;

  // wait for modem task to start
  osEventFlagsWait(modemTaskStartedEventHandle, 0x00000001UL, osFlagsWaitAny, osWaitForever);

  DebugPrint("Modem task started\r\n");

  DebugPrint("Testing connection to modem\r\n");
  modemStatus = ModemHello(250UL);
  ModemDebugPrintStatus("Hello", modemStatus);

  DebugPrint("Registering on network\r\n");
  do
  {
	modemStatus = ModemGetNetworkRegistrationStatus(&registrationStatus, 250UL);
	osDelay(1000UL);
  } while (!registrationStatus);
  ModemDebugPrintStatus("Register on network", modemStatus);

  DebugPrint("Setting manual data read\r\n");
  modemStatus = ModemSetManualDataRead(250UL);
  ModemDebugPrintStatus("Set manual read", modemStatus);

  DebugPrint("Configuring data connection\r\n");
  modemStatus = ModemConfigureDataConnection(ACCESS_POINT_NAME, USER_NAME, PASSWORD, 250UL);
  ModemDebugPrintStatus("Configure data connection", modemStatus);

  DebugPrint("Activating data connection\r\n");
  modemStatus = ModemActivateDataConnection(10000UL);
  ModemDebugPrintStatus("Activate data connection", modemStatus);

  DebugPrint("Getting own IP address\r\n");
  modemStatus = ModemGetOwnIpAddress(ipAddress, MODEM_MAX_IP_ADDRESS_LENGTH, 250UL);
  ModemDebugPrintStatus("Get own IP address", modemStatus);

  MqttSetPingResponseCallback(PingResponseCallback);
  MqttSetPublishCallback(PublishCallback);
  MqttSetSubscribeResponseCallback(SubscribeResponseCallback);
  MqttSetUnsubscribeResponseCallback(UnsubscribeResponseCallback);

  // open TCP connection to broker
  DebugPrint("Opening TCP connection\r\n");
  modemStatus = ModemOpenTcpConnection("broker.hivemq.com", 1883U, 8000UL);
  ModemDebugPrintStatus("Open TCP connection", modemStatus);

  // connect to broker
  DebugPrint("Connecting to MQTT broker\r\n");
  mqttStatus = MqttConnect("1234", NULL, NULL, 600U, 20000UL);

  MqttDebugPrintStatus("MQTT connect", mqttStatus);
  if (mqttStatus != MQTT_OK)
  {
	DebugPrint("Could not connect, giving up\r\n");
	while (true)
	{
	  osDelay(1000UL);
	}
  }

  // subscribe to BluePillDemo/test3
  DebugPrint("Subscribing to BluePillDemo/test3\r\n");
  subscribeResponseReceived = false;
  mqttStatus = MqttSubscribe("BluePillDemo/test3", 0x0001U, 5000UL);
  MqttDebugPrintStatus("3MQTT subscribe", mqttStatus);

  while (true)
  {
    mqttStatus = MqttHandleResponse(5000UL);
    MqttDebugPrintStatus("MQTT subscribe handle response", mqttStatus);

    if (subscribeResponseReceived)
    {
	  break;
    }
    osDelay(1000UL);
  }
  DebugPrint("MQTT subscribe response received\r\n");

  // kick off first ADC
  HAL_ADC_Start_IT(&hadc1);

  // go into main loop breaking if ADC scaled value is > 95 && push button pressed
  while (newPinState == GPIO_PIN_SET || newScaledAdcValue < 95UL)
  {
	  // wait until previous ADC conversion complete
	while (!adcConversionComplete)
	{
	  osDelay(10UL);
	}

	// kick off next conversion
	adcConversionComplete = false;
	HAL_ADC_Start_IT(&hadc1);

	// read pushbutton state
	newPinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);

	// has state changed?
	if (newPinState != previousPinState)
	{
		// it's changed so publish
	  MqttDebugPrintStatus("MQTT publish test2", mqttStatus);
	  if (newPinState == GPIO_PIN_SET)
	  {
		  mqttStatus = MqttPublish("BluePillDemo/test2", (uint8_t *)"1", 1, false, 5000UL);
	  }
	  else
	  {
		  mqttStatus = MqttPublish("BluePillDemo/test2", (uint8_t *)"0", 1, false, 5000UL);
	  }
	  MqttDebugPrintStatus("MQTT publish test2", mqttStatus);
	  previousPinState = newPinState;
	}

	// get latest value scaled to 0-100
	newScaledAdcValue = (HAL_ADC_GetValue(&hadc1) * 100UL) / 4096UL;

	// has it changed?
	if (newScaledAdcValue != previousScaledAdcValue)
	{
	  // it's changed so publish
	  itoa(newScaledAdcValue, buf, 10);
	  mqttStatus = MqttPublish("BluePillDemo/test1", (uint8_t *)buf, strlen(buf), false, 5000UL);
	  MqttDebugPrintStatus("MQTT publish test1", mqttStatus);
	  previousScaledAdcValue = newScaledAdcValue;
	}

	// go into wait loop checking for responses
	startTime = osKernelGetTickCount();
	while (true)
	{
		MqttHandleResponse(5000UL);
		if (osKernelGetTickCount() > startTime + 1000UL)
		{
			break;
		}
	    osDelay(250UL);
	}
  }

  // disconnect from broker
  DebugPrint("Disconnecting from broker\r\n");
  mqttStatus = MqttDisconnect(10000UL);
  MqttDebugPrintStatus("MQTT disconnect", mqttStatus);

  // shut down data connection
  DebugPrint("Deactivating data connection\r\n");
  modemStatus = ModemDeactivateDataConnection(10000UL);
  ModemDebugPrintStatus("Deactivate data connection", modemStatus);

  // power down modem
  DebugPrint("Power down modem");
  modemStatus = ModemPowerDown(10000UL);
  ModemDebugPrintStatus("Power down modem", modemStatus);

  (void)modemStatus;
  (void)mqttStatus;

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */ 
}

/* USER CODE BEGIN Header_modemTask */
/**
* @brief Function implementing the modem thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_modemTask */
void modemTask(void *argument)
{
  /* USER CODE BEGIN modemTask */
  ModemReset();
  ModemInit();

  // signal main task that this thread has started
  osEventFlagsSet(modemTaskStartedEventHandle, 0x00000001U);

  /* Infinite loop */
  DoModemTask();

  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END modemTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
