#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif
extern xTaskHandle pvTcpClientThreadHandle;
void tcp_client_thread_init( void * param );
void tcp_client_thread_delete( void );


#ifdef __cplusplus
}
#endif

#endif

