/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */


#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/posix/sys/socket.h"

#include "freertos/Timers.h"

#include <stdio.h>
#include <sys/types.h>
//#include <netinet/in.h> 
//#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
//#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "json/cJSON.h"
#include "PwmAdj.h"
#include "SoftSpi.h"
#include "AD200CommHdl.h"

/* Define --------------------------------------------------------------------*/
/* Struct --------------------------------------------------------------------*/
/* Varible -------------------------------------------------------------------*/ 
char Mp3FilePath[50];
/* FreeRTOS Task Handle ------------------------------------------------------*/
xTaskHandle pvHttpServerTaskHandle;
/* FreeRTOS Queue Handle -----------------------------------------------------*/
/* FreeRTOS Semaphore Handle -------------------------------------------------*/
/* function ------------------------------------------------------------------*/

#define ISspace(x) isspace((int)(x)) 

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request(int);
void bad_request(int);
//void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
//void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
//void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

void GetReply( int client ,  char * BodyData , uint16_t BodyDataLen );
uint8_t ParamParse( char * ParamStr , char * *ParamPtrBuf , uint8_t ParamPtrBufSize );
bool GetParamValue( char * *ParamPtrBuf , uint8_t ParamCnt , char const *ParamName , char * ParamValue );
uint8_t FormDataGetValue( char *FormData , char * Name , char * ValueBuf , uint16_t BufLen );

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client)
{
	char buf[1024];
	int numchars;
	char method[10];
	char url[256];
	size_t i, j;
 	char *query_string = NULL;
	uint8_t KeepAlive = 0;

 	numchars = get_line(client, buf, sizeof(buf));
	
	i = 0; j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1) )
	{
		method[i] = buf[j];
		i++; j++;
	}
	method[i] = '\0';

	if ( ( strcasecmp(method, "GET") != 0 ) && ( strcasecmp(method, "POST") != 0 ) )
	{
		unimplemented(client);
	}

	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
	j++;
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	query_string = url;
	while ((*query_string != '?') && (*query_string != '\0'))
	query_string++;
	if (*query_string == '?')
	{
		*query_string = '\0';
		query_string++;
		// cannot_execute( client );
	}
//	printf("url:%s\r\n",url);
//	printf("query:%s\r\n",query_string); 
		
	if ( strcasecmp(method, "GET") == 0 )
	{
		numchars = get_line(client, buf, sizeof(buf) );
		while ((numchars > 0) && strcmp( "\n", buf) )
		{
			numchars = get_line(client, buf, sizeof(buf));
		}
		
		if( strcmp( url , "/") > 0 )
		{

			if( strcasecmp( url , "/mp3_player") == 0 )
			{ 
				char *ParamBuf[3];
				char ParamValue[100];
				uint8_t ParamCnt;
				uint8_t err =0 ;

				cJSON * Result = cJSON_CreateObject( );                          // Create JSON Object.
				
				ParamCnt = ParamParse( query_string  , ParamBuf , 3 );
				
				 if( GetParamValue( ParamBuf , ParamCnt, "download_path=" , ParamValue ) ==true )
				 {
					printf("download_path:%s\r\n" , ParamValue );

					strcpy( Mp3FilePath , ParamValue );
					
					tcp_client_thread_delete( );
					tcp_client_thread_init( Mp3FilePath );
				 }
				 else
				 {
					err ++;
				 }


				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
		 		GetReply( client ,  Body , BodyLen );
				free( Body );
				 
			}
			else if( strcasecmp( url , "/mp3_stop") == 0 )
			{ 
				uint8_t err = 0 ;
				cJSON * Result = cJSON_CreateObject( );                          // Create JSON Object.

				tcp_client_thread_delete( );
				
				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
		 		GetReply( client ,  Body , BodyLen );
				free( Body );
				 
			}	
			else if( strcasecmp( url , "/mp3_suspend") == 0 )
			{
				uint8_t err = 0 ;
				cJSON * Result = cJSON_CreateObject( );                          // Create JSON Object.

				CommTxMsgType CommTxMsg;
				
				CommTxMsg.Cmd = 0x9A;
				CommTxMsg.Param0 = 0x00;
				CommTxMsg.Param1 = 0x00;
				CommTxMsg.Datas = NULL;
				CommTxMsg.DataLen = 0x00;

				xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );
				
				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
		 		GetReply( client ,  Body , BodyLen );
				free( Body );		
			}
			else if( strcasecmp( url , "/mp3_resume") == 0 )
			{
				uint8_t err = 0 ;
				cJSON * Result = cJSON_CreateObject( );                          // Create JSON Object.

				CommTxMsgType CommTxMsg;
				
				CommTxMsg.Cmd = 0x9B;
				CommTxMsg.Param0 = 0x00;
				CommTxMsg.Param1 = 0x00;
				CommTxMsg.Datas = NULL;
				CommTxMsg.DataLen = 0x00;

				xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );
				
				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
		 		GetReply( client ,  Body , BodyLen );
				free( Body );					
			}
			else if( strcasecmp( url , "/mp3_volume") == 0 )
			{
				char *ParamBuf[3];
				char ParamValue[100];
				uint8_t ParamCnt;
				uint8_t err =0 ;

				cJSON * Result = cJSON_CreateObject( ); 						 // Create JSON Object.
				
				ParamCnt = ParamParse( query_string  , ParamBuf , 3 );
				
				 if( GetParamValue( ParamBuf , ParamCnt, "value=" , ParamValue ) ==true )
				 {
					 int Value = atoi( ParamValue );
				 
					 if( ( Value >= 0 ) && (Value <=30 ) )
					 {
						 CommTxMsgType CommTxMsg;
						 
						 CommTxMsg.Cmd = 0xD0;
						 CommTxMsg.Param0 = Value;
						 CommTxMsg.Param1 = 0x00;
						 CommTxMsg.Datas = NULL;
						 CommTxMsg.DataLen = 0x00;
						 
						 xQueueSendToBack( CmdTxHandlTaskQueueHandle , &CommTxMsg , 20 / portTICK_RATE_MS );

					 }
					 else
					 {
						err ++;
					 }

					printf("light_value:%d\r\n" , Value );
					
				 }
				 else
				 {
					err ++;
				 }


				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
				GetReply( client ,	Body , BodyLen );
				free( Body );

			}
			else if( strcasecmp( url , "/lamp") == 0 )
			{
				char *ParamBuf[3];
				char ParamValue[100];
				uint8_t ParamCnt;
				uint8_t err =0 ;

				cJSON * Result = cJSON_CreateObject( ); 						 // Create JSON Object.
				
				ParamCnt = ParamParse( query_string  , ParamBuf , 3 );
				
				 if( GetParamValue( ParamBuf , ParamCnt, "light_value=" , ParamValue ) ==true )
				 {
				 	 int Value = atoi( ParamValue );
				 
					 if( ( Value >= 0 ) && (Value <=100 ) )
					 {
					 	uint8_t i , ReadData;
						
					 	for( i = 0 ; i < 3 ; i ++ )
					 	{
							SpiWriteReg( 0x81 , Value );

							if( SpiReadReg( 0x01 , &ReadData ) )
							{
								if( ReadData == Value )
								{
									break;
								}
								else
								{
									printf("light_value write fail!\r\n" );
								}
							}
						}
						 
					 }

					printf("light_value:%d\r\n" , Value );
					
				 }
				 else
				 {
					err ++;
				 }


				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
				GetReply( client ,	Body , BodyLen );
				free( Body );

			}
			else if( strcasecmp( url , "/led") == 0 )
			{
				char *ParamBuf[3];
				char ParamValue[100];
				uint8_t ParamCnt;
				uint8_t err =0 ;

				cJSON * Result = cJSON_CreateObject( ); 						 // Create JSON Object.
				
				ParamCnt = ParamParse( query_string  , ParamBuf , 3 );
				
				 if( GetParamValue( ParamBuf , ParamCnt, "light_value=" , ParamValue ) ==true )
				 {
				 	 int Value = atoi( ParamValue );
				 
					 if( ( Value >= 0 ) && (Value <=100 ) )
					 {
						 SpiWriteReg( 0x82 , Value );
					 }

					printf("light_value:%d\r\n" , Value );
					
				 }
				 else
				 {
					err ++;
				 }


				if( err == 0 )
				{
					cJSON_AddNumberToObject( Result , "status" , 1 );
				}
				else
				{
					cJSON_AddNumberToObject( Result , "status" , 0 );
					cJSON_AddStringToObject( Result , "msg" , "param error!");
				}
				 
				uint16_t BodyLen;
				char *Body =  cJSON_Print( Result );
				cJSON_Delete( Result );

				if( Body != NULL )
				{
					BodyLen = strlen( Body );
//					printf("Body length:%d!\r\n" , BodyLen );
				}
				else
				{
					printf("Json fail!\r\n");
					BodyLen = 0;
				}
				
				GetReply( client ,	Body , BodyLen );
				free( Body );				
			}
			
		}
	}

	vTaskDelay( 150 / portTICK_RATE_MS );
	close( client );
}
/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
uint8_t ParamParse( char * ParamStr , char * *ParamPtrBuf , uint8_t ParamPtrBufSize )
{
	char * token;
	uint8_t i = 0;

	token = strtok( ParamStr , "&" );
	while( ( token != NULL ) && ( i < ParamPtrBufSize ) )
	{
		ParamPtrBuf[i] = token;
		token = strtok( NULL , "&" );
		i ++;
	}

	return i;
}
/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
bool GetParamValue( char * *ParamPtrBuf , uint8_t ParamCnt , char const *ParamName , char * ParamValue )
{
	uint8_t i;
	uint8_t j;

	j = strlen( ParamName );
	for( i = 0 ; i < ParamCnt ; i ++ )
	{
		if( strncmp( ParamPtrBuf[i] , ParamName , j ) == 0 )
		{
			strcpy( ParamValue , ParamPtrBuf[i] + j );
			return true;
		}
	}
	return false;
}
/**
    * @brief  no .    
    * @note   no.
    * @param  no.
    * @retval no.
    */
uint8_t FormDataGetValue( char *FormData , char * Name , char * ValueBuf , uint16_t BufLen )
{
	char *Ptr, *Ptr1;
	uint8_t NameLen;
	
	NameLen = strlen( Name );
	Ptr = strstr( FormData , "name=" );

	if( Ptr != NULL )
	{
		if( strncmp( (Ptr + 6 ) , Name , NameLen )  == 0 )
		{
			Ptr = strstr( ( Ptr + 6 + NameLen + 1 ) , "\r\n\r\n" );

			if( Ptr != NULL )
			{
				Ptr1 = strstr( Ptr + 4 , "\r\n" );
				if( Ptr1 != NULL )
				{
					*Ptr1 = '\0';

					if( strlen( Ptr +4 ) <= BufLen )
					{
						strcpy( ValueBuf , ( Ptr + 4) );

						return 1;
					}
				}
			}
		}
	
	}

	return 0;
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
	char buf[200];
      
	buf[0]='\0';
	strcat(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
//	send(client, buf, sizeof(buf), 0);
	strcat(buf, "Content-type: text/html\r\n");
//	send(client, buf, sizeof(buf), 0);
	strcat(buf, "\r\n");
//	send(client, buf, sizeof(buf), 0);
	strcat(buf, "<P>Your browser sent a bad request, ");
//	send(client, buf, sizeof(buf), 0);
	strcat(buf, "such as a POST without a Content-Length.\r\n");
	send(client, buf, sizeof(buf), 0);

	
}
/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
	char buf[200];

	strcat(buf, "HTTP/1.0 500 Internal Server Error\r\n");
//	send(client, buf, strlen(buf), 0);
	strcat(buf, "Content-type: text/html\r\n");
//	send(client, buf, strlen(buf), 0);
	strcat(buf, "\r\n");
//	send(client, buf, strlen(buf), 0);
	strcat(buf, "<P>Error cannot execution.\r\n");
	send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
	printf("%s\r\n", sc );
 //perror(sc);
// exit(1);
}
/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
				recv(sock, &c, 1, 0);
				else
				c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
		c = '\n';
	}
	buf[i] = '\0';

	return(i);
}
/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void GetReply( int client ,  char * BodyData , uint16_t BodyDataLen )
{
	char *buf = malloc( BodyDataLen * sizeof( uint8_t ) + 100 );
	
	if( buf != NULL )
	{
		sprintf( buf , "HTTP/1.0 200 OK\r\nContent-type: text/html\r\nContent-Length: %d\r\n" , BodyDataLen );
		strcat( buf , "\r\n" );
		if( BodyData != NULL )
		{
			strcat( buf , BodyData );
		}

		send(client, buf, strlen(buf) , 0 );
		
		free( buf );
		
	}
	else
	{
		printf("Heap not enogh!\r\n");
	}
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
 char buf[1024];
 (void)filename;  /* could use filename to determine file type */

 strcpy(buf, "HTTP/1.0 200 OK\r\n");
 send(client, buf, strlen(buf), 0);
 strcpy(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 strcpy(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}
#if 0
/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
 FILE *resource = NULL;
 int numchars = 1;
 char buf[1024];

 buf[0] = 'A'; buf[1] = '\0';
 while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  numchars = get_line(client, buf, sizeof(buf));

 resource = fopen(filename, "r");
 if (resource == NULL)
  not_found(client);
 else
 {
  headers(client, filename);
  cat(client, resource);
 }
 fclose(resource);
}
#endif
/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
	int httpd = 0;
	struct sockaddr_in name;

	httpd = socket( PF_INET, SOCK_STREAM, 0 );
	if (httpd == -1)
	{
		return -1;
	}
	
	memset( &name, 0, sizeof( name ) );
	
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind( httpd, (struct sockaddr *)&name, sizeof(name)) < 0 )
	{
		return -1;
	}

	if (*port == 0)  /* if dynamically allocating a port */
	{
	int namelen = sizeof(name);
	if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
	{
		return -1;
	}
	
	*port = ntohs(name.sin_port);
	}
	if ( listen(httpd, 5) < 0 )
	{
		return -1;
	}
	
	return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
 	char buf[1024];

	buf[0]='\0';
	 strcat(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, SERVER_STRING);
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "Content-Type: text/html\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "</TITLE></HEAD>\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "<BODY><P>HTTP request method not supported.\r\n");
//	 send(client, buf, strlen(buf), 0);
	 strcat(buf, "</BODY></HTML>\r\n");
	 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
void httpd_server_thread( void *arg )
{
	 int server_sock = -1;
	 u_short port = 80;
	 int client_sock = -1;
	 uint8_t Cnt = 0;
//	 int32 nNetTimeout = 200 ;
	 struct sockaddr_in client_name;
	 
	 int client_name_len = sizeof( client_name );
	 //pthread_t newthread;

	 vTaskSuspend( NULL );
	 
	 server_sock = startup(&port);
	 
	 if( server_sock == -1 )
	 {
		vTaskDelete( NULL );
		return;
	 }
//	 setsocsetsockoptkopt( server_sock ,SOL_SOCKET , SO_RCVTIMEO ,(char*)&nNetTimeout, sizeof(int) );
	 printf("httpd running on port %d\n", port);
	 
	 while (1)
	 {
	 	client_sock = accept( server_sock , (struct sockaddr *)&client_name , &client_name_len );
//		printf("Server accept requst,socket:%d!\r\n" , client_sock );
		if( client_sock >= 0 )
		{
			accept_request( client_sock );
			printf("SerSock:%d!\r\n" , client_sock ); 
		}
		else
		{
			printf("accept fail!\r\n");
		}

		if( ++Cnt > 50 )
		{
			Cnt = 0 ;
			printf( "http server stack lave :%d!\r\n" , uxTaskGetStackHighWaterMark( NULL ) );
		}

 	}

	close( server_sock );

}
/**
  * @brief  Initialize the HTTP server (start its thread) 
  * @param  none
  * @retval None
  */
void httpd_server_thread_init( )
{
	pvHttpServerTaskHandle = sys_thread_new("httpd_server" ,  httpd_server_thread , NULL, 1024 , 7 );
	if( pvHttpServerTaskHandle != NULL )
	{
		printf("httpd_server Created!task_hdl : %x\r\n" , pvHttpServerTaskHandle );
	}
	
}
/**
  * @brief  none
  * @param  none
  * @retval None
  */
bool MP3DownLoadThread(  void * param )
{
	;
}


