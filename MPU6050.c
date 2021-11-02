#include "MPU6050.h"
#include "parametros_atmega.h"

#define MPU6050 0xD0

volatile uint8_t escrita, oper_TWI_concl, end_escr_leit, dado_escr, dado_leit, passo, cont_max_erro;

void inic_TWI () //SCL = 100 kHz (limite do DS1307) com F_CPU = 16 MHz
{
	//Ajuste da frequ�ncia de trabalho - SCL = F_CPU/(16+2.TWBR.Prescaler)
	TWBR = 18;
	set_bit (TWSR, TWPS0);
	set_bit(TWCR, TWINT) | set_bit(TWCR, TWEN) | set_bit(TWCR, TWIE);
}

void escreve_MPU6050 (uint8_t end_escrita, uint8_t dado)
{
	//passa vari�veis da fun��o para as vari�veis globais alteradas na ISR
	
	escrita = 1;                   //1 para escrita, 0 para leitura
	end_escr_leit = end_escrita;
	dado_escr = dado;
	oper_TWI_concl = 0;           //trava do sistema at� a conclus�o da transmiss�o
	
	start_bit ();                //envia o Start bit. Passo (1)
	passo = 1;
	cont_max_erro = 255;
	
	while (oper_TWI_concl==0);  /*se for cr�tica a espera, o programa principal pode
							      gerenciar esta opera��o*/
}

uint8_t le_MPU6050 (uint8_t end_leitura)
{
	//passa vari�veis da fun��o para as vari�veis globais alteradas na ISR
	
	escrita = 0;                  //1 para escrita 0 para leitura
	end_escr_leit = end_leitura;
	oper_TWI_concl=0;            //trava do sistema at� a conclus�o da transmiss�o
	
	start_bit ();                 //envia o Start bit. Passo (1)
	passo = 1;
	cont_max_erro = 255;
	
	while (oper_TWI_concl==0);  /*se for cr�tica a espera, o programa principal pode
								  gerenciar esta opera��o*/
	
	return dado_leit;
}

ISR (TWI_vect)                         //Rotina de interrup��o da TWI
{
	static unsigned char fim_escrita;
	switch (TWSR & 248)               //l� o c�digo de resultado do TWI e executa a pr�xima a��o
	{
		/*LEITURA E ESCRITA
		  PASSO 2 <start condition transmitted>. Passo (1) conclu�do, executa passo (2)*/
		case (TW_START):
		TWDR = (MPU6050 << 1) | TW_WRITE; //envia endere�o do dispositivo e o bit de escrita
		clr_start_bit();//limpa o start bit
		passo = 2;
		break;
		
		/*LEITURA E ESCRITA
		PASSO 3 <SLA+W transmitted, ACK received>. Passo (2) conclu�do, executa passo (3)*/
		case (TW_MT_SLA_ACK):
		TWDR = end_escr_leit;//envia o endere�o de escrita ou leitura
		passo=3;
		fim_escrita=0; //inicializa vari�vel para uso na escrita, PASSO 4
		break;
		
		/*LEITURA E ESCRITA
		PASSO 4 <data transmitted, ACK received>. Passo (3) conclu�do, executa passo (4).
		Passo (4) conclu�do, executa passo (5) (s� na escrita). O passo (4) para uma leitura � o
		rein�cio*/
		case (TW_MT_DATA_ACK):
		if(fim_escrita) //se o passo (4) foi conclu�do executa o (5), escrita
		{
			stop_bit();
			oper_TWI_concl = 1; //avisa que opera��o foi conclu�da
			break;
		}
		//envia um �nico dado quando for opera��o de escrita e depois um stop_bit()
		if(escrita)
		{
			TWDR = dado_escr;//dado para escrita no endere�o de escrita
			fim_escrita = 1;//avisa que � o �ltimo dado a ser escrito
		}
		else
			start_bit();//envia rein�cio (s� para opera��o de leitura)
			passo = 4;
		break;
		
		/*LEITURA
		PASSO 5 <repeated start condition transmitted>. Passo (4) conclu�do, executa o (5)*/
		case (TW_REP_START):
		TWDR = 0xD1; //Envia endere�o e read bit (4)
		clr_start_bit(); //limpa start bit
		passo = 5;
		break;
		
		/*LEITURA
		PASSO 6 <SLA+R transmitted, ACK received>. Passo (5) conclu�do, prepara para a
		execu��o do NACK*/
		case (TW_MR_SLA_ACK) :
		TWCR &=~(1<<TWEA); //enviar� um NACK ap�s a recep��o do dado
		passo=6;
		break;
		
		/*LEITURA
		PASSO 7 <data received, NACK returned>. Passo (6) conclu�do, NACK recebido, executa
		passo (7)*/
		case (TW_MR_DATA_NACK):
		dado_leit = TWDR; //dado lido
		stop_bit();
		oper_TWI_concl = 1; //avisa que opera��o foi conclu�da
		break;
		
		/*TRATAMENTO DOS POSS�VEIS ERROS
		Quando um erro acontece a opera��o errada � repetida at� funcionar ou at�
		que o contador para o n�mero m�ximo de tentavas chegue a zero*/
		default:
		cont_max_erro--;
		switch(passo)
		{
			case(1): start_bit(); break;
			
			case(2): TWDR = 0xD0; break;
			
			case(3): TWDR = end_escr_leit; break;
			
			case(4):
				if(escrita)
					TWDR = dado_escr;
				else
					start_bit();//rein�cio
			break;
			case(5): TWDR = 0xD1; break;
			
			case(6): TWCR &=~(1<<TWEA); break;
		}
		
		/*para saber se houve estouro na contagem ou insucesso na
		corre��o dos erros, basta ler cont_max_erro.*/
		if(cont_max_erro==0)
		{
			stop_bit();
			oper_TWI_concl = 1; //libera o sistema
		}
	}
	set_bit(TWCR, TWINT); //limpa flag de interrup��o
}