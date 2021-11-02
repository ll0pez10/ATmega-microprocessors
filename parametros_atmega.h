#ifndef PARAMETROS_ATMEGA_H_
#define PARAMETROS_ATMEGA_H_

#define F_CPU 16000000UL                               //define a frequência do microcontrolador - 16MHz
#include <avr/io.h>
#include <util/delay.h>                                //biblioteca para o uso das rotinas de delay
#include <util/twi.h>
#include <avr/pgmspace.h>                             //para o uso do PROGMEM, gravação de dados na memória flash
#include <avr/interrupt.h>                            //para uso das interrupções
#include <stdint.h>


//Definições de macros para o trabalho com bits

#define set_bit(y,bit) (y|=(1<<bit))                 //coloca em 1 o bit x da variável Y
#define clr_bit(y,bit) (y&=~(1<<bit))                //coloca em 0 o bit x da variável Y
#define cpl_bit(y,bit) (y^=(1<<bit))                 //troca o estado lógico do bit x da variável Y
#define tst_bit(y,bit) (y&(1<<bit))                  //retorna 0 ou 1 conforme leitura do bit
#define _BV(bit)       (1 << (bit))

void ADC_conv (void);
uint8_t FastIntSqrRoot (int root);
void messageSend(char tag, int dataUp, int dataDown);
void nextRange(char reg);
void startSelfTest();
void initPCINT();
void changePower();


#endif /* PARAMETROS_ATMEGA_H_ */