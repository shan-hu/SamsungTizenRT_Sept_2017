//
//  iiot_routine.c
//  industrial_iot_project
//
//  Created by Tao Ma-SSI on 4/20/17.
//  Copyright © 2017 Tao Ma-SSI. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_error.h>
#include <artik_adc.h>
#include <artik_platform.h>
#include <math.h>

#define ADC_UPDATE_PERIOD 10000 //unit: microsecond 200Hz
#define CLOUD_UPDATE_FREQ 20  //unit: times of sensor update freq 200Hz
#define SPEED_CONTROL_DURATION 100

#define VIBRATION_NORMALIZATION_FACTOR 0.115
#define VIBRATION_MAX 100

#define MAX_MESSAGE_SIZE 150
#define MAX_INT_SIZE 30

// ADC_DC <---> FITTED DC_PWM
// ----------------------------
//     6   <---> 15
//     7   <---> 16
//     8   <---> 17
//     9   <---> 18
//     10  <---> 19
//     11  <---> 20
//     12  <---> 21
//     13  <---> 22
//     14  <---> 22
//     15  <---> 23
//     16  <---> 24
//     17  <---> 25
//     18  <---> 25
//     19  <---> 26
//     20  <---> 27
//     21  <---> 27
//     22  <---> 28
//     23  <---> 29
//     24  <---> 29
//     25  <---> 30
//     26  <---> 31
//     27  <---> 31
//     28  <---> 32
//     29  <---> 32
//     30  <---> 33
#define MOTOR_DC_LIMIT 22

#define debug_p 1


#include "industrial_iot_routine.h"

int th1 = 1000;
float adc_min = 640.0;
float adc_max = 3200.0;
float pwm_dc_min = 0.0;
float pwm_dc_max = 100.0;
int windmill_control_global = 0;
int fd1;

static void *sensor_read_motor_control(void *parameter)
{
    int tmp_val = 0;
    int vib_val = 0;
    int potent_val = 0;
    int dc_value = 0;
    int tmp_result = 0;
    int vib_result = 0;

    int cnt = 0;
    int cnt2 = 0;
    int dc_val_pre = 0;
    int duration = 0;
    bool turn_windmill = 0;
    
    float a = (pwm_dc_max - pwm_dc_min)/(adc_max - adc_min);
    float b = pwm_dc_min - adc_min*a;
    float tv;

    pwm_init(0, &fd1);
    pwm_init2(1);

    // Windmill
    pwm_generator2(0);

    usleep(ADC_UPDATE_PERIOD);
    
    while(1)
    {
      if (adc_read(POTENTIOMETER_PIN, POTENT_PROCESSING, &potent_val, &dc_value) != S_OK)
      {
          printf("Failed to read potentiometer sensor over ADC. \n");
      }
      if (adc_read(TMP_SENSOR_PIN, TEMP_PROCESSING, &tmp_val, &tmp_result) != S_OK)
      {
          printf("Failed to read temperatuer sensor over ADC. \n");
      }
      if (adc_read(VIB_SENSOR_PIN, PIEZO_PROCESSING, &vib_val, &vib_result) != S_OK)
      {
          printf("Failed to read vibration sensor over ADC. \n");
      }


      tv = a*(float)potent_val + b;
      //tv = 10.0*sqrt((double)tv);  //non linear mapping for DC motors
      dc_value = (int) round(tv);

      if (dc_value < 0)
        dc_value = 0;
   

      #if (defined debug_p && debug_p)
        //printf("DC(round) = %d|", dc_value);  // --- debug --- //
        //dc_value = (potent_val - adc_min)/570 + 6;           
        printf("DC ORIG = %d \t", dc_value);  // --- debug --- //  
        fprintf(stdout, "\n");  // --- debug --- //
      #endif


      // normalize vibration data here
      int vibReport = 0;
      //vibReport = (int) round((float) vib_result * (float) VIBRATION_NORMALIZATION_FACTOR);
      vibReport = (int) round((float) vib_result * vibration_normalization_factor);

      #if (defined debug_p && debug_p)
          printf("VIB_ADC=%d ", vib_result);
          printf("VIB REP = %d \t", vibReport);
      #endif

      // record the speed for reporting  
      dc_val_pre = dc_value;

      if (vibReport > VIBRATION_MAX)
      {
        vibReport = VIBRATION_MAX;
      }

      if(vibReport >= motor_control_vib_global)  
      {
        if(dc_value > MOTOR_DC_LIMIT)
        {
          dc_value = MOTOR_DC_LIMIT;
          cnt2 = SPEED_CONTROL_DURATION;
        }

        
        #if (defined debug_p && debug_p)
            printf("[*** DC=%d, CNT2=%d]\t", dc_value, cnt2);
        #endif

        //send pwm to turn the wind mill
        turn_windmill = 1;
        duration = 0;
      }
      else
      {
        if(cnt2 > 0)  //speed overwrite lasts some duration
        {
          if(dc_value <= MOTOR_DC_LIMIT)
              --cnt2;
          else
              dc_value = MOTOR_DC_LIMIT;
        }

        #if (defined debug_p && debug_p)
            printf("[VVV DC=%d, CNT2=%d]\t", dc_value, cnt2);
        #endif

        turn_windmill = 0;
      }
	    
	    if(turn_windmill == 1)
	    {
            pwm_generator2(9);
            duration = SPEED_CONTROL_DURATION;
	    }
	    else
	    {
            if (cnt2 == 0)
		    {
                pwm_generator2(0);
                duration = SPEED_CONTROL_DURATION;
		    }
	    }

	    if(duration > 0)
    		--duration;
	

      dc_value = (int) (round(6.0*sqrt((double)dc_value)));  //non linear mapping for DC motors

      #if (defined debug_p && debug_p)    
          printf("DC CAL = %d \t", dc_value);  // --- debug --- //  
      #endif

      pwm_generator(dc_value, fd1);
 
      if(cnt == CLOUD_UPDATE_FREQ)
      {
        cnt = 0;
        //[] = "{\"speed\":0,\"temperature\":1,\"vibration\":2}";
        char message[MAX_MESSAGE_SIZE];
        char result_str[MAX_INT_SIZE];

        memset(result_str, 0, MAX_MESSAGE_SIZE);
        
        strcat(message,"{\"speed\":");
        itoa(dc_val_pre,result_str,10);
        strcat(message,result_str);
        
        strcat(message, ",\"temperature\":");
        memset(result_str, 0, MAX_INT_SIZE);
        itoa(tmp_result,result_str,10);
        strcat(message,result_str);
        
        strcat(message, ",\"vibration\":");
        memset(result_str, 0, MAX_INT_SIZE);
        itoa(vibReport,result_str,10);
        strcat(message,result_str);
        
        strcat(message,"}");
        
        printf("The message to the cloud is %s\n",message);
        SendMessageToCloud(message);
          
      }
      ++cnt;
      usleep(ADC_UPDATE_PERIOD);
    }
    return NULL;
}

int sensor_motor_thread(void)
{
    static pthread_t tid;
    pthread_attr_t attr;
    int status;
    struct sched_param sparam;
    
    pthread_attr_init(&attr);
    sparam.sched_priority = 100;
    status = pthread_attr_setschedparam(&attr, &sparam);
    status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    status = pthread_attr_setstacksize(&attr, 1024 * 8);
    if (status != OK)
    {
        printf("timedwait_test: pthread_attr_setschedparam failed, status=%d\n", status);
    }
    status = pthread_create(&tid, &attr, sensor_read_motor_control, NULL);
    if (status) {
        printf("Failed to create thread for motor-speed-control\n");
        return 0;
    }
    pthread_setname_np(tid, "sensor-read");
    pthread_join(tid, NULL);
    
    return 1;
}

int iiot_start()
{
    //create a sensor reading thread
    if (sensor_motor_thread() == 0)
    {
        printf("sensor_read_thread failed!");
        return 0;
    }
    return 1;
}


