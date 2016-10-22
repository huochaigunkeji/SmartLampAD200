/******************************************************************************
 * Copyright 2013-2014 
 *
 * FileName:TcpServer.c
 *
 * Description: 
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"

#include "freertos/portmacro.h"
#include "stdint.h"
#include "driver/gpio.h"

#include "freertos/semphr.h"

/* Define --------------------------------------------------------------------*/
#define SOFT_SPI_CS				GPIO_Pin_5
#define SOFT_SPI_SCK			GPIO_Pin_14
#define SOFT_SPI_MOSI			GPIO_Pin_12
#define SOFT_SPI_MISO			GPIO_Pin_13
#define SOFT_SPI_MISO_NUM		13

#define SOFT_SPI_CS_L			GPIO_OUTPUT( SOFT_SPI_CS , 0 )
#define SOFT_SPI_CS_H			GPIO_OUTPUT( SOFT_SPI_CS , 1 )

#define SOFT_SPI_SCK_L			GPIO_OUTPUT( SOFT_SPI_SCK , 0 )
#define SOFT_SPI_SCK_H			GPIO_OUTPUT( SOFT_SPI_SCK , 1 )

/* Struct --------------------------------------------------------------------*/
/* Varible -------------------------------------------------------------------*/
xSemaphoreHandle xMutexSpi = NULL;


/**
	* @brief  no.	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
static void SoftSpiDelayUs( uint16_t DelayUs )
{
	uint32_t TimerValueOld;
	uint32_t TimerValueCur;	

	TimerValueOld = system_get_time();

	for( ;; )
	{
		TimerValueCur = system_get_time();

		if( TimerValueCur > TimerValueOld )
		{
			if( TimerValueCur - TimerValueOld >= DelayUs )
			{
				break;
			}
		}
		else if( TimerValueCur < TimerValueOld )
		{
			if( ( 0xFFFFFFFF - TimerValueOld ) + TimerValueCur >= DelayUs )
			{
				break;
			}
		}
	}

}

/**
	* @brief  no.	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
bool SoftSpiInit( void )
{
	GPIO_ConfigTypeDef	GPIOConfig;
	
	GPIOConfig.GPIO_Pin = SOFT_SPI_CS;
	GPIOConfig.GPIO_Mode = GPIO_Mode_Output;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	gpio_config( &GPIOConfig );

	GPIOConfig.GPIO_Pin = SOFT_SPI_SCK;
	GPIOConfig.GPIO_Mode = GPIO_Mode_Output;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	gpio_config( &GPIOConfig );	
	
	GPIOConfig.GPIO_Pin = SOFT_SPI_MOSI;
	GPIOConfig.GPIO_Mode = GPIO_Mode_Output;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	gpio_config( &GPIOConfig );

	GPIOConfig.GPIO_Pin = SOFT_SPI_MISO;
	GPIOConfig.GPIO_Mode = GPIO_Mode_Input;
	GPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	GPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	gpio_config( &GPIOConfig );	

	GPIO_OUTPUT( SOFT_SPI_CS , 1 );
	GPIO_OUTPUT( SOFT_SPI_SCK , 1 );
	GPIO_OUTPUT( SOFT_SPI_MOSI , 1 );

	xMutexSpi = xSemaphoreCreateMutex( );

	if( xMutexSpi != NULL )
	{
		return true;
	}
	printf("xMutexSpi create fail!\r\n");
	return false;
}
/**
	* @brief  no.
	* @note   no.
	* @param  no.
	* @retval no.
	*/
static void SpiWriteByte( uint8_t Data )
{
	uint8_t i;
	uint8_t BitValue;

	for( i = 0 ; i < 8 ; i ++ )
	{
		SOFT_SPI_SCK_L;

		BitValue = ( Data << i ) & 0x80;

		BitValue >>= 7;
		GPIO_OUTPUT( SOFT_SPI_MOSI , BitValue );
		
		SoftSpiDelayUs( 3);
		
		SOFT_SPI_SCK_H;

		SoftSpiDelayUs( 3 );
		
	}

}
/**
	* @brief  no.
	* @note   no.
	* @param  no.
	* @retval no.
	*/
static uint8_t SpiReadByte( void )
{
	int8_t i;
	uint8_t BitValue;
	uint8_t data = 0;
	
	GPIO_OUTPUT( SOFT_SPI_MOSI , 0 );
	
	for( i = 7 ; i >= 0 ; i -- )
	{
		SOFT_SPI_SCK_L;
		
		SoftSpiDelayUs( 3);
		
		BitValue = GPIO_INPUT_GET( SOFT_SPI_MISO_NUM );

		BitValue &= 0x01;

		BitValue = BitValue << i;

		data |= BitValue;
		
		SOFT_SPI_SCK_H;

		SoftSpiDelayUs( 3 );
		
	}

	return data;
}

/**
	* @brief  no.
	* @note   no.
	* @param  no.
	* @retval no.
	*/
bool SpiWriteReg( uint8_t RegAddr , uint8_t Data )
{
//	vPortEnterCritical( );

	if( xSemaphoreTake( xMutexSpi, 1000 / portTICK_RATE_MS ) != pdTRUE )
	{
		printf("xMutexSpi take fail!\r\n");
		return false;
	}
	SOFT_SPI_CS_L;
	SoftSpiDelayUs( 12 );

	SpiWriteByte( RegAddr | 0x80 );
	SpiWriteByte( Data );

	SoftSpiDelayUs( 12 );
	SOFT_SPI_CS_H;
	xSemaphoreGive( xMutexSpi );

	return true;

//	vPortExitCritical( );
}
/**
	* @brief  no.
	* @note   no.
	* @param  no.
	* @retval no.
	*/
uint8_t SpiReadReg( uint8_t RegAddr , uint8_t * Data )
{
//	vPortEnterCritical( );
	if( xSemaphoreTake( xMutexSpi, 1000 / portTICK_RATE_MS ) != pdTRUE )
	{
		printf("xMutexSpi take fail!\r\n");
		return false;
	}

	uint8_t vale;
	
	SOFT_SPI_CS_L;
	SoftSpiDelayUs( 12 );

	SpiWriteByte( RegAddr & 0x7F );
	SpiReadByte( );
	vale = SpiReadByte( );
	*Data = vale;
	SoftSpiDelayUs( 12 );
	SOFT_SPI_CS_H;
	xSemaphoreGive( xMutexSpi );
//	vPortExitCritical( );
	return true;

}


