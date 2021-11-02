#include "parametros_atmega.h"
#include "metodos_motor.h"
#include "MPU6050.h"

volatile uint16_t Inicio_Sinal = 0, Distancia = 0;
static uint8_t latch_state;
char newData;
int microstep[] = {255, 200, 150, 100, 50, 40, 20, 10, 5, 0};

union nome
{
	short int y;
	struct
	{
		short int c1: 8;
		short int c2: 8;
	}campo;
}U1;

ISR ( TIMER1_CAPT_vect )                          //interrupção por captura do valor do TCNT1
{
	cpl_bit (TCCR1B, ICES1);                        //troca a borda de captura do sinal
	
	if( !tst_bit(TCCR1B, ICES1) )                    //lê o valor de contagem do TC1 na borda de subida do sinal
	Inicio_Sinal = ICR1;                          //salva a primeira contagem para determinar a largura do pulso
	
	else                                         //lê o valor de contagem do TC1 na borda de descida do sinal
	Distancia = (ICR1 - Inicio_Sinal) / 58;        /*agora ICR1 tem o valor do TC1 na borda de
	                                               descida do sinal, então calcula a distância */
}

int main (void)
{
	
	cli();
    //motor 1
    latch_state &= ~_BV(MOTOR1_A) & ~_BV(MOTOR1_B); // set both motor pins to 0  
    latch_tx ();
    enable ();
    
    //motor 4
    latch_state &= ~_BV(MOTOR4_A) & ~_BV(MOTOR4_B); // set both motor pins to 0
    latch_tx ();
    enable ();
    
    initPWM1 ();
    initPWM4 ();	                     
	
	//TC1 com prescaler = 8, captura na borda de subida (sonar)
	set_bit(TCCR1B, ICES1)| set_bit(TCCR1B, CS11);   
	
	//habilita a interrupcao do TC1 por captura
	set_bit(TIMSK1, ICIE1);                      
	
	//habilita a interrupcao por overflow do TC0
	set_bit(TIMSK0, TOIE0);
	
	//habilita a interrupcao por overflow do TC2
	set_bit(TIMSK2, TOIE2);
	
	//TWI e UART
	
	int in = 0;
	unsigned char data[6];
	newData = 1;

	DDRC = 0xFF;   // Port C contains the pins for i2c
	DDRB = 1<<5;

	usart_init(9600, F_CPU );

	// set SCL to 400 kHz
	// SCL freq = (CPU freq)/(16 + 2*TWBR*4^TWPS)
	//          = 16*10^6/(16+2*3*4^1) = 400 kHz
	TWBR = 3;
	TWSR = 1;

	_delay_ms(1000);//Wait for power up and monitor connection

	usart_send('R'); //Send reset indicator
	if ((in = read_reg(0x75)) == 0x68) {
		uart_str("Found MPU!\n");
		} else {
		uart_str("init error, got value: ");
		usart_send(in);
		usart_send('\0'); //Send null byte to ensure monitor is killed
		return 0;
	}

	write_reg(0x6B, 0x02);
	
	//habilita interrupcoes
	sei();

    while (1) 
    {
		int i = 0;
		
		run (FORWARD, 1);
		run (FORWARD, 4);
		
		if (Distancia < 30) 
		{
			clr_bit(TIMSK2, TOIE2);
			clr_bit(TIMSK0, TOIE0);
			
			while (OCR0B != 0)
			{
				OCR0B= microstep[i];
				OCR2B= microstep[i];
				i++;
			}
			i = 0;
			
			run (BACKWARD, 4);
			run (BACKWARD, 1);
			
			_delay_ms (50);
			
			set_bit(TIMSK2, TOIE2);
			set_bit(TIMSK0, TOIE0);
			
			run (FORWARD, 4);
			run (BACKWARD, 1);
		}
		
		U1.y = Distancia;
		usart_send (U1.campo.c1);
		usart_send (U1.campo.c2);
		
		if (newData == 1) 
		{
			read_reg_multiple(data, 0x3B, 6); //Read the accelerometer registers
			messageSend('x', data[0], data[1]);
			messageSend('y', data[2], data[3]);
			messageSend('z', data[4], data[5]);

			read_reg_multiple(data, 0x43, 6); //Read the gyroscope registers
			messageSend('r', data[0], data[1]);
			messageSend('p', data[2], data[3]);
			messageSend('Y', data[4], data[5]);
		}
		

		//parse control inputs
		while ( usart_istheredata() ) 
		{
			switch ( usart_recv() ) 
			{
				case 'a': //Change accelerometer full-scale range
				nextRange(0x1C);
				break;
				case 'g': //Change gyro full-scale range
				nextRange(0x1B);
				break;
				case 't': //Start self testing
				startSelfTest();
				break;
				case 'p': //Modify power setting
				changePower();
				break;
			}
		}
	}
}