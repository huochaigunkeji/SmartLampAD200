/******************************************************************************
 * Copyright 2013-2014 
 *
 * FileName:yeelink.c
 *
 * Description: 
 *
 * Modification history:
 *     2016/6/20, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"

#include "espressif/esp8266/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "json/cJSON.h"
#include "PwmAdj.h"

#include "HttpRequst.h"

xTaskHandle pvYeelinkRequstThreadHandle;

void YeelinkRequst( void *pvParameters )
{
	vTaskSuspend( NULL );

	for( ;; )
	{
		char *RecvBuffer;

		RecvBuffer = malloc( sizeof( char ) * 1024 );
		if( RecvBuffer != NULL )
		{
			if( HttpRequst( GET , "api.yeelink.net/v1.1/device/349173/sensor/391009/datapoints" , NULL , 0 , (uint8_t *)RecvBuffer , 1024 ) == true )
			{
				int value;
				cJSON *root = cJSON_Parse( RecvBuffer);
		
				cJSON *child  = cJSON_GetObjectItem( root, "value" );

				if( child != NULL )
				{
					value = child->valueint;

					if( ( value >= 0 ) && (  value <= 100 ) )
					{
						ScrCloseTimeValue = (100 - value);
					}
				}

				cJSON_Delete( root );
				cJSON_Delete( child );
		
				printf("yeelink value:%d\r\n" , value );
			}
			else
			{
				printf("Http requst fail!\r\n" );
			}

			free( RecvBuffer );
		}
		
		vTaskDelay( 2000 / portTICK_RATE_MS );				// ÑÓÊ±2s.
	}
}

void YeelinkRequstInit( void )
{
	pvYeelinkRequstThreadHandle = sys_thread_new("YeelinkRequst" ,  YeelinkRequst , NULL, 256 , 6 );
	if( pvYeelinkRequstThreadHandle != NULL )
	{
		printf("Yeelink Requst Thread Created!\r\n"  );
	}
}

