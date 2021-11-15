/*	Author: Charles Hong
 *  Partner(s) Name: N/A
 *	Lab Section: 022
 *	Assignment: Lab #11  Exercise #3
 *	Exercise Description: Display keypad on LCD
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *  Demo Link: https://drive.google.com/file/d/1xJS4oyVJ-7V1D396HYj59RlnmkxbhoWl/view?usp=sharing
 */

#include <avr/io.h>
#include "io.h"
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif


volatile unsigned char TimerFlag = 0; //TimerISR sets it to 1, programmer sets it to 0
unsigned long _avr_timer_M = 1; //start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; //current internal count of 1ms ticks

void TimerOn() {
    TCCR1B = 0x0B;
    OCR1A = 125;
    TIMSK1 = 0x02;
    TCNT1  = 0;
    _avr_timer_cntcurr = _avr_timer_M;
    SREG |= 0x80;
}

void TimerOff() {
    TCCR1B = 0x00;
}

void TimerISR() {
    TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
    _avr_timer_cntcurr--;
    if(_avr_timer_cntcurr == 0) {
        TimerISR();
        _avr_timer_cntcurr = _avr_timer_M;
    }
}

void TimerSet(unsigned long M) {
    _avr_timer_M = M;
    _avr_timer_cntcurr = _avr_timer_M;
}
unsigned char GetBit(unsigned char port, unsigned char number) 
{
    return ( port & (0x01 << number) );
}
unsigned char GetKeypadKey() {

    // Check keys in col 1
    PORTC = 0xEF;
    asm("nop"); // add a delay to allow PORTx to stabilize before checking
    if ( GetBit(PINC,0)==0 ) { return '1'; }
    if ( GetBit(PINC,1)==0 ) { return '4'; }
    if ( GetBit(PINC,2)==0 ) { return '7'; }
    if ( GetBit(PINC,3)==0 ) { return '*'; }

    // Check keys in col 2
    PORTC = 0xDF;
    asm("nop"); // add a delay to allow PORTx to stabilize before checking
    if ( GetBit(PINC,0)==0 ) { return '2'; }
    if ( GetBit(PINC,1)==0 ) { return '5'; }
    if ( GetBit(PINC,2)==0 ) { return '8'; }
    if ( GetBit(PINC,3)==0 ) { return '0'; }

    // Check keys in col 3
    PORTC = 0xBF;
    asm("nop"); // add a delay to allow PORTx to stabilize before checking
    if ( GetBit(PINC,0)==0 ) { return '3'; }
    if ( GetBit(PINC,1)==0 ) { return '6'; }
    if ( GetBit(PINC,2)==0 ) { return '9'; }
    if ( GetBit(PINC,3)==0 ) { return '#'; }

    // Check keys in col 4
    PORTC = 0x7F;
    asm("nop"); // add a delay to allow PORTx to stabilize before checking
    if ( GetBit(PINC,0)==0 ) { return 'A'; }
    if ( GetBit(PINC,1)==0 ) { return 'B'; }
    if ( GetBit(PINC,2)==0 ) { return 'C'; }
    if ( GetBit(PINC,3)==0 ) { return 'D'; }
    
    return '\0';
}


//Struct for Tasks represent a running process in our simple real-time operating system
typedef struct _task{
    // Tasks should have members that include: state, period,
    //a measurement of elapsed time, and a function pointer.
    signed   char state;        //Task's current state
    unsigned long period;       //Task period
    unsigned long elapsedTime;  //Time elapsed since last task tick
    int (*TickFct)(int);        //Task tick function
} task;
unsigned char numTasks = 2;
task tasks[2];


unsigned char keySymbol;
unsigned char prevSymbol;
enum KeypadSM {keypad_start, wait, press} keypad_state;
int KeypadTick(int state) {
    unsigned char prevSymbol = GetKeypadKey();
    switch (state) {
        case keypad_start:
        state = wait;
        keySymbol = '\0';
        prevSymbol = '\0';
        break;
        case wait:
        if(prevSymbol=='\0') {
            state = wait;
        } else {
            state = press;
            keySymbol = prevSymbol;
        }
        break;
        case press:
        if(keySymbol==prevSymbol) {
            state = press;
        } else {
            state = wait;
        }
        break;
    }
    return state;
}

char string[] = "                 CS120B is Legend... wait for it DARY!";
unsigned char strSz = 54;
unsigned char idx = 0;
enum DisplaySM {display_start, dis, d_wait} display_state;
int DisplayTick(int state) {
    switch(state) {
        case display_start:
        state = d_wait;
        LCD_ClearScreen();
        break;
        case dis:
        LCD_Cursor(1);
        LCD_WriteData(keySymbol);
        LCD_Cursor(2);
        state = dis;
        break;
        case d_wait:
        LCD_Cursor(1);
        if(keySymbol=='\0') state = d_wait;
        else state = dis;
        break;
    }
    return state;
}
int main(void) {
    DDRA = 0x00; PORTA = 0xFF; //buttons for later
    DDRB = 0xFF; PORTB = 0x00; //LCD
    DDRC = 0xF0; PORTC = 0x0F; //Keypad
    DDRD = 0xFF; PORTD = 0x00; //LCD
    // LCD_init();
    // LCD_DisplayString(1,"HI.");
    
    const unsigned long GCDPeriod = 200;


    tasks[0].state = keypad_start;
    tasks[0].period = 200;
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFct = &KeypadTick;

    tasks[1].state = display_start;
    tasks[1].period = 200;
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFct = &DisplayTick;

    TimerSet(GCDPeriod);
    TimerOn();
    LCD_init();
    LCD_ClearScreen();
    while (1) {
        for(int i = 0; i < numTasks; i++) {
            if(tasks[i].elapsedTime >= tasks[i].period) {
                tasks[i].state = tasks[i].TickFct(tasks[i].state);
                tasks[i].elapsedTime = 0;
            }
            tasks[i].elapsedTime += GCDPeriod;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
