#ifndef __SOFT_SPI_H__
#define __SOFT_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif
bool SoftSpiInit( void );
bool SpiWriteReg( uint8_t RegAddr , uint8_t Data );
bool SpiReadReg( uint8_t RegAddr , uint8_t * Data );

#ifdef __cplusplus
}
#endif

#endif

