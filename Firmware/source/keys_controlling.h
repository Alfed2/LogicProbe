/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __KEYS_CONTROLLING_H
#define __KEYS_CONTROLLING_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x.h"
#include "config.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  KEY_RELEASED = 0,
  KEY_PRESSED_WAIT,
  KEY_PRESSED,
  KEY_WAIT_FOR_RELEASE,
  KEY_HOLD,
} key_state_t;

typedef struct
{
  // GPIO_Pin_x
  uint16_t pin_name;
  
  //GPIOx
  GPIO_TypeDef* gpio_name;
  
  uint8_t state_prev;
  uint8_t state;
  
  uint8_t change_pin_pull;

} charge_t;


typedef struct
{
  // GPIO_Pin_x
  uint16_t pin_name;
  
  //GPIOx
  GPIO_TypeDef* gpio_name;
  
  // Current key state
  uint8_t current_state;
  
  //Processed state
  key_state_t state;
  
  //Previous state
  key_state_t prev_state;
  
  //Timestamp of pressed or released time
  uint32_t key_timestamp;
  
} key_item_t;


/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void keys_init(void);
void key_handling(void);

void keys_functons_init_hardware(key_item_t* key_item);
void keys_functons_update_key_state(key_item_t* key_item);
void charge_functons_init_hardware(key_item_t* key_item);
/*void init_charge_functons_status(charge_t* charge);*/
void charge_functons_status(charge_t* charge);

#endif /* __KEYS_CONTROLLING_H */
