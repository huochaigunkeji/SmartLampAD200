#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif
extern xTaskHandle pvUdpServerThreadHandle;

void UdpServerInit( void );

#ifdef __cplusplus
}
#endif

#endif
