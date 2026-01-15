#if NON_BLOCKING
// SlimLoRaTimers.h
#ifndef SLIM_LORA_TIMERS_H
#define SLIM_LORA_TIMERS_H

#include <Arduino.h> // For byte, uint8_t, unsigned long
#include <avr/io.h>  // Required for AVR register and bit definitions
#include <avr/interrupt.h> // Required for ISR() macro (though often in cpp, good practice for ISR_VECT)

// --- Conditional Timer Selection Macros ---
// These macros abstract the timer-specific register names and ISR vector.
// The selection is based on predefined AVR microcontroller macros.
// Timer3 is preferred for ATmega1284P/ATmega2560 if available, otherwise Timer1.

#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega2560__)
    // For ATmega1284P, ATmega2560, etc., use Timer3
    #define LORAWAN_TIMER_NAME "Timer3"
    #define LORAWAN_TIMER_ISR_VECT TIMER3_COMPA_vect // Interrupt Service Routine Vector

    // Timer3 Control Registers
    #define LORAWAN_TCCRA TCCR3A
    #define LORAWAN_TCCRB TCCR3B
    #define LORAWAN_TCNT TCNT3   // Timer/Counter Register
    #define LORAWAN_OCRA OCR3A   // Output Compare Register A
    #define LORAWAN_TIMSK TIMSK3 // Timer Interrupt Mask Register
    #define LORAWAN_OCIEA OCIE3A // Output Compare A Match Interrupt Enable Bit

    // Prescaler bit definitions for Timer3 (CS3x bits in TCCR3B)
    #define LORAWAN_PRESCALER_DIV_1   (1 << CS30)
    #define LORAWAN_PRESCALER_DIV_8   (1 << CS31)
    #define LORAWAN_PRESCALER_DIV_64  ((1 << CS31) | (1 << CS30))
    #define LORAWAN_PRESCALER_DIV_256 (1 << CS32)
    #define LORAWAN_PRESCALER_DIV_1024 ((1 << CS32) | (1 << CS30))

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32U4__)
    // For ATmega328P, ATmega168, etc., use Timer1
    #define LORAWAN_TIMER_NAME "Timer1"
    #define LORAWAN_TIMER_ISR_VECT TIMER1_COMPA_vect // Interrupt Service Routine Vector

    // Timer1 Control Registers
    #define LORAWAN_TCCRA TCCR1A
    #define LORAWAN_TCCRB TCCR1B
    #define LORAWAN_TCNT TCNT1   // Timer/Counter Register
    #define LORAWAN_OCRA OCR1A   // Output Compare Register A
    #define LORAWAN_TIMSK TIMSK1 // Timer Interrupt Mask Register
    #define LORAWAN_OCIEA OCIE1A // Output Compare A Match Interrupt Enable Bit

    // Prescaler bit definitions for Timer1 (CS1x bits in TCCR1B)
    #define LORAWAN_PRESCALER_DIV_1   (1 << CS10)
    #define LORAWAN_PRESCALER_DIV_8   (1 << CS11)
    #define LORAWAN_PRESCALER_DIV_64  ((1 << CS11) | (1 << CS10))
    #define LORAWAN_PRESCALER_DIV_256 (1 << CS12)
    #define LORAWAN_PRESCALER_DIV_1024 ((1 << CS12) | (1 << CS10))

#else
    // Fallback or error for unsupported MCUs
    #error "Unsupported ATmega microcontroller for SlimLoRa timer configuration!"
    // Define dummy macros to prevent further compilation errors if not building for supported AVR
    #define LORAWAN_TIMER_NAME "Unknown"
    #define LORAWAN_TIMER_ISR_VECT INT_VECTOR_UNUSED
    #define LORAWAN_TCCRA 0
    #define LORAWAN_TCCRB 0
    #define LORAWAN_TCNT 0
    #define LORAWAN_OCRA 0
    #define LORAWAN_TIMSK 0
    #define LORAWAN_OCIEA 0
    #define LORAWAN_PRESCALER_DIV_1 0
    #define LORAWAN_PRESCALER_DIV_8 0
    #define LORAWAN_PRESCALER_DIV_64 0
    #define LORAWAN_PRESCALER_DIV_256 0
    #define LORAWAN_PRESCALER_DIV_1024 0
#endif

// WGM bits for CTC mode are generally consistent for 16-bit timers
// WGM12 for Timer1, WGM32 for Timer3 are usually in the same bit position (WGMx2) in TCCRxB
// The following macro will effectively set the WGMx2 bit, which is correct for CTC mode on 16-bit timers.
#define LORAWAN_WGM_CTC_MODE (1 << WGM12) 

#if defined CATCH_DIVIDER && defined (__AVR__)
extern volatile uint8_t clockShift; // Declare extern to access the global clockShift from SlimLoRa.cpp
#endif

// Declare the global counter for 1-second ticks (if still in use)
extern volatile byte loraWanRxTimerCounter;

// Flag to signal that the one-shot RX timer has triggered
extern volatile bool rxTimerTriggered;

// Initialize the selected hardware timer for 1-second periodic ticks
void initializeLoRaWAN_Timer();

// Setup the selected hardware timer for a one-shot interrupt after 'duration_micros'
void setupRxOneShotTimer(unsigned long duration_micros);

// Stop the RX one-shot timer and clear its flag
void stopRxOneShotTimer();

#endif // SLIM_LORA_TIMERS_H
#endif // NON_BLOCKING
