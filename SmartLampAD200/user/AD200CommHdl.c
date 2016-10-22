/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#include "lwip/sockets.h"
#include "lwip/dns.h"

#include "string.h"

#include "driver/uart.h"
#include "Crc32.h"
#include "AD200CommHdl.h"

/* Define ---------------------------------------------------------------------*/
#define COMMRSVDATAPOS					10
#define COMMINXDATAPOS					12
#define COMMLENDATAPOS					16
#define COMMFRMDATAPOS					20

#define COMMRSVDATALEN					2
#define COMMINXDATALEN					4
#define COMMLENDATALEN					4

/* FreeRTOS Task Handle -------------------------------------------------------*/
xTaskHandle AD200CommRxHdlTaskHandle; 
xTaskHandle AD200CommTxHdlTaskHandle;
/* FreeRTOS Queue Handle ------------------------------------------------------*/
xQueueHandle CmdTxHandlTaskQueueHandle;
xQueueHandle CmdRxHandlTaskQueueHandle;
/* FreeRTOS Semaphore Handle --------------------------------------------------*/
xSemaphoreHandle AD200RequseDataSemaphoreHandle = NULL;
xSemaphoreHandle UartTxIdleSemaphoreHandle = NULL;

uint8_t CmdBuffer[512];

uint8_t CmdTxBuffer[20 + 2048 ] = { 0x55 , 0x55 , 0x55 , 0x55 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 };

/**
    * @brief  no.
    * @note   no.
    * @param  no.
    * @retval no.
    */
void CommTx( uint8_t Cmd , uint8_t CmdParam0 , uint8_t CmdParam1 , uint8_t * Data , uint32_t DataLen )
{
	uint8_t CheckSum;
	uint16_t i = 0;
	
	CmdTxBuffer[ 4 ] = Cmd;					// Command.
	CmdTxBuffer[ 5 ] = CmdParam0;
	CmdTxBuffer[ 6 ] = CmdParam1;
//	CmdTxBuffer[ 7 ] = 0x00;				// SrcAddr.
//	CmdTxBuffer[ 8 ] = 0x00;				// DestAddr.

	CheckSum = 0x00;
	for( i = 0 ; i < 9 ; i ++ )
	{
		CheckSum += CmdTxBuffer[i];
	}

	CmdTxBuffer[9] = CheckSum;	//CheckSum.

	if( ( DataLen > 0 ) )
	{
		CmdTxBuffer[ COMMLENDATAPOS ] = (uint8_t)(DataLen & 0xFF);
		CmdTxBuffer[ COMMLENDATAPOS + 1 ] = (uint8_t)(( DataLen >> 8 ) & 0xFF);
		CmdTxBuffer[ COMMLENDATAPOS + 2 ] = (uint8_t)(( DataLen >> 16 ) & 0xFF);
		CmdTxBuffer[ COMMLENDATAPOS + 3 ] = (uint8_t)(( DataLen >> 24 ) & 0xFF);
		
		if( ( Data != NULL ) && ( DataLen <= 2048 ) )
		{
			memcpy( (uint8_t *)&CmdTxBuffer[ COMMFRMDATAPOS ] , Data , DataLen );

			uint32_t crc32_value;
			crc32_value = crc32( (uint8_t *)&CmdTxBuffer[ COMMFRMDATAPOS ] , 2048 );

			CmdTxBuffer[ COMMINXDATAPOS ] = (uint8_t)(crc32_value & 0xFF);
			CmdTxBuffer[ COMMINXDATAPOS + 1 ] = (uint8_t)(( crc32_value >> 8 ) & 0xFF);
			CmdTxBuffer[ COMMINXDATAPOS + 2 ] = (uint8_t)(( crc32_value >> 16 ) & 0xFF);
			CmdTxBuffer[ COMMINXDATAPOS + 3 ] = (uint8_t)(( crc32_value >> 24 ) & 0xFF);

	//		printf("Package data!\r\n");
		}
	}
#if 0	
	for( i = 0; i < 20 + 2 * 1024 ; i ++ )
	{
		uart_tx_one_char( UART0 , CmdTxBuffer[ i ] );
	}
#endif	

}
/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
void ICACHE_FLASH_ATTR
CmdTxHandltask(void *pvParameters)
{
	CommTxMsgType CommTxMsg;
	
	CmdTxHandlTaskQueueHandle = xQueueCreate( 3 , sizeof( CommTxMsgType ) );
	vSemaphoreCreateBinary( UartTxIdleSemaphoreHandle );
	
	for( ;; )
	{
		if( xQueueReceive( CmdTxHandlTaskQueueHandle , &CommTxMsg ,  5000 / portTICK_RATE_MS ) != pdPASS )                  // Blocking wait will be no timeout limit.
		{
			continue;
		}

#if 1
		if( xSemaphoreTake( UartTxIdleSemaphoreHandle ,  10000 / portTICK_RATE_MS ) == true )
		{
			CommTx( CommTxMsg.Cmd , CommTxMsg.Param0 , CommTxMsg.Param1 , CommTxMsg.Datas , CommTxMsg.DataLen );
			printf("UartFifoTx!\r\n");
			UartFifoTxStart( UART0 , CmdTxBuffer , 20 + 2 * 1024 );
		}
#endif		
		
	}
}

/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
void ICACHE_FLASH_ATTR
CmdRxHandltask(void *pvParameters)
{
	uint8_t Msg;

	CmdRxHandlTaskQueueHandle = xQueueCreate( 1 , 1 );
	vSemaphoreCreateBinary( AD200RequseDataSemaphoreHandle );
	
	xSemaphoreTake( AD200RequseDataSemaphoreHandle ,  20 / portTICK_RATE_MS );
	
	for( ;; )
	{
		if( xQueueReceive( CmdRxHandlTaskQueueHandle , &Msg ,  5000 / portTICK_RATE_MS ) != pdPASS )                  // Blocking wait will be no timeout limit.
		{
			continue;
		}
#if 0
		printf("Cmd:");

		uint16_t i;
		
		for( i = 0 ; i < Msg ; i ++ )
		{
			printf("%02X " , CmdBuffer[i] );
		}

		printf("\r\n");
#endif
		if( ( CmdBuffer[0] == 0x55 ) && ( CmdBuffer[1] == 0x55 ) &&  ( CmdBuffer[2] == 0x55 ) && ( CmdBuffer[3] == 0x55 ) ) //Check Protocol Head.
		{
			switch( CmdBuffer[4] )	
			{
				case 0x70 :
					printf("Net Config!\r\n");
	//				xSemaphoreGive( SmartSemaphoreHandle );

					vTaskDelay( 5000 / portTICK_RATE_MS );
					CommTx( 0x71 , 0x01 , 0x00 , NULL , 0 );
					break;
				case 0x91 :
					printf("Requset Data!\r\n");
					xSemaphoreGive( AD200RequseDataSemaphoreHandle );
					break;
				case 0xA0 :
					printf("Song Play Finished!\r\n");
					break;
				case 0x51 :
					printf("Data Send error!\r\n");
					break;				
				default :

					break;
			}

			printf("Cmd:%02X!\r\n" , CmdBuffer[4] );
			
		}
		else
		{
			printf("Cmd:");
			
			uint16_t i;
			
			for( i = 0 ; i < Msg ; i ++ )
			{
				printf("%02X " , CmdBuffer[i] );
			}
			
			printf("\r\n");
		}
	}
}
/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
	
void AD200CommHdlInit( void )
{

	xTaskCreate( CmdRxHandltask, "CmdRxHandltask", 384, NULL, 9, &AD200CommRxHdlTaskHandle );                          // priority : 9.
	if( AD200CommRxHdlTaskHandle != NULL )
	{
		printf("AD200 Comm Rx Handle Task Created!\r\n"  );
	}

	xTaskCreate( CmdTxHandltask, "CmdTxHandltask", 384, NULL, 8, &AD200CommTxHdlTaskHandle );                       // priority : 8.
	if( AD200CommTxHdlTaskHandle != NULL )
	{
		printf("AD200 Comm Tx Handle Task Created!\r\n"  );
	}

}

