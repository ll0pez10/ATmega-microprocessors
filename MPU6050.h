#ifndef MPU6050_H_
#define MPU6050_H_

//para uso das definições do TWI
#include <util/twi.h>                                

//definicoes de macros para comunicacao I2C
#define start_bit() TWCR |= (1<<TWSTA)
#define stop_bit() TWCR |= (1<<TWSTO)
#define clr_start_bit() TWCR &= ~(1<<TWSTA)

//TWI
void inic_TWI ();
void escreve_MPU6050 (uint8_t end_escrita, uint8_t dado);
uint8_t le_MPU6050 (uint8_t end_leitura);
ISR (TWI_vect);

#endif /* MPU6050_H_ */