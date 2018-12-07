#ifndef _INDUSTRIAL_IOT_ADC_H_
#define _INDUSTRIAL_IOT_ADC_H_

#define POTENT_PROCESSING 0
#define TEMP_PROCESSING 1
#define PIEZO_PROCESSING 3
#define DO_NOTHING 4

#define POTENTIOMETER_PIN 0
#define TMP_SENSOR_PIN 1
#define VIB_SENSOR_PIN 3
#define UNUSED_PIN 2

#define TEMP_THRESHOULD 78
#define VIB_THRESHOLD 60

#define LED_702 49  // XGPIO20 (BLUE LED)
#define LED_703 45  // XGPIO16 (RED LED)
#define LED_PIN_7 48  // CON 709 pin 7
#define LED_PIN_8 50  // CON 708 pin 8

// Filter
#define WINDOW_SIZE 50

#define DEBUG_PRINT 0

int setLED(int pin, bool state);
int piezoProcessing(int val);
int tempProcessing(int val);
int potentiometerProcessing(int val);
int adc_read(int pin, int option, int *val, int *result);
int adc_close(void);


#endif
