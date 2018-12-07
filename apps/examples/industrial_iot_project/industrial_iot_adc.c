
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <artik_module.h>
#include <artik_error.h>
#include <artik_adc.h>
#include <artik_gpio.h>
#include <artik_platform.h>

#include "industrial_iot_adc.h"

// ADC
static artik_adc_module *adcSensor = NULL;
static artik_adc_handle adcHandle = NULL;
static artik_adc_config adcConfig;
static artik_error adcSensorErr = S_OK;

// GPIO
static artik_gpio_module *gpio = NULL;
static artik_gpio_config gpioConfig;
static artik_gpio_handle gpioHandle = NULL;

// Pizeo ADC processing
static int prevADC = 0;

static int delta = 0;
static float filteredADC = 0;

float buffer[WINDOW_SIZE];
static int winIndex = 0;


// ADC reading
static int sampleIndex = 0;

// LED status
static bool tmpLEDStatus = 0;
static bool vibLEDStatus = 0;

float MIN_ADC = 650.0;


/***************************************
 Turn ON/OFF the LED on the dev board
 ****************************************/
int setLED(int pin, bool state)
{
    char name[16] = "";
    int ret = 0;
    
    gpio = (artik_gpio_module *)artik_request_api_module("gpio");
    if (!gpio) {
        fprintf(stderr, "GPIO module is not available\n");
        return -1;
    }
    
    memset(&gpioConfig, 0, sizeof(gpioConfig));
    gpioConfig.dir = GPIO_OUT;
    gpioConfig.id = pin;
    snprintf(name, 16, "gpio%d", gpioConfig.id);
    gpioConfig.name = name;
    
    if (gpio->request(&gpioHandle, &gpioConfig) != S_OK) {
        fprintf(stderr, "Failed to request GPIO %d\n", gpioConfig.id);
        ret = -1;
        goto exit;
    }
    
    //printf("in setLED: pin=%d,state=%d\n", pin, state);
    gpio->write(gpioHandle, state ? 1 : 0);
    gpio->release(gpioHandle);
    
exit:
    artik_release_api_module(gpio);
    return ret;
}


/***************************************
 Process Piezo ADC DATA
 ****************************************/
int piezoProcessing(int val)
{
    int vibration = 0;

    delta = abs(val - prevADC);
    buffer[winIndex] = (float) delta / WINDOW_SIZE;
    prevADC = val;

    #if (defined DEBUG_PRINT && DEBUG_PRINT)
      printf("delta[%d]=%d\t", sampleIndex, delta);  // --- debug --- //
    #endif
    
    filteredADC = 0;
    
    if (sampleIndex >= WINDOW_SIZE)
    {
        int i;
        for (i = 0; i < WINDOW_SIZE; i++)
        {
            filteredADC += buffer[i];
        }
    }
    winIndex = (winIndex + 1) % WINDOW_SIZE;
    
    vibration = (int) round(filteredADC);
    
    #if (defined DEBUG_PRINT && DEBUG_PRINT)
      char str[10];   // --- debug --- //
      sprintf(str, "%.4g", filteredADC);  // --- debug --- //
      printf("VIB_ADC=%s|%d\t", str, vibration);
    #endif
    
    // Set the LED on dev board
    if ((vibration > VIB_THRESHOLD) && !vibLEDStatus)
    {
        //fprintf(stdout, "VIB Warning!");
        //setLED(LED_PIN_8, 1);
        setLED(LED_702, 1); // the BLUE LED
        vibLEDStatus = 1;
    }
    
    // Set the LED on dev board
    if ((vibration < (VIB_THRESHOLD - 20)) && vibLEDStatus)
    {
        //fprintf(stdout, "VIB OK!");
        //setLED(LED_PIN_8, 0);
        setLED(LED_702, 0); // the BLUE LED
        vibLEDStatus = 0;
    }

    return vibration;
}


/**********************************************************
 Convert TMP36 ADC data to temperature in Celsius
 The reference voltage is 3.3 Volt
 The ADC is 12 bit
 **********************************************************/
int convertADCtoCelsius(int adc)
{
    float celsius = (((float)adc* 3.3 / 4095) - 0.5) * 100;
    
    float fahrenheit = (celsius * 9.0 / 5.0) + 32.0;
    
    int temp = (int) round(fahrenheit);
    
#if (defined DEBUG_PRINT && DEBUG_PRINT)
    char str[10];   // --- debug --- //
    sprintf(str, "%.4g", fahrenheit);  // --- debug --- //
    printf("TMP_F=%s|%d\t", str, temp);  // --- debug --- //
#endif
    
    return temp;
}


/***************************************
 Process TMP36 ADC DATA
 ****************************************/
int tempProcessing(int val)
{
    int temperature = convertADCtoCelsius(val);
    
    //printf("in tempProcessing: val=%d, status=%d\n", val, tmpLEDStatus);
    if ((temperature >= TEMP_THRESHOULD) && !tmpLEDStatus)
    {
        //printf("val=%d, status=%d\n", val, tmpLEDStatus); // --- debug --- //
        //fprintf(stdout, "TMP Warning!"); // --- debug --- //
        //setLED(LED_PIN_7, 1);
        setLED(LED_703, 1);  // the RED LED
        
        tmpLEDStatus = 1;
    }
    
    if ((temperature < (TEMP_THRESHOULD - 5)) && tmpLEDStatus)
    {
        //fprintf(stdout, "TMP OK!"); // --- debug --- //
        //setLED(LED_PIN_7, 0);
        setLED(LED_703, 0);  // the RED LED
        tmpLEDStatus = 0;
    }
    
    //float tempC = convertADCtoCelsius(val);
    return temperature;
}


/***************************************
 potentiometerProcessing
 ****************************************/
int potentiometerProcessing(int val)
{
    //fprintf(stdout, "Potentiometer OK!");
    //adc_val_global[POTENT_PROCESSING] = val;
    float tempDC = ((float) val - MIN_ADC)/570 + 6;
    int dc_value = (int) round(tempDC);
    
#if (defined DEBUG_PRINT && DEBUG_PRINT)
    char str[10];   // --- debug --- //
    sprintf(str, "%.4g", tempDC);  // --- debug --- //
    printf("DC=%s|%d\t", str, dc_value);  // --- debug --- //
#endif
    
    return dc_value;
}


/***************************************
 READ RAW ADC DATA
 ****************************************/
int adc_read(int pin, int option, int *val, int *result)
{
    adcSensorErr = S_OK;
    char name[16] = "";
    int pin_num = pin;
    
    adcSensor = (artik_adc_module *)artik_request_api_module("adc");
    if (!adcSensor) {
        fprintf(stderr, "ADC module is not available\n");
        return -1;
    }
    
    memset(&adcConfig, 0, sizeof(adcConfig));
    adcConfig.pin_num = pin_num;
    snprintf(name, 16, "adc%d", adcConfig.pin_num);
    adcConfig.name = name;
    
    adcSensorErr = adcSensor->request(&adcHandle, &adcConfig);
    if (adcSensorErr != S_OK) {
        fprintf(stderr, "Failed to request ADC %d (%d)\n", adcConfig.pin_num, adcSensorErr);
        artik_release_api_module(adcSensor);
    }
    
    adcSensorErr = adcSensor->get_value(adcHandle, val);
    if (adcSensorErr != S_OK) {
        fprintf(stderr, "Failed to read ADC %d value (%d)\n", adcConfig.pin_num, adcSensorErr);
        printf("err: prepare to release ADC handle\n");
        adcSensor->release(adcHandle);
        artik_release_api_module(adcSensor);
        return adcSensorErr;
    }
#if (defined DEBUG_PRINT && DEBUG_PRINT)
    fprintf(stdout, "ADC[%d][%d]=%d\t", adcConfig.pin_num, sampleIndex, *val);
#endif
    
    adcSensor->release(adcHandle);
    
    switch (option)
    {
        case PIEZO_PROCESSING:
            *result = piezoProcessing(*val);
            sampleIndex++;
            break;
        case TEMP_PROCESSING:
            *result = tempProcessing(*val);
            break;
        case POTENT_PROCESSING:
            //*result = potentiometerProcessing(*val);
            break;
            
        default:
            break;
            
    }
    
    return adcSensorErr;
}


int adc_close()
{
    adcSensor->release(adcHandle);
    artik_release_api_module(adcSensor);
    return S_OK;
}

