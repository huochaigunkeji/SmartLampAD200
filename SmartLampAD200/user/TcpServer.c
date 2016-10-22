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

#include "espressif/esp8266/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "Led.h"
#include "HttpRequst.h"
#include "PwmAdj.h"

xTaskHandle pvTcpServerThreadHandle;

int StartUp( uint16_t port )
{
	int sock_fd = 0;
	struct sockaddr_in name;

	sock_fd = socket( PF_INET, SOCK_STREAM, 0 );
	if (sock_fd == -1)
	{
		return -1;
	}
	
	memset( &name, 0, sizeof( name ) );
	
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind( sock_fd, (struct sockaddr *)&name, sizeof(name)) < 0 )
	{
		return -1;
	}

	if ( port == 0)  /* if dynamically allocating a port */
	{
		return -1;
	}
	if ( listen(sock_fd, 5) < 0 )
	{
		return -1;
	}
	
	return(sock_fd);	
}

/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/

void tcp_server_thread( void *pvParameters )
{
	char ReadBuf[21];
	int ReadLen;
	int server_sock = -1, client_sock = -1 ;
	
	struct sockaddr_in client_name;
	int client_name_len = sizeof( client_name );

	vTaskSuspend( NULL );

	server_sock = StartUp( 88 );

	if( server_sock == -1 )
	{
		printf("Tcp server startup fail!\r\n");

		vTaskDelete(NULL);
		return;
	}

	for( ;; )
	{
		client_sock = accept( server_sock , (struct sockaddr *)&client_name , &client_name_len );

		if( client_sock != -1)
		{
			ReadLen = recv( client_sock , ReadBuf , 20 , 0 );
			if( ReadLen > 0 )
			{
				ReadBuf[ReadLen] = '\0';
				printf("tcp recv msg:%s!\r\n" , ReadBuf );

				if( strncmp( ReadBuf , "PwmDuty:" , 8 ) == 0 )
				{
					if( ReadBuf[8] != '\0' )
					{
						int Value = atoi( &ReadBuf[8] );

						if( ( Value >= 0 ) && (Value <=100 ) )
						{
							ScrCloseTimeValue = (100 - Value);
//							xQueueSendToBack( PwmAdjControlTaskRxQueueHandle , (uint8_t*)&Value , 20 /portTICK_RATE_MS );
						}
						else
						{
							send( client_sock , "PwmDuty error, range:0~100!" , sizeof( "PwmDuty error, range:0~100!" ) , 0 );
						}
					}
				} 
				else if( strcmp( ReadBuf , "turn on led") == 0)
				{
//					led_on( );
				}
				else if( strcmp( ReadBuf , "turn off led") == 0)
				{
//					led_off( );
				}
				else if( strcmp( ReadBuf , "http get") == 0 )
				{
					char *RecvBuffer;

					RecvBuffer = malloc( sizeof( char ) * 1024 );
					if( RecvBuffer != NULL )
					{
						if( HttpRequst( GET , "api.yeelink.net/v1.1/device/347822/sensor/388779/datapoints" , NULL , 0 , (uint8_t *)RecvBuffer , 1024 ) == true )
						{
							printf("Http Recv:\r\n%s\r\n" , RecvBuffer );
						}
						else
						{
							printf("Http requst fail!\r\n" );
						}

						free( RecvBuffer );
					}
					
				}
			}
		}

		close( client_sock );
	}
}

/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
void tcp_server_thread_init( void )
{
	pvTcpServerThreadHandle = sys_thread_new("tcp_server" ,  tcp_server_thread , NULL, 256 , 7 );
	if( pvTcpServerThreadHandle != NULL )
	{
		printf("tcp_server Created!\r\n"  );
	}
}

