#include "parametros_atmega.h"
#include "metodos_motor.h"
#include "twi.h"

static uint8_t latch_state;
volatile int contador;
volatile float x, y, a, b, c;
volatile float xk1 = 0, xk2 = 0, yk1 = 0, yk2 = 0;

void enable (void)
{
	// setup the latch
	
	set_bit (LATCH_DDR, LATCH);
	set_bit (ENABLE_DDR, ENABLE);
	set_bit (CLK_DDR, CLK);
	set_bit (SER_DDR, SER);

	latch_state = 0;

	latch_tx();  // "reset"

	clr_bit(ENABLE_PORT, ENABLE); // enable the chip outputs! 
}

void latch_tx (void)
{
	uint8_t i;

	clr_bit (LATCH_PORT, LATCH);

	clr_bit (SER_PORT, SER);

	for (i=0; i<8; i++) 
	{
		clr_bit (CLK_PORT, CLK);
		
		if (latch_state & _BV(7-i)) 
		{
			set_bit (SER_PORT, SER);
		} 
		else 
		{
			clr_bit (SER_PORT, SER);
		}
		set_bit (CLK_PORT, CLK);
	}
	set_bit (LATCH_PORT, LATCH);
}

//num = numero do motor de 1 a 4

void run (uint8_t cmd, uint8_t motornum)
{
	uint8_t a, b;
	
	switch (motornum)
	{
		case 1:
		a = MOTOR1_A; b = MOTOR1_B; break;
		
		case 2:
		a = MOTOR2_A; b = MOTOR2_B; break;
		
		case 3:
		a = MOTOR3_A; b = MOTOR3_B; break;
		
		case 4:
		a = MOTOR4_A; b = MOTOR4_B; break;
		
		default:
		return;
	}
	
	switch (cmd) 
	{
		case FORWARD:
		set_bit (latch_state, a);
		clr_bit (latch_state, b);
		latch_tx ();
		break;
		
		case BACKWARD:
		clr_bit (latch_state, a);
		set_bit (latch_state, b);
		latch_tx ();
		break;
		
		case RELEASE:
		clr_bit (latch_state, a);
		clr_bit (latch_state, b);
		latch_tx ();
		break;
	}
}

void initPWM1 (void) 
{
	// use PWM from timer2A on PB3 (Arduino pin #11)
	set_bit(TCCR2A, COM2A1) | set_bit(TCCR2A, COM2B1) | set_bit(TCCR2A, WGM20) | set_bit(TCCR2A, WGM21); // fast PWM, turn on oc2a
	set_bit(TCCR2B, WGM22) | set_bit(TCCR2B, CS22);
	OCR2A = 10;
	OCR2B = 20;
}

void initPWM4 (void) 
{
	// use PWM from timer0B / PD5 (pin 5)
	set_bit(TCCR0A,COM0B1) | set_bit(TCCR0A, COM0A1) | set_bit(TCCR0A, WGM00) | set_bit(TCCR0A, WGM01);
	set_bit(TCCR0B, CS01) | set_bit(TCCR0B, CS00) | set_bit(TCCR0B, WGM02);
	OCR0B = 10;
	OCR2B = 20;
} 

//mudar para interupcao do TC0 e TC2
ISR ( TIMER2_OVF_vect )                           //interrupção do TC2 
{                                                //aqui vai a saida "impressa" no sinal PWM
	x = 1.0;
	
	if (contador == 4000){
		y = -0.91*yk2 + 1.9*yk1 + 0.01*xk2;
		
		xk2 = xk1;
		xk1 = x;
		yk2 = yk1;
		yk1 = y;
		contador = 0;
		OCR2B = y*100;                   //representa a velocidade de rotacao do motor
	}
	contador++;
}

ISR ( TIMER0_OVF_vect )                           //interrupção do TC0
{                                                //aqui vai a saida "impressa" no sinal PWM
	x = 1.0;
	
	if (contador == 4000){
		y = -0.91*yk2 + 1.9*yk1 + 0.01*xk2;
		
		xk2 = xk1;
		xk1 = x;
		yk2 = yk1;
		yk1 = y;
		contador = 0;
		OCR0B = y*100;
	}
	contador++;
}
