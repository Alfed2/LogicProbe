

/* Includes ------------------------------------------------------------------*/
#include "data_processing.h"
#include "adc_controlling.h"
#include "generator_timer.h"
#include "menu_controlling.h"

/* Private typedef -----------------------------------------------------------*/
// Hz
#define DATA_PROC_LOGIC_PROBE_SAMPLE_RATE       10000

// Number of ADC points in "GENERATOR_TIMER" period
#define DATA_PROC_LOGIC_PROBE_SAMPLE_PERIOD     (DATA_PROC_LOGIC_PROBE_SAMPLE_RATE / GENERATOR_TIMER_FREQ)

// Half of "DATA_PROC_LOGIC_PROBE_SAMPLE_PERIOD" - time of same logic level
#define DATA_PROC_LOGIC_PROBE_SAMPLE_HPERIOD    (DATA_PROC_LOGIC_PROBE_SAMPLE_PERIOD / 2)

//Number of HPERIODS in all captured data
#define DATA_PROC_LOGIC_PROBE_HPERIODS_NUM      (MAIN_ADC_CAPTURED_POINTS / DATA_PROC_LOGIC_PROBE_SAMPLE_HPERIOD)

// Number of sampled points to skip
#define DATA_PROC_LOGIC_PROBE_START_OFFSET      (4)

//ADC1 points
#define DATA_PROC_LOGIC_PROBE_BIG_DIFF_THRESHOLD 250

//ADC1 points
#define DATA_PROC_LOGIC_PROBE_LOW_DIFF_THRESHOLD 10

#define DATA_PROC_LOGIC_PROBE_HIGH_STATE_THRESHOLD 260 //~2V
#define DATA_PROC_LOGIC_PROBE_LOW_STATE_THRESHOLD  130 //~1V

#define DATA_PROC_LOGIC_PROBE_ANALYSE_LENGTH    \
 (DATA_PROC_LOGIC_PROBE_SAMPLE_HPERIOD - DATA_PROC_LOGIC_PROBE_START_OFFSET * 2) //start and end


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
data_processing_state_t data_processing_state = PROCESSING_IDLE;

//Hz
uint32_t current_sample_rate = 0;

// Odd items - high state of "generator timer", even - low state
uint16_t logic_probe_results[DATA_PROC_LOGIC_PROBE_HPERIODS_NUM];

// Last input state, detected by logic probe
signal_state_t logic_probe_signal_state;

extern menu_mode_t main_menu_mode;
extern volatile cap_status_type adc_capture_status;
extern volatile uint16_t adc_raw_buffer0[ADC_BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/
void data_processing_logic_probe_handler(void);
void data_processing_process_logic_probe_data(void);
void data_processing_correct_raw_data(uint16_t zero_offset);
uint16_t data_processing_get_adc_offset(void);
uint16_t data_processing_calc_adc_average(uint16_t* adc_buffer, uint16_t length);

/* Private functions ---------------------------------------------------------*/

// Remove sampling offset from ADC1 results
void data_processing_correct_raw_data(uint16_t zero_offset)
{
  float a_coef = (float)zero_offset / (float)(MAIN_ADC_HALF_VALUE - zero_offset);
  float b_coef = -a_coef * (float)MAIN_ADC_HALF_VALUE;
  
  for (uint16_t i = 0; i < MAIN_ADC_CAPTURED_POINTS; i++)
  {
    uint16_t raw_value = adc_raw_buffer0[i*2];
    float tmp_value = a_coef * raw_value + b_coef;//correction offset
    tmp_value = (float)raw_value + tmp_value;
    if (tmp_value < 0.0f)
      tmp_value = 0.0;
    adc_raw_buffer0[i*2] = (uint16_t)tmp_value;
  }
}

//Return ADC1 zero offset in ADC points for current sample rate
uint16_t data_processing_get_adc_offset(void)
{
  if (current_sample_rate == DATA_PROC_LOGIC_PROBE_SAMPLE_RATE)
    return 11;//todo
  
  return 0xFFFF;//error
}


// This function must be called when "main_menu_mode" is changed
// Switch capture mode
void data_processing_main_mode_changed(void)
{
  data_processing_state = PROCESSING_IDLE;
  if (main_menu_mode == MENU_MODE_LOGIC_PROBE)
  {
    adc_set_sample_rate(DATA_PROC_LOGIC_PROBE_SAMPLE_RATE);
    current_sample_rate = DATA_PROC_LOGIC_PROBE_SAMPLE_RATE;
    generator_timer_activate_gpio();
  }
  else
  {
    generator_timer_deactivate_gpio();
  }
}

// PControlling data sampling and processing
void data_processing_handler(void)
{
  switch (main_menu_mode)
  {
    case MENU_MODE_LOGIC_PROBE:
      data_processing_logic_probe_handler();
    break;
    
    default: break;
  }
}

void data_processing_start_new_capture(void)
{
  data_processing_state = PROCESSING_IDLE;
}

// Data sampling and processing for "logic probe" mode
void data_processing_logic_probe_handler(void)
{
  if (data_processing_state == PROCESSING_IDLE)
  {
    generator_timer_start();
    adc_capture_start();
    data_processing_state = PROCESSING_CAPTURE_RUNNING;
  }
  else if (data_processing_state == PROCESSING_CAPTURE_RUNNING)
  {
    if (adc_capture_status == CAPTURE_DONE)
    {
      data_processing_state = PROCESSING_DATA;
      data_processing_correct_raw_data(data_processing_get_adc_offset());
      data_processing_process_logic_probe_data();
      data_processing_state = PROCESSING_DATA_DONE;
    }
  }
}

void data_processing_process_logic_probe_data(void)
{
  uint8_t i;

  for (i = 0; i < DATA_PROC_LOGIC_PROBE_HPERIODS_NUM; i++)
  {
    uint16_t start = (DATA_PROC_LOGIC_PROBE_SAMPLE_HPERIOD * i + DATA_PROC_LOGIC_PROBE_START_OFFSET) * 2;//adc1
    logic_probe_results[i] = data_processing_calc_adc_average(
      (uint16_t*)&adc_raw_buffer0[start], DATA_PROC_LOGIC_PROBE_ANALYSE_LENGTH);
  }
  
  int16_t diff_results[DATA_PROC_LOGIC_PROBE_HPERIODS_NUM / 2];
  uint8_t big_diff_cnt = 0;
  uint8_t low_diff_cnt = 0;
  
  for (i = 0; i < DATA_PROC_LOGIC_PROBE_HPERIODS_NUM; i+= 2)
  {
    diff_results[i/2] = logic_probe_results[i] - logic_probe_results[i + 1];
    
    if (diff_results[i/2] > DATA_PROC_LOGIC_PROBE_BIG_DIFF_THRESHOLD)
      big_diff_cnt++;
    
    if (diff_results[i/2] < DATA_PROC_LOGIC_PROBE_LOW_DIFF_THRESHOLD)
      low_diff_cnt++;
  }
  
  if (big_diff_cnt == (DATA_PROC_LOGIC_PROBE_HPERIODS_NUM / 2))
    logic_probe_signal_state = SIGNAL_TYPE_Z_STATE;
  else if (low_diff_cnt == (DATA_PROC_LOGIC_PROBE_HPERIODS_NUM / 2))
  {
    //Signal is stable all the time - mean strong external signal
    if (logic_probe_results[0] > DATA_PROC_LOGIC_PROBE_HIGH_STATE_THRESHOLD)
      logic_probe_signal_state = SIGNAL_TYPE_HIGH_STATE;
    else if (logic_probe_results[0] < DATA_PROC_LOGIC_PROBE_LOW_STATE_THRESHOLD)
      logic_probe_signal_state = SIGNAL_TYPE_LOW_STATE;
    else
      logic_probe_signal_state = SIGNAL_TYPE_UNKOWN_STATE;
  }
  else
  {
    logic_probe_signal_state = SIGNAL_TYPE_PULSED_STATE;
  }
}

// Calculate average value from RAW adc data
uint16_t data_processing_calc_adc_average(uint16_t* adc_buffer, uint16_t length)
{
  uint16_t i;
  if (length == 0)
    return 0;
  
  uint32_t summ = 0;
  for (i = 0; i < (length * 2); i+= 2)
  {
    summ+= adc_buffer[i];// data from two ADC's alternates
  }
  return (uint16_t)(summ / length);
}