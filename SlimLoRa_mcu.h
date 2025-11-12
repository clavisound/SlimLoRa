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

#elif defined(ESP32)
// =============================================================================
// ESP32 (Xtensa/RISC-V with FreeRTOS) Implementation
// =============================================================================
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h> // For portMUX_TYPE

// Define ATOMIC_RESTORESTATE for ESP32 to match other architectures
#define ATOMIC_RESTORESTATE

// Declare a spinlock for protecting shared resources.
// This needs to be initialized when the module is loaded or globally.
// For simplicity here, we assume it's initialized elsewhere or globally.
// A common pattern is to declare it as static and initialize it.
// For this macro definition, we'll assume 'mySpinlock' is available in scope.
// If not, a global or static declaration would be needed in the .cpp file.

/**
 * @brief Macro to create a critical section on ESP32 using FreeRTOS spinlocks.
 *
 * This macro ensures that a block of code is executed atomically with respect
 * to other tasks and ISRs on both cores of the ESP32. It uses FreeRTOS
 * critical section primitives.
 *
 * Usage:
 * static portMUX_TYPE mySpinlock = portMUX_INITIALIZER_UNLOCKED; // Declare and initialize spinlock
 * ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 *   // Your critical code here, accessing shared resources protected by mySpinlock
 * }
 */
#define ATOMIC_BLOCK(type) \
    static portMUX_TYPE _slimlora_spinlock = portMUX_INITIALIZER_UNLOCKED; \
    for ( uint8_t _ToDo = 1; _ToDo ; _ToDo = 0 ) \
        for ( taskENTER_CRITICAL(&_slimlora_spinlock), _ToDo = (_ToDo == 1) ; \
              _ToDo ; taskEXIT_CRITICAL(&_slimlora_spinlock), _ToDo = (_ToDo == 0) )

#else
// =============================================================================
// Unsupported Architecture
// =============================================================================
#error "SlimLoRa: This architecture is not supported for atomic operations."

#endif

#endif // SLIM_LORA_ATOMIC_H
