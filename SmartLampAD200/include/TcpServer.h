#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif
extern xTaskHandle pvTcpServerThreadHandle;
void tcp_server_thread_init( void );

#ifdef __cplusplus
}
#endif

#endif

