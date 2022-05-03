/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x.h"

/* Exported types ------------------------------------------------------------*/
extern volatile uint32_t ms_tick;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define START_TIMER(x, duration)  (x = (ms_tick + duration))
#define TIMER_ELAPSED(x)  ((ms_tick > x) ? 1 : 0)

#if defined ( __ICCARM__ ) // IAR
    #define ENTER_CRITICAL(x)       x=__get_interrupt_state(); __disable_interrupt()
    #define LEAVE_CRITICAL(x)       __set_interrupt_state(x)

#elif defined (__CC_ARM) // KEIL
    #define ENTER_CRITICAL(x)       x=__disable_irq()
    #define LEAVE_CRITICAL(x)       if (!x) __enable_irq()
    #define NO_INIT                 __attribute__((zero_init))
#else
  #error ERROR
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */
