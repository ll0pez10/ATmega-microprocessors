#include "MPU6050.h"
#include "parametros_atmega.h"

#define MPU6050 0xD0

volatile uint8_t escrita, oper_TWI_concl, end_escr_leit, dado_escr, dado_leit, passo, cont_max_erro;

void inic_TWI () //SCL = 100 kHz (limite do DS1307) com F_CPU = 16 MHz
{
	//Ajuste da frequência de trabalho - SCL = F_CPU/(16+2.TWBR.Prescaler)
	TWBR = 18;
	set_bit (TWSR, TWPS0);
	set_bit(TWCR, TWINT) | set_bit(TWCR, TWEN) | set_bit(TWCR, TWIE);
}

void escreve_MPU6050 (uint8_t end_escrita, uint8_t dado)
{
	//passa variáveis da função para as variáveis globais alteradas na ISR
	
	escrita = 1;                   //1 para escrita, 0 para leitura
	end_escr_leit = end_escrita;
	dado_escr = dado;
	oper_TWI_concl = 0;           //trava do sistema até a conclusão da transmissão
	
	start_bit ();                //envia o Start bit. Passo (1)
	passo = 1;
	cont_max_erro = 255;
	
	while (oper_TWI_concl==0);  /*se for crítica a espera, o programa principal pode
							      gerenciar esta operação*/
}

uint8_t le_MPU6050 (uint8_t end_leitura)
{
	//passa variáveis da função para as variáveis globais alteradas na ISR
	
	escrita = 0;                  //1 para escrita 0 para leitura
	end_escr_leit = end_leitura;
	oper_TWI_concl=0;            //trava do sistema até a conclusão da transmissão
	
	start_bit ();                 //envia o Start bit. Passo (1)
	passo = 1;
	cont_max_erro = 255;
	
	while (oper_TWI_concl==0);  /*se for crítica a espera, o programa principal pode
								  gerenciar esta operação*/
	
	return dado_leit;
}

ISR (TWI_vect)                         //Rotina de interrupção da TWI
{
	static unsigned char fim_escrita;
	switch (TWSR & 248)               //lê o código de resultado do TWI e executa a próxima ação
	{
		/*LEITURA E ESCRITA
		  PASSO 2 <start condition transmitted>. Passo (1) concluído, executa passo (2)*/
		case (TW_START):
		TWDR = (MPU6050 << 1) | TW_WRITE; //envia endereço do dispositivo e o bit de escrita
		clr_start_bit();//limpa o start bit
		passo = 2;
		break;
		
		/*LEITURA E ESCRITA
		PASSO 3 <SLA+W transmitted, ACK received>. Passo (2) concluído, executa passo (3)*/
		case (TW_MT_SLA_ACK):
		TWDR = end_escr_leit;//envia o endereço de escrita ou leitura
		passo=3;
		fim_escrita=0; //inicializa variável para uso na escrita, PASSO 4
		break;
		
		/*LEITURA E ESCRITA
		PASSO 4 <data transmitted, ACK received>. Passo (3) concluído, executa passo (4).
		Passo (4) concluído, executa passo (5) (só na escrita). O passo (4) para uma leitura é o
		reinício*/
		case (TW_MT_DATA_ACK):
		if(fim_escrita) //se o passo (4) foi concluído executa o (5), escrita
		{
			stop_bit();
			oper_TWI_concl = 1; //avisa que operação foi concluída
			break;
		}
		//envia um único dado quando for operação de escrita e depois um stop_bit()
		if(escrita)
		{
			TWDR = dado_escr;//dado para escrita no endereço de escrita
			fim_escrita = 1;//avisa que é o último dado a ser escrito
		}
		else
			start_bit();//envia reinício (só para operação de leitura)
			passo = 4;
		break;
		
		/*LEITURA
		PASSO 5 <repeated start condition transmitted>. Passo (4) concluído, executa o (5)*/
		case (TW_REP_START):
		TWDR = 0xD1; //Envia endereço e read bit (4)
		clr_start_bit(); //limpa start bit
		passo = 5;
		break;
		
		/*LEITURA
		PASSO 6 <SLA+R transmitted, ACK received>. Passo (5) concluído, prepara para a
		execução do NACK*/
		case (TW_MR_SLA_ACK) :
		TWCR &=~(1<<TWEA); //enviará um NACK após a recepção do dado
		passo=6;
		break;
		
		/*LEITURA
		PASSO 7 <data received, NACK returned>. Passo (6) concluído, NACK recebido, executa
		passo (7)*/
		case (TW_MR_DATA_NACK):
		dado_leit = TWDR; //dado lido
		stop_bit();
		oper_TWI_concl = 1; //avisa que operação foi concluída
		break;
		
		/*TRATAMENTO DOS POSSÍVEIS ERROS
		Quando um erro acontece a operação errada é repetida até funcionar ou até
		que o contador para o número máximo de tentavas chegue a zero*/
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
					start_bit();//reinício
			break;
			case(5): TWDR = 0xD1; break;
			
			case(6): TWCR &=~(1<<TWEA); break;
		}
		
		/*para saber se houve estouro na contagem ou insucesso na
		correção dos erros, basta ler cont_max_erro.*/
		if(cont_max_erro==0)
		{
			stop_bit();
			oper_TWI_concl = 1; //libera o sistema
		}
	}
	set_bit(TWCR, TWINT); //limpa flag de interrupção
}