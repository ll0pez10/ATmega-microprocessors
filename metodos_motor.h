#ifndef METODOS_MOTOR_H_
#define METODOS_MOTOR_H_

// Bit positions in the 74HCT595 shift register output
#define MOTOR1_A 2
#define MOTOR1_B 3
#define MOTOR2_A 1
#define MOTOR2_B 4
#define MOTOR4_A 0
#define MOTOR4_B 6
#define MOTOR3_A 5
#define MOTOR3_B 7

// Constants that the user passes in to the motor calls
#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3
#define RELEASE 4

// Arduino pin names for interface to 74HCT595 latch
#define LATCH 4
#define LATCH_DDR DDRB
#define LATCH_PORT PORTB

#define CLK_PORT PORTD
#define CLK_DDR DDRD
#define CLK 4

#define ENABLE_PORT PORTD
#define ENABLE_DDR DDRD
#define ENABLE 7

#define SER 0
#define SER_DDR DDRB
#define SER_PORT PORTB

//metodos responsaveis pelo funcionamento do motor DC
void latch_tx (void);
void enable (void);
void run (uint8_t cmd, uint8_t motornum);

void initPWM1 (void);
void initPWM4 (void);
ISR ( TIMER0_OVF_vect );
ISR ( TIMER2_OVF_vect );

uint8_t getlatchstate (void);

#endif /* METODOS_MOTOR_H_ */