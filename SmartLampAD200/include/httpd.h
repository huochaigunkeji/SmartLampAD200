#ifndef __HTTPD_H__
#define __HTTPD_H__

#ifdef __cplusplus
extern "C" {
#endif
extern xTaskHandle pvHttpServerTaskHandle;
void httpd_server_thread_init( void );

#ifdef __cplusplus
}
#endif

#endif

