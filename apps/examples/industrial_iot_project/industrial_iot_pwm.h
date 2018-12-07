//Ambient/Babble IoT World demo 2017

#ifndef INDUSTRIAL_IOT_PWM_H
#define INDUSTRIAL_IOT_PWM_H

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

#include <shell/tash.h>
#include <tinyara/shell/tash.h>

#define CONFIG_EXAMPLES_PWM_DEVPATH "dev/pwm0"
#define CONFIG_EXAMPLES_PWM_DEVPATH2 "dev/pwm1"

struct pwm_state_s
{
  bool      initialized;
  uint32_t  channel;
  FAR char *devpath;
  uint8_t   duty;
  uint32_t  freq;
  int       duration;
};

struct pwm_info_s info;


static void pwm_devpath(FAR struct pwm_state_s *pwm, FAR const char *devpath);


char devicename[10];
int pwm_init(int, int *fd);
int pwm_generator(int, int);
int pwm_close(int);

#endif



