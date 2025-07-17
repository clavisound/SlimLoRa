#ifndef SLIM_LORA_ATOMIC_H
#define SLIM_LORA_ATOMIC_H

// This header provides a cross-platform implementation for the ATOMIC_BLOCK macro,
// which is used to create critical sections where interrupts are disabled.

#if defined(__AVR__)
// =============================================================================
// AVR (ATmega) Implementation
// =============================================================================
#include <util/atomic.h>

#elif defined(ARDUINO_ARCH_SAMD)
// =============================================================================
// ARM (ATSAMD21) Implementation
// =============================================================================
#include <Arduino.h> // For __get_PRIMASK, __set_PRIMASK, __disable_irq

// These are placeholders to make the code that uses them compile.
// The ARM implementation always uses a "restore state" approach.
#define ATOMIC_RESTORESTATE
#define ATOMIC_FORCEON

/**
 * @brief Macro to create a critical section on ARM Cortex-M microcontrollers.
 *
 * This macro emulates the behavior of AVR's ATOMIC_BLOCK. It saves the global
 * interrupt mask state (PRIMASK), disables interrupts, executes the code
 * within the block, and then restores the original interrupt state.
 *
 * It is implemented using a for-loop trick to manage the scope of the
 * primask_state variable and ensure cleanup code is always executed.
 *
 * Usage:
 * ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 *   // Your critical code here
 * }
 */
#define ATOMIC_BLOCK(type) \
    for ( uint32_t primask_state = __get_PRIMASK(), __ToDo = 1; \
          __ToDo ? (__disable_irq(), 1) : 0; \
          __set_PRIMASK(primask_state), __ToDo = 0 )

#else
// =============================================================================
// Unsupported Architecture
// =============================================================================
#error "SlimLoRa: This architecture is not supported for atomic operations."

#endif

#endif // SLIM_LORA_ATOMIC_H
