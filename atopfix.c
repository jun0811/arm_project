#include "LPC17xx.h"
#include "Board_LED.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_libcfg_default.h"
#include "lpc17xx_pinsel.h"
#include "debug_frmwrk.h"

// uart rx interrupt reg
// uart tx polling func
// timer interrupt reg
// uart 설정은 func reg 섞어서
// uart rx랑 timer 방식이 polling이여서 감점
// interrupt할거면 status 더 만들고 main안에 
// while(complete) 문 제거

/*****************************************
	Global var. declaration
******************************************/
int32_t global_status;
uint32_t Password;
uint32_t balance;
uint32_t UART_rxdata;
uint32_t Selected_op;

/*****************************************
	function declaration
******************************************/
void Send_Ascii( uint8_t  num);
int Get_Balance_Length(int32_t num); 
int Int_to_Ascii(int32_t num);
int Ascii_to_Int(int32_t num);
int Power10(int32_t num);
/*****************************************
	main function
******************************************/
int main(void){
	
	// var. declaration
	UART_CFG_Type UART_config_struct;
	int32_t i;
	int32_t length;
	uint32_t balance_tmp;
	////////////////////////////////////////////////////////////////
	
	// var. initialization
	global_status = 0;
	Password = 1234;
	balance = 0;
	UART_rxdata = 0;
	Selected_op = 0;
	length = 0;
	balance_tmp =0;
	SystemCoreClockUpdate();
	LED_Initialize();
	////////////////////////////////////////////////////////////////
	
	// NVIC init
	NVIC->ICPR[0] |= (1<<3);	// NVIC Timer2 clear pending
	NVIC->ISER[0] |= (1<<3);		// NVIC Timer2 set enable
	NVIC->ICPR[0] |= (1<<5);	// NVIC UART0 clear pending
	NVIC->ISER[0] |= (1<<5);		// NVIC UART0 set enable
	////////////////////////////////////////////////////////////////
	
	// Power & PCLK init
	LPC_SC -> PCONP |= (1<<22);	// system control , timer2 on
	LPC_SC -> PCLKSEL1 |= (0<<12);	// divide cclk by 4 for timer2
	LPC_SC -> PCONP |= (1<<3);	// system control , UART0 on
	LPC_SC -> PCLKSEL0 |= (0<<6);	// UART0 PCLK = CCLK/4
	////////////////////////////////////////////////////////////////
	
	// Timer2 init
	LPC_TIM2 -> IR = 0xFFFFFFFF;	// clear interrupt flag
	LPC_TIM2 -> TC = 0;		// clear timer counter
	LPC_TIM2 -> PC = 0;		// clear prescale counter
	LPC_TIM2 -> PR = 0;		// clear prescale reg.
	LPC_TIM2->TCR = 0x00000002;	// timer control reg. reset[1] =1, disable[0]=0
	LPC_TIM2->CTCR = 0x00000000;	// counter control reg. timer mode[1:0] =00
	LPC_TIM2->MR0 = 0x0773593F;	// match reg. 125000000-1, no PR, 125000000 * 40nsec = 5sec
	LPC_TIM2->MCR = 0x00000003;	// match control reg. no stop at match[2]=0, reset[1]=1, interrupt[0]=1
	LPC_TIM2->TCR = 0x00000001;	// timer control reg. normal[1]=0, enable[0]=1
	////////////////////////////////////////////////////////////////
	
	// UART0  init
	UART_config_struct.Baud_rate = 19200;
	UART_config_struct.Parity = UART_PARITY_NONE;
	UART_config_struct.Databits = UART_DATABIT_8;
	UART_config_struct.Stopbits = UART_STOPBIT_1;
	UART_Init( LPC_UART0, &UART_config_struct);
	////////////////////////////////////////////////////////////////
	
	// UART0 enable & Pin Select
	LPC_PINCON -> PINSEL0 |= (1<<4);	// TXD0 func   pin 설정
	LPC_PINCON -> PINSEL0 |= (1<<6);	// RXD0 func   pin 설정
	LPC_UART0 -> IER = 0x01;	//	UART  RBR interrupt enable
	LPC_UART0 -> FCR = 0x01;	// Tx/Rx FIFO enable
	LPC_UART0 -> TER = 0x80;	// UART transmitter enable	
	////////////////////////////////////////////////////////////////
	
	global_status = 2;
	
	// while문 : UART0 Tx polling 
	while(1){
		switch (global_status){
			case 0 : // Set PassWord
				Send_Ascii(10);	// next line
				Send_Ascii(13);	// Enter
				Send_Ascii(83);	// S
				Send_Ascii(69);	// E  
				Send_Ascii(84);	// T  
				Send_Ascii(32);	// space
				Send_Ascii(80);	// P
				Send_Ascii(87);	// W   SETPassWord
				Send_Ascii(58);	// :
				global_status = 1;
				break;
			
			case 2 : // insert PassWord 
				Send_Ascii(10);	// next line
				Send_Ascii(13);	// Enter
				Send_Ascii(80);	// P
				Send_Ascii(87);	// W  PassWord
				Send_Ascii(58);	// :
				global_status = 3;
				break;
										
				
			case 4 :	// Display Balance
				Send_Ascii(10);	// next line
				Send_Ascii(13);	// Enter
				Send_Ascii(66);	// B  Balance
				Send_Ascii(58);	// :
				balance_tmp = balance;
				length = Get_Balance_Length(balance_tmp);
				for ( i= length+1 ; 0<i ; i--){
					Send_Ascii(Int_to_Ascii(balance_tmp/Power10(i-1)));
					balance_tmp -= (balance_tmp/ Power10(i-1) * Power10(i-1));}
				global_status = 5;
				break;
				
			case 6 : // select Withdraw or Deposit or Reset PW
				Send_Ascii(10);	// next line
				Send_Ascii(13);	// Enter
				Send_Ascii(68);	// D
				Send_Ascii(47);	// /
				Send_Ascii(87);	// W  Deposit or Withdraw
				Send_Ascii(47);	// /
				Send_Ascii(82);	// R  Deposit or Withdraw
				Send_Ascii(58);	// :
				global_status = 7;
				break;
										
			case 8 : // How Much ( D or W)
				Send_Ascii(10);	// next line
				Send_Ascii(13);	// Enter
				Send_Ascii(72);	// H
				Send_Ascii(47);	// /
				Send_Ascii(77);	// M  How Much
				Send_Ascii(58);	// :
				global_status = 9;
				break;
			
			default : 	// Wait Rx 
				break;
			}
		}
	////////////////////////////////////////////////////////////////
}

/*****************************************
	Send Ascii 
******************************************/
void Send_Ascii( uint8_t  num){
	int32_t UART_tx_status;
	UART_tx_status = UART_GetLineStatus(LPC_UART0);	// LPC_UART0->LSR
	while((UART_tx_status & 0x60) == 0x00){	// transmitter or THR empty check
		UART_tx_status = UART_GetLineStatus(LPC_UART0);	// LPC_UART0->LSR
	}
	UART_SendByte(LPC_UART0, num);	//
}

/*****************************************
	Get Length of Balance
******************************************/
int Get_Balance_Length(int32_t num){
	int32_t count = 0;
	while(1)
	{
		num /= 10;
		if(num ==0)
			break;
		else
			count = count + 1;
	}
	return count;
}

/*****************************************
	Integer to Ascii
******************************************/
int Int_to_Ascii(int32_t num){
	int32_t result;
	switch (num){
		case 0 : result = 48;	break;
		case 1 : result = 49;	break;
		case 2 : result = 50;	break;
		case 3 : result = 51;	break;
		case 4 : result = 52;	break;
		case 5 : result = 53;	break;
		case 6 : result = 54;	break;
		case 7 : result = 55;	break;
		case 8 : result = 56;	break;
		case 9 : result = 57;	break;
		default : result = 0; break;
	}
	return result;
}

/*****************************************
	Ascii to Integer
******************************************/
int Ascii_to_Int(int32_t num){
	int32_t result;
	switch (num){
		case 48 : result = 0;	break;
		case 49 : result = 1;	break;
		case 50 : result = 2;	break;
		case 51 : result = 3;	break;
		case 52 : result = 4;	break;
		case 53 : result = 5;	break;
		case 54 : result = 6;	break;
		case 55 : result = 7;	break;
		case 56 : result = 8;	break;
		case 57 : result = 9;	break;
		default : break;
		}
	return result;
}

/*****************************************
	Power by 10
******************************************/
int Power10(int32_t num){
	int32_t i;
	int32_t result;
	result = 1;
	for( i = num ; i > 0 ; i--){
		result *= 10;
	}
	return result;
}

/*****************************************
	Timer2 interrupt
******************************************/
void TIMER2_IRQHandler (void){
	LPC_TIM2-> IR = 0x00000001;	// clear timer2 interrupt flag
	if( global_status >1){
		global_status = 2;
		UART_rxdata = 0;}
}

/*****************************************
	UART Rx interrupt
******************************************/
void UART0_IRQHandler(void){
	
	// var delclaration
	int32_t int_status;
	int32_t status;
	int32_t rx_temp;
	////////////////////////////////////////////////////////////////
	
	// Timer2 reset
	LPC_TIM2 -> TC = 0;		// clear timer counter
	LPC_TIM2 -> PC = 0;		// clear prescale counter
	////////////////////////////////////////////////////////////////
	
	// 입력 받기
	int_status = LPC_UART0->IIR;
	if( (int_status & 0x0000000F) == 4) // RBR interrupt check 
	{
		status = LPC_UART0->LSR;
		if( (status & 0x8E) == 0x00)					// error or not
			rx_temp = LPC_UART0->RBR;
		else
			rx_temp = 48;
		if(rx_temp > 32)
			Send_Ascii(rx_temp);}
	////////////////////////////////////////////////////////////////
	
	// global status switch case
	if(rx_temp == 27){	// ESC
		global_status = 2;
		UART_rxdata = 0;}
	else{
		switch (global_status){
			case 1 : // Set PassWord 
				if(rx_temp == 13){	// Enter 면
					Password = UART_rxdata;
					UART_rxdata = 0;
					global_status = 2;}
				else if(rx_temp >= 48 && rx_temp<= 57){	// 값이 숫자면
					UART_rxdata = UART_rxdata*10 + Ascii_to_Int(rx_temp);}
				break;
		
			case 3 : // insert PassWord 
				if(rx_temp == 13){	// Enter 면
					if(UART_rxdata == Password ){
						global_status = 4;}
					else{
						Send_Ascii(10);	// next line
						Send_Ascii(69);	// E(Error) display
						global_status = 2;}	
					UART_rxdata = 0;}
				else if(rx_temp >= 48 && rx_temp<= 57){	// 값이 숫자면
						UART_rxdata = UART_rxdata*10 + Ascii_to_Int(rx_temp);}
				break;
				
			case 5 :	// any key..
				global_status = 6;
				break;
				
			case 7 : // select Withdraw or Deposit or Reset
				if(rx_temp == 68 || rx_temp == 100){	// D or d
					global_status = 8;
					Selected_op = 0;}	// Deposit}
				else if (rx_temp == 87 || rx_temp == 119){	// W or w
					global_status = 8;
					Selected_op = 1;	}// Withdraw
				else if(rx_temp == 82 || rx_temp == 114){	// R or r
					global_status = 0;}
				break;
										
										
			case 9 : // Deposit or Withdraw
				if(rx_temp == 13){	// Enter 면
					if(Selected_op == 0){
						balance += UART_rxdata;
						global_status = 4;}
					else{
						if(balance >= UART_rxdata){
							balance -= UART_rxdata;
							global_status = 4;}
						else{
							Send_Ascii(10);	// next line
							Send_Ascii(69);	// E
							global_status = 8;}}
					UART_rxdata = 0;}
				else if(rx_temp >= 48 && rx_temp<= 57){	// 값이 숫자면
					UART_rxdata = UART_rxdata*10 +Ascii_to_Int(rx_temp);}
				break;
			
			default : // else
				break;
		}
	}
}

#ifdef DEBUG
void check_failed(uint8_t *file, uint32_t line)
{
	while(1);
}
#endif
