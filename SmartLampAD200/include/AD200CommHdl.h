#ifndef __AD200COMMHDL_H__
#define __AD200COMMHDL_H__

#ifdef __cplusplus
extern "C" {
#endif
/* typedef -------------------------------------------------------------------*/
typedef struct{
	uint32_t DataLen;
	uint8_t *Datas;
	uint8_t Cmd;
	uint8_t Param0;
	uint8_t Param1;
}CommTxMsgType;

void AD200CommHdlInit( void );
extern xSemaphoreHandle AD200RequseDataSemaphoreHandle;
extern xSemaphoreHandle UartTxIdleSemaphoreHandle;
extern xQueueHandle CmdTxHandlTaskQueueHandle;
extern xQueueHandle CmdRxHandlTaskQueueHandle;

extern uint8_t CmdBuffer[];

#ifdef __cplusplus
}
#endif

#endif

