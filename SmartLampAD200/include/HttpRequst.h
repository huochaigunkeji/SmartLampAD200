#ifndef __HTTP_REQUST_H__
#define __HTTP_REQUST_H__


#define GET 	0
#define POST 	1

#ifdef __cplusplus
extern "C" {
#endif

bool HttpRequst( uint8_t method , char * Url , char * body , uint16_t bodylen , uint8_t *buff , uint16_t bufflen );

#ifdef __cplusplus
}
#endif

#endif

