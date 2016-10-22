/******************************************************************************
 * Copyright 2013-2014 
 *
 * FileName: UdpServer.c
 *
 * Description: 
 *
 * Modification history:
 *     2016/06/20, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"
#include "driver/gpio.h"
#include "driver/Hw_timer.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "espressif/pwm.h"

/*Definition of GPIO PIN params, for GPIO initialization*/
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM 12
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO12

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_1_OUT_IO_NUM 15
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO15

#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_2_OUT_IO_NUM 14
#define PWM_2_OUT_IO_FUNC  FUNC_GPIO14

#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
#define PWM_3_OUT_IO_NUM 4
#define PWM_3_OUT_IO_FUNC  FUNC_GPIO4


#define ZERO_DETECT_PIN_NUM							5
#define SCR_CONTROL_PIN								GPIO_Pin_13
#define ZERO_DETECT_PIN 							GPIO_Pin_5
#define	SCECLOSETIMEMAXVALUE						7500

/* FreeRTOS Task Handle -------------------------------------------------------*/
xTaskHandle PwmAdjControlTaskHandle;
/* FreeRTOS Queue Handle -----------------------------------------------------*/
xQueueHandle PwmAdjControlTaskRxQueueHandle;
/* Varilble ------------------------------------------------------------------*/

uint8_t ScrCloseTimeValue	= 0;				//unit : 99us.range:0~100(>=0 ,<=100);


uint32 io_info[4][3] =
{{PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
{PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
{PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
{PWM_3_OUT_IO_MUX,PWM_3_OUT_IO_FUNC,PWM_3_OUT_IO_NUM}};

uint32_t PwmDuty[4] = { 0 , 0 , 0 , 0 };

/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/

void PwmAdjControlTask( void *pvParameters )
{
	uint8_t AdjDuty = 50;
	PwmAdjControlTaskRxQueueHandle = xQueueCreate( 3 , sizeof( uint8_t ) );								// Create a queue capable of containing 2 pointers to AMessage structures.

	pwm_init( 1000 , PwmDuty , 4 , io_info );					// period : 1ms,duty : 0%,channel num: 4.
	pwm_set_duty( 500 , 1 );
	pwm_start( );

	for( ;; )
	{
		if( xQueueReceive( PwmAdjControlTaskRxQueueHandle , &AdjDuty ,  portMAX_DELAY ) != pdPASS )                  // Blocking wait will be no timeout limit.
		{
			continue;
		}

		uint16_t AdjValue = 1000 * AdjDuty / 100 ;

		pwm_set_duty( AdjValue , 1 );
		pwm_start( );
		
	}
}
/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
void PwmAdjControlInit( void )
{
	xTaskCreate( PwmAdjControlTask, "PwmAdjControlTask", 256, NULL, 1, &PwmAdjControlTaskHandle );			// task priority : 1.
}

/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
void IRAM_ATTR hw_timer_cb( void )
{
	uint32_t TimerValueOld;
	uint32_t TimerValueCur;	
	
	GPIO_OUTPUT( SCR_CONTROL_PIN, 0 );

	TimerValueOld = system_get_time();

	for( ;; )
	{
		TimerValueCur = system_get_time();

		if( TimerValueCur > TimerValueOld )
		{
			if( TimerValueCur - TimerValueOld >= 20 )
			{
				break;
			}
		}
		else if( TimerValueCur < TimerValueOld )
		{
			if( ( 0xFFFFFFFF - TimerValueOld ) + TimerValueCur >= 20 )
			{
				break;
			}
		}
	}

	GPIO_OUTPUT( SCR_CONTROL_PIN, 1 );

	
}
/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
 void IRAM_ATTR ZeroCrossingDetectHandler( void )
{
	uint32 gpio_status;
	uint32 gpio_value;
	uint32 TimerValue;

	gpio_status = GPIO_REG_READ( GPIO_STATUS_ADDRESS );
    GPIO_REG_WRITE( GPIO_STATUS_W1TC_ADDRESS , gpio_status );

	if( gpio_status & (BIT(ZERO_DETECT_PIN_NUM)) )
	{
		gpio_value = GPIO_REG_READ(GPIO_IN_ADDRESS);
		if( ( gpio_value & BIT(ZERO_DETECT_PIN_NUM) ) == BIT(ZERO_DETECT_PIN_NUM) ) 				 // rising edge.
		{
			if( ( ScrCloseTimeValue > 0 ) && ( ScrCloseTimeValue < 100 ) )
			{
				TimerValue = ScrCloseTimeValue * SCECLOSETIMEMAXVALUE / 100;
				hw_timer_arm( TimerValue );
			}
			else if( ScrCloseTimeValue == 0 )
			{
				GPIO_OUTPUT( SCR_CONTROL_PIN, 0 );
			}

			printf("rising edge!\r\n");
		}
		else 													// falling edge.
		{
			GPIO_OUTPUT( SCR_CONTROL_PIN, 1 );
			printf("falling edge!\r\n");
		}

	}
}

/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
void ZeroCrossingDetectInit( void )
{
	GPIO_ConfigTypeDef	GPIOConfig;

	GPIOConfig.GPIO_Pin = SCR_CONTROL_PIN;								//Control pin.
	GPIOConfig.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_DIS;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	gpio_config( &GPIOConfig );
	GPIO_OUTPUT( SCR_CONTROL_PIN, 1 );
	
	GPIOConfig.GPIO_Pin = ZERO_DETECT_PIN;								//Detect pin.
	GPIOConfig.GPIO_Mode = GPIO_Mode_Input;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_ANYEDGE;
	gpio_config( &GPIOConfig );
	
	gpio_intr_handler_register( ZeroCrossingDetectHandler , NULL );

	_xt_isr_unmask(1 << 4);	

//	hw_timer_set_func( hw_timer_cb ); 
	hw_timer_intr_handler_register( hw_timer_cb , NULL );

	hw_timer_init(0);							//not autoload.

	
}


