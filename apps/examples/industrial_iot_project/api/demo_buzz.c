/****************************************************************************
 * examples/demo_buzz/demo_buzz.c
 *
 *   Copyright (C) 2011-2012, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>
//#include <sys/boardctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>

#include <tinyara/pwm.h>

#include "buzz.h"

#include <shell/tash.h>
#include <tinyara/shell/tash.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pwm_state_s
{
  bool      initialized;
  uint32_t  channel;
  FAR char *devpath;
  uint8_t   duty;
  uint32_t  freq;
  int       duration;
};

struct test_case_s
{
  uint8_t   duty;
  uint32_t  freq;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct pwm_state_s g_pwmstate;

static struct test_case_s g_pwmtest[]=
{
  {0, 1000},
  {20, 1000},  
  {40, 1000},    
  {80, 1000},      
  {100, 1000},
  
  {10, 5000},
  {30, 5000},  
  {50, 5000},    
  {70, 5000},      
  {90, 5000},
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_devpath
 ****************************************************************************/

static void pwm_devpath(FAR struct pwm_state_s *pwm, FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (pwm->devpath)
    {
      free(pwm->devpath);
    }

  /* Then set-up the new device path by copying the string */

  pwm->devpath = strdup(devpath);
}

/****************************************************************************
 * Name: pwm_help
 ****************************************************************************/

static void buzz_help(FAR struct pwm_state_s *pwm)
{

  printf("Usage: pwm [OPTIONS]\n");
  printf("\nArguments are \"sticky\".  For example, once the PWM frequency is\n");
  printf("specified, that frequency will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-p devpath] selects the PWM device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_PWM_DEVPATH, pwm->devpath ? pwm->devpath : "NONE");
  printf("  [-f frequency] selects the pulse frequency.  "
         "Default: %d Hz Current: %u Hz\n",
         CONFIG_EXAMPLES_PWM_FREQUENCY, pwm->freq);

  printf("  [[-c channel1] [[-c channel2] ...]] selects the channel number for each channel.  ");
  printf("Default: 0, Current: %d \n",pwm->channel);         

  printf("  [-d duty] selects the pulse duty as a percentage.  "
         "Default: %d %% Current: %d %%\n",
         CONFIG_EXAMPLES_PWM_DUTYPCT, pwm->duty);

  printf("  [-t duration] is the duration of the pulse train in useconds.  "
         "Default: %d Current: %d\n",
         CONFIG_EXAMPLES_PWM_DURATION, pwm->duration);
  printf("  [-h] shows this message and exits\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct pwm_state_s *pwm, int argc, FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long value;
  int index;
  int nargs;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'f':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("Frequency out of range: %ld\n", value);
                exit(1);
              }

            pwm->freq = (uint32_t)value;
            index += nargs;
            break;

          case 'c':
            nargs = arg_decimal(&argv[index], &value);
            if (value > 10)
              {
                printf("Channel out of range: %ld\n", value);
                exit(1);
              }

            pwm->channel = value;
            index += nargs;            
            break;

          case 'd':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > 99)
              {
                printf("Duty out of range: %ld\n", value);
                exit(1);
              }


            pwm->duty = (uint8_t)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            pwm_devpath(pwm, str);
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1) // || value > INT_MAX)
              {
                printf("Duration out of range: %ld\n", value);
                exit(1);
              }

            pwm->duration = (int)value;
            index += nargs;
            break;

          case 'h':
            buzz_help(pwm);
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            buzz_help(pwm);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int demo_buzz_test(int argc, char *argv[])
#endif
{
  struct pwm_info_s info;
  int fd;
  int testcount;
  int ret;
  char devicename[10];
  
  /* Initialize the state data */

  if (!g_pwmstate.initialized)
    {
      g_pwmstate.channel     = 0;
      g_pwmstate.freq        = CONFIG_EXAMPLES_PWM_FREQUENCY;
      g_pwmstate.duration    = CONFIG_EXAMPLES_PWM_DURATION;
      g_pwmstate.duty        = CONFIG_EXAMPLES_PWM_DUTYPCT;      
      g_pwmstate.initialized = true;
    }

  /* Has a device been assigned? */

  if (!g_pwmstate.devpath)
    {
      /* No.. use the default device */

      pwm_devpath(&g_pwmstate, CONFIG_EXAMPLES_PWM_DEVPATH);
    }

  /* Initialization of the PWM hardware is performed by logic external to
   * this test.
   */

  /* Parse the command line */
  parse_args(&g_pwmstate, argc, argv);

  if(g_pwmstate.channel != 0)
    {
      snprintf(devicename, 10, "/dev/pwm%d",g_pwmstate.channel);  
      pwm_devpath(&g_pwmstate, devicename);      
    }   
    
  /* Open the PWM device for reading */

  fd = open(g_pwmstate.devpath, O_RDONLY);
  if (fd < 0)
    {
      //ret = boardctl(BOARDIOC_PWMTEST_SETUP, 0);
      ret = s5j_board_pwm_setup(g_pwmstate.channel, devicename);
      if (ret != OK)
        {
          printf("pwm_main: boardctl failed: %d\n", errno);
          goto errout;
        }
        
      /* Open the PWM device for reading */

      fd = open(g_pwmstate.devpath, O_RDONLY);
      if (fd < 0)
        {
          printf("pwm_main: open %s failed: %d\n", g_pwmstate.devpath, errno);
          goto errout;
        }          
    }

  /* Configure the characteristics of the pulse train */
  for (testcount=0; testcount<10; testcount++)
    {
      info.frequency = g_pwmtest[testcount].freq;
      info.duty      = (uint16_t)g_pwmtest[testcount].duty;
      printf("Test case %d: channel[%d] frequency[%u hz] duty[%d%%] \n",
             testcount, g_pwmstate.channel, g_pwmtest[testcount].freq, g_pwmtest[testcount].duty);

      ret = ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
      if (ret < 0)
        {
          printf("pwm_main: ioctl(PWMIOC_SETCHARACTERISTICS) failed: %d\n", errno);
          goto errout_with_dev;
        }

      /* Then start the pulse train.  Since the driver was opened in blocking
       * mode, this call will block if the count value is greater than zero.
       */
      ret = ioctl(fd, PWMIOC_START, 0);
      if (ret < 0)
        {
          printf("pwm_main: ioctl(PWMIOC_START) failed: %d\n", errno);
          goto errout_with_dev;
        }

      /* It a non-zero count was not specified, then wait for the selected
       * duration, then stop the PWM output.
       */

      /* Wait for the specified duration */
#if 1 
  usleep(1000000); //temperarily for test purpose
#else 
  sleep(g_pwmstate.duration);
#endif	
   	
      /* Then stop the pulse train */
    	
      printf("pwm_main: stopping output\n");
    	
    	ret = ioctl(fd, PWMIOC_STOP, 0);
    	if (ret < 0)
    	  {
    	    printf("pwm_main: ioctl(PWMIOC_STOP) failed: %d\n", errno);
    	    goto errout_with_dev;
    	  }

	  }

  close(fd);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd);
errout:
  fflush(stdout);
  return ERROR;
}


