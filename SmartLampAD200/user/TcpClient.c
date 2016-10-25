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
#include "lwip/lwip/Netdb.h"
#include <unistd.h>
#include "AD200CommHdl.h"
#include "string.h"


//#define SERVER_IP			"124.193.120.164"
//#define SERVER_PORT			80
//#define URI_PATH			"/mp3/xajh.mp3"

//#define SERVER_IP			"89.16.185.174"
//#define SERVER_PORT			8003
//#define URI_PATH			"/autodj"
/* Define --------------------------------------------------------------------*/
/* Struct --------------------------------------------------------------------*/
/* Varible -------------------------------------------------------------------*/
static int32 sockfd = -1;
char * data_buf = NULL;
char NetDataBuf[2048];
uint16_t NetDataRecvLen = 0;
/* FreeRTOS Task Handle ------------------------------------------------------*/
xTaskHandle pvTcpClientThreadHandle = NULL;


/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/
void tcp_client_thread( void *pvParameters )
{
	char * Url = pvParameters;
	struct	hostent hostinfo,*phost;
	char buf[101];
	char hostname[100];
	char portstr[6];
	int port;
	int ret;
	char *ipaddr = NULL;
	char *ptr , *ptr1, *path;

	if( Url == NULL )
	{
		printf("error:have no url!\r\n");
		return ;
	}

	if( strncmp( Url , "http://" , 7) == 0 )
	{ 
		Url += 7;
	}

	ptr = strchr( Url , ':' );

	if( ptr == NULL )
	{
		ptr = strchr( Url , '/' );
		if( ptr != NULL )
		{
			port = 80;
		}
		else
		{
			ptr += strlen( Url );
		}

	    path = ptr;
	}
	else
	{
		ptr1 = strchr( ptr , '/' );

		if( ptr1 == NULL )
		{
			ptr1 = strlen( ptr ) + ptr;
		}
		if( ( ptr1 - ptr - 1) < 6 )
		{
			strncpy( portstr , ptr + 1 , ptr1 - ptr - 1 );
			portstr[ptr1 - ptr - 1] = '\0';
			port = atoi( portstr);
		}
		else
		{
			printf("error:parase port!\r\n");
			return;
		}

		path = ptr1;
	}


	if( (ptr - Url) < 100 )
	{
		memcpy( hostname , Url , ptr - Url );
		hostname[ ptr - Url ] = 0x00;
	}
	else
	{
		printf("url is too long!\r\n");
		return;
	}

	printf("hostname:%s,port:%d!\r\n" , hostname , port );
	err_t err;
	uint8_t CycleNum = 0;

	if( inet_addr( hostname ) == INADDR_NONE )
	{
		do
		{
			if( CycleNum > 0 )
			{
	//			sys_msleep( 100 );
				printf("get host by name Count:%d!\r\n" , CycleNum );
				vTaskDelay( 100 / portTICK_RATE_MS );
			}
			err = gethostbyname_r( hostname , &hostinfo , buf , 100 , &phost , &ret );
		}
		while( ( err != ERR_OK ) && ( ++CycleNum < 5 ) );
	
		if( err == ERR_OK )
		{

			int i;

			for( i = 0; hostinfo.h_addr_list[i]; i++ )
			{
				ipaddr = inet_ntoa( *(struct in_addr*)hostinfo.h_addr_list[i] );
				if( ipaddr != NULL )
				{
	//				printf("host addr is:%s\n",  ipaddr );
					break;
				}
			}

			if( ipaddr == NULL )
			{
				printf("error:get ip fail!\r\n");
				return ;
			}
		}
		else
		{
			printf( "error:gethostbyname\r\n" );

			pvTcpClientThreadHandle = NULL;
			vTaskDelete( NULL );
			return ;
		}
	}
	else
	{
		ipaddr = hostname;
		printf("host name is ip!\r\n");
	}

	CommTxMsgType CommTxMsg;
	
	int len;
	struct sockaddr_in address;
	int result;
	int32 nNetTimeout = 2000;

	 data_buf = (char *)malloc( 1461 * sizeof(char) );
	
	if( data_buf == NULL )
	{
		close( sockfd );
		printf("tcp client buf get fail!\r\n");

		pvTcpClientThreadHandle = NULL;
		vTaskDelete( NULL );
		return;
	}
	
	for( ; ; )
	{
		sockfd = socket( PF_INET, SOCK_STREAM, 0 );
		if( sockfd == -1 )
		{
			printf("tcp client socket fail!\r\n");

			vTaskDelay( 100 / portTICK_RATE_MS );
			
			continue;
		}
		else
		{
			printf("tcp CliSock:%d!\r\n" , sockfd );
		}

		memset( &address , 0 , sizeof( address ) );

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr( ipaddr );
		address.sin_port = htons( port );
		address.sin_len = sizeof(address);
		len = sizeof(address);

		int ConnCount;
		ConnCount = 0;
		
		do
		{
		 	result = connect( sockfd, (struct sockaddr *)&address, len );
			if( result != 0 )
			{
				vTaskDelay( 1000 / portTICK_RATE_MS );
			}

			ConnCount++;
			
		}while( ( result != 0 ) && ( ConnCount < 10 ) );

		if( ConnCount == 10 )
		{
			close( sockfd );
			sockfd = -1;
			printf("tcp client connect timerout!\r\n" );
			continue;
		}

		printf("tcp client connect ok!\r\n" );

		setsockopt( sockfd ,SOL_SOCKET , SO_RCVTIMEO ,(char*)&nNetTimeout, sizeof(int) );

		sprintf( data_buf , "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n" , path , hostname );
		write( sockfd ,  data_buf , strlen( data_buf ) );	
		 
		bool ret;
		
		int ReadLen;
		int ReadFailNum;
		uint32_t ContentLen = 0;
		uint32_t ReadTotalLen = 0;
		uint16_t NetDataCopyLen;
	
		
		ReadFailNum = 0;
		NetDataRecvLen = 0;
		
		while( 1 )
		{
			ReadLen = read( sockfd , data_buf , 1460 );

			if( ReadLen > 0 )
			{
				data_buf[ReadLen] = '\0';
				
				if( ReadTotalLen > 0 )
				{
					ReadTotalLen += ReadLen;
					printf( "ReadTotalLen:%d\r\n" , ReadTotalLen );

					if(  NetDataRecvLen + ReadLen >= 2048  )
					{	
						memcpy( (uint8_t *)&NetDataBuf[NetDataRecvLen] , data_buf , 2048 - NetDataRecvLen );

						xSemaphoreTake( AD200RequseDataSemaphoreHandle ,  10000 / portTICK_RATE_MS );	//Wait 10s.

						CommTxMsg.Cmd = 0x92;
						CommTxMsg.Param0 = 0x00;
						CommTxMsg.Param1 =0x00;
						CommTxMsg.Datas = NetDataBuf;
						CommTxMsg.DataLen = 2048;

						xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );

						memcpy( NetDataBuf , (uint8_t *)&data_buf[ 2048 - NetDataRecvLen ] , ReadLen - ( 2048 - NetDataRecvLen ) );
						NetDataRecvLen = ReadLen - ( 2048 - NetDataRecvLen );
					}
					else
					{
						memcpy( (uint8_t *)&NetDataBuf[NetDataRecvLen] , data_buf , ReadLen );
						NetDataRecvLen += ReadLen;
					}

					if( ReadTotalLen >= ContentLen )
					{
						if( NetDataRecvLen > 0 )
						{
							xSemaphoreTake( AD200RequseDataSemaphoreHandle ,  20000 / portTICK_RATE_MS );	//Wait 10s.

							CommTxMsg.Cmd = 0x92;
							CommTxMsg.Param0 = 0x00;
							CommTxMsg.Param1 =0x00;
							CommTxMsg.Datas = NetDataBuf;
							CommTxMsg.DataLen = NetDataRecvLen;

							xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );
						}
 
 						printf("MP3 Code Data Send Ok!\r\n");
					}
#if 0
					uint16_t i;

					for( i = 0 ; i < ReadLen ; i ++ )
					{
						printf("%02X " , data_buf[i]);
					}
#endif					
				}
				else
				{
					int err = -1;
					
					if( strncmp( data_buf, "HTTP/", 5 ) == 0 )
					{
						if( strncmp( &data_buf[9] , "200" , 3 ) == 0 )
						{
							char *ptr1 = NULL , *ptr2 = NULL;
							
							ptr1 = strstr( data_buf , "Content-Length: ");
							if( ptr1 != NULL )
							{
								char LengthStr[15];
								
								ptr2 = strstr( ptr1 , "\r\n" );

								if( ptr2 != NULL )
								{
									ptr1 += 16;
									memcpy( LengthStr , ptr1 , ptr2 - ptr1 );
									LengthStr[ ptr2 - ptr1 ] = '\0';
									ContentLen = atoi( LengthStr );

									printf("file ContentLen:%d\r\n" , ContentLen );
									ptr1 = strstr( ptr2 , "\r\n\r\n" );

									if( ptr1 != NULL )
									{
										ReadTotalLen = ReadLen - ( ptr1 + 4 - data_buf );
										ptr1 += 4; 
										err = 0;
										
										CommTxMsg.Cmd = 0x90;
										CommTxMsg.Param0 = 0x00;
										CommTxMsg.Param1 =0x00;
										CommTxMsg.Datas = NULL;
										CommTxMsg.DataLen = ContentLen;

										xSemaphoreTake( AD200RequseDataSemaphoreHandle ,  50 / portTICK_RATE_MS );	//Wait 50ms.
										
										xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );

										memcpy( NetDataBuf , ptr1 , ReadTotalLen );
										NetDataRecvLen = ReadTotalLen;

										
#if 0
										uint16_t i;
					
										for( i = 0 ; i < ReadTotalLen ; i ++ )
										{
											printf("%02X " , ptr1[i]);
										}
#endif	
									}
								}
							}
						}
					}

					if( err == -1 )
					{
						close( sockfd );
						sockfd = -1;					
						break;
					}
				}
				
//				write( sockfd , data_buf , ReadLen );

				if( ReadTotalLen >= ContentLen )
				{
#if 0
					uint16_t i;

					for( i = 0 ; i < ReadLen ; i ++ )
					{
						printf("%02X " , data_buf[i]);
					}
#endif						
					printf("file download complete!\r\n");
					close( sockfd );
					sockfd = -1;					
					goto end;
				}
				continue;
			}
			else if( ReadLen < 0 )
			{
				if( ( errno == EWOULDBLOCK ) || ( errno == EAGAIN ) )
				{
					printf("tcp recv timeout!\r\n");
					continue;
				}
				
				printf("tcp read error:%d!\r\n" , errno );
//				close( sockfd );
//				sockfd = -1;
//				break;
				close( sockfd );
				sockfd = -1;					
				goto end;
			}
			else
			{
//				printf("tcp connect close!\r\n");
//				close( sockfd );
//				sockfd = -1;
//				break;
				printf("tcp connect close!\r\n");
				close( sockfd );
				sockfd = -1;					
				goto end;
			}
		}
	
	}
end:
	printf("tcp client delete!\r\n");
	free( data_buf );
	pvTcpClientThreadHandle = NULL;
	vTaskDelete( NULL );
	
}
/**
	* @brief  no .	  
	* @note   no.
	* @param  no.
	* @retval no.
	*/

void tcp_client_thread_init( void * param ) 
{
	pvTcpClientThreadHandle = sys_thread_new("tcp_client" ,  tcp_client_thread , param, 384 , 6 );
	if( pvTcpClientThreadHandle != NULL )
	{
		printf("tcp_client Created!\r\n"  );
	}

}

void tcp_client_thread_delete( void )
{
	if( pvTcpClientThreadHandle != NULL )
	{
		vTaskDelete( pvTcpClientThreadHandle );
		pvTcpClientThreadHandle = NULL;
	}
	if( sockfd != -1 )
	{
		close( sockfd );
	}
	if( data_buf != NULL )
	{
		free( data_buf );
	}
}

