/*
 * UART.cc
 *
 *  Created on: Dec 10, 2009
 *      Author: phooky
 */
#include "util/UART.hh"
#include <Platform.hh>
#include <stdint.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "util/DebugPin.hh"

// MEGA168_DOUBLE_SPEED_MODE is 1 if USXn is 1.
#ifndef MEGA168_DOUBLE_SPEED_MODE
#define MEGA168_DOUBLE_SPEED_MODE 0
#endif

#if MEGA168_DOUBLE_SPEED_MODE
#define UBRR_VALUE 51
#define UCSR0A_VALUE _BV(U2X0)
#else
#define UBRR_VALUE 25
#define UCSR0A_VALUE 0
#endif


/// Adapted from ancient arduino/wiring rabbit hole
#define INIT_SERIAL(uart_) \
{ \
    UBRR0H = UBRR_VALUE >> 8; \
    UBRR0L = UBRR_VALUE & 0xff; \
    \
    /* set config for uart, explicitly clear TX interrupt flag */ \
    UCSR0A = UCSR0A_VALUE | _BV(TXC0); \
    UCSR0B = _BV(RXEN0) | _BV(TXEN0); \
    UCSR0C = _BV(UCSZ01)|_BV(UCSZ00); \
    /* defaults to 8-bit, no parity, 1 stop bit */ \
}

UART uart[UART_COUNT] = {
		UART(0)
};


// Unlike the old implementation, we go half-duplex: we don't listen while sending.
inline void listen() {
	PORTD &= ~_BV(4);
	PORTC &= ~_BV(2);
}

inline void speak() {
	PORTD |= _BV(4);
	PORTC |= _BV(2);
}

UART::UART(uint8_t index) : index_(index), enabled_(false) {
	INIT_SERIAL(0);
	DDRD |= _BV(4);
	DDRC |= _BV(2);
	listen();
}

/// Subsequent bytes will be triggered by the tx complete interrupt.
void UART::beginSend() {
	if (!enabled_) { return; }
	uint8_t send_byte = out_.getNextByteToSend();
	speak();
	UDR0 = send_byte;
}

void UART::enable(bool enabled) {
	enabled_ = enabled;
	if (enabled) {
		UCSR0B |=  _BV(RXCIE0) | _BV(TXCIE0);
	} else {
		UCSR0B &= ~(_BV(RXCIE0) | _BV(TXCIE0));
	}
}

// Send and receive interrupts
ISR(USART_RX_vect)
{
	uart[0].in_.processByte( UDR0 );
}

ISR(USART_TX_vect)
{
	if (uart[0].out_.isSending()) {
		UDR0 = uart[0].out_.getNextByteToSend();
	} else {
		listen();
	}
}
