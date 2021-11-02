#include "parametros_atmega.h"


void ADC_conv ()
{
	//configura ADC
	
	set_bit(ADMUX, REFS1) | set_bit(ADMUX,REFS0);                               //Tensão interna de ref (1.1V), canal 0
	
	ADCSRA ^= ADCSRA;
	clr_bit(ADCSRA, ADIF);                                                     /*habilita o AD, habilita interrupção, modo de conversão
	                                                                             contínua, prescaler = 128*/
	ADCSRB = 0x00;                                                             //modo de conversão contínua
	set_bit(DIDR0, ADC0D);                                                   //desabilita pino PC0 como I/0, entrada do ADC0
}

// Fast Integer Square Root
// Sqrt(128^2 + 128^2) = 181.02 so we only need an 8bit register

uint8_t FastIntSqrRoot (int root)
{
	uint8_t tmp = 0;
	
	for (int8_t i=7; i > -0; i--)
	{
		tmp |= (1 << i);
		if ( (tmp * tmp) > root)
		tmp ^= (1 << i);
	}
	return tmp;
}
/*
 * changePower: toggle between standard and low power interfaces
 * reads the current setting of cycle and writes based on that
 */
void changePower()
{
   if ((read_reg(0x6B) & 0x20) == 0) {
      write_reg(0x6B, 0x24); //Enable cycle, disable temp, use internal clock
      write_reg(0x6C, 0x37); //Wakeup of 40 Hz, disable gyroscopes
   } else {
      write_reg(0x6C, 0x00); //Enable all components
      write_reg(0x6B, 0x02); //Disable cycle, revert to Y gyro clock source
   }
}

/*
 * Run built-in self test on all accelerometers and gyros
 */
void startSelfTest()
{
   write_reg(0x1B, 0xE0);
   write_reg(0x1C, 0xF0);
}

/*
 * Switch the range setting of the gyro or accelerometer
 *
 * parameters:
 * reg - the register to modify, either 0x1B or 0x1C
 *       undefined operation for other values.
 */
void nextRange(char reg)
{
   char in;
   in = (read_reg(reg) & 0x18) >> 3;
   if (in != 3) {
      in++;
   } else {
      in = 0;
   }
   write_reg(reg, in << 3);
}

/*
 * Send the standard format of message over UART connection
 * message is 3 bytes long, one byte as a message identifier and then a 16 bit
 * number representing the data.
 */
void messageSend(char tag, int dataUp, int dataDown)
{
   usart_send(tag);
   usart_send(dataUp);
   usart_send(dataDown);
}

int8_t vetor_velocidade_ponto (int ax, int ay, int vx, int vy, int t2, uint8_t *vetor_velocidade[2])
{
	int8_t velocidade_x =  vx + ax * t2;
	int8_t velocidade_y =  vy + ay * t2;
	
	uint8_t modulo = FastIntSqrRoot ( ( velocidade_x * velocidade_x ) + ( velocidade_y * velocidade_y ) );
	vetor_velocidade[0] = velocidade_x/modulo;
	vetor_velocidade[1] = velocidade_y/modulo;
}


