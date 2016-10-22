#ifndef __PWM_ADJ_H__
#define __PWM_ADJ_H__

#ifdef __cplusplus
extern "C" {
#endif
extern xQueueHandle PwmAdjControlTaskRxQueueHandle;
extern uint8_t ScrCloseTimeValue;
void PwmAdjControlInit( void );
void ZeroCrossingDetectInit( void );


#ifdef __cplusplus
}
#endif

#endif
