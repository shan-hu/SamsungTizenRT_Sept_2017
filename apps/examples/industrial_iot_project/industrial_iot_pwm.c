//Ambient/Babble IoT World demo 2017

#include "industrial_iot_pwm.h"

static struct pwm_state_s g_pwmstate;

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

int pwm_init(int pwm_pin, int *fd)
{
  info.frequency = 100;
  //int ret;
  
  /* Initialize the state data */

  if (!g_pwmstate.initialized)
    {
      g_pwmstate.channel     = pwm_pin;
      g_pwmstate.freq        = 60;
      g_pwmstate.duration    = 5;
      g_pwmstate.duty        = 0;
      g_pwmstate.initialized = true;
    }

  /* Has a device been assigned? */

  if (!g_pwmstate.devpath)
    {
      /* No.. use the default device */
      if(pwm_pin == 0)
          pwm_devpath(&g_pwmstate, CONFIG_EXAMPLES_PWM_DEVPATH);
      else if(pwm_pin == 1)
	  pwm_devpath(&g_pwmstate, CONFIG_EXAMPLES_PWM_DEVPATH2);
    }

  //if(g_pwmstate.channel != 0)
    //{
      snprintf(devicename, 10, "/dev/pwm%d",g_pwmstate.channel);  
      pwm_devpath(&g_pwmstate, devicename);      
    //}  
    int ret = 0; 
    *fd = open(g_pwmstate.devpath, O_RDONLY);
    if (*fd < 0)
    {
        ret = s5j_board_pwm_setup(g_pwmstate.channel, devicename);
        if (ret != OK)
        {
          printf("pwm_main: boardctl failed: %d\n", errno);
          fflush(stdout);
          return ERROR;
        }

        *fd = open(g_pwmstate.devpath, O_RDONLY);
        if (*fd < 0)
        {
          printf("pwm_main: open %s failed: %d\n", g_pwmstate.devpath, errno);
          fflush(stdout);
          return ERROR;
        }          
    } 
    return 1;
}

int pwm_generator(int dc, int fd)
{
    int ret = 0;
    
    ret = ioctl(fd, PWMIOC_STOP, 0);
    if (ret < 0)
    {
        printf("pwm_main: ioctl(PWMIOC_STOP) failed: %d\n", errno);
        close(fd);
    }
      //int tmp = 7;
      info.frequency = 60;
      info.duty = (ub16_t)dc;  //(uint16_t)dc;   //
      //printf("Change motor speed, duty cycle = %x. \n", info.duty);
      ret = ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
      if (ret < 0)
      {
        printf("pwm_main: ioctl(PWMIOC_SETCHARACTERISTICS) failed: %d\n", errno);
        close(fd);
      }
      ret = ioctl(fd, PWMIOC_START, 0);
      if (ret < 0)
      {
        printf("pwm_main: ioctl(PWMIOC_START) failed: %d\n", errno);
        close(fd);
      }

    return OK;
}
int pwm_close(int fd)
{
    close(fd);
    fflush(stdout);
    return 1;
}

///////////////////////////////////////////////////////////////////////

struct pwm_state_s2
{
  bool      initialized;
  uint32_t  channel;
  FAR char *devpath;
  uint8_t   duty;
  uint32_t  freq;
  int       duration;
};

static struct pwm_state_s2 g_pwmstate2;

static void pwm_devpath2(FAR struct pwm_state_s2 *pwm, FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (pwm->devpath)
    {
      free(pwm->devpath);
    }

  /* Then set-up the new device path by copying the string */

  pwm->devpath = strdup(devpath);
}


static int fd2;
struct pwm_info_s info2;
char devicename2[10];
int pwm_init2(int pwm_pin)
{
  info2.frequency = 60; //100;
  //int ret;
  
  /* Initialize the state data */

  if (!g_pwmstate2.initialized)
    {
      g_pwmstate2.channel     = pwm_pin;
      g_pwmstate2.freq        = 60;//1000;
      g_pwmstate2.duration    = 5;
      g_pwmstate2.duty        = 0;
      g_pwmstate2.initialized = true;
    }

  /* Has a device been assigned? */

  if (!g_pwmstate2.devpath)
    {
      /* No.. use the default device */

      pwm_devpath2(&g_pwmstate2, CONFIG_EXAMPLES_PWM_DEVPATH2);
    }

  //if(g_pwmstate.channel != 0)
    //{
      snprintf(devicename2, 10, "/dev/pwm%d",g_pwmstate2.channel);  
      pwm_devpath2(&g_pwmstate2, devicename2);      
    //}  
    int ret = 0; 
    fd2 = open(g_pwmstate2.devpath, O_RDONLY);
    if (fd2 < 0)
    {
        ret = s5j_board_pwm_setup(g_pwmstate2.channel, devicename2);
        if (ret != OK)
        {
          printf("pwm_main: boardctl failed: %d\n", errno);
          fflush(stdout);
          return ERROR;
        }

        fd2 = open(g_pwmstate2.devpath, O_RDONLY);
        if (fd2 < 0)
        {
          printf("pwm_main: open %s failed: %d\n", g_pwmstate2.devpath, errno);
          fflush(stdout);
          return ERROR;
        }          
    }
    pwm_generator2(0);
    return 1;
}

int pwm_generator2(int dc)
{
    int ret = 0;
    
    ret = ioctl(fd2, PWMIOC_STOP, 0);
    if (ret < 0)
    {
        printf("pwm_main: ioctl(PWMIOC_STOP) failed: %d\n", errno);
        close(fd2);
    }
      //int tmp = 7;
      info2.frequency = 60;
      info2.duty = (ub16_t)dc;  //(uint16_t)dc;   //
      //printf("Change motor speed, duty cycle = %x. \n", info.duty);
      ret = ioctl(fd2, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info2));
      if (ret < 0)
      {
        printf("pwm_main: ioctl(PWMIOC_SETCHARACTERISTICS) failed: %d\n", errno);
        close(fd2);
      }
      ret = ioctl(fd2, PWMIOC_START, 0);
      if (ret < 0)
      {
        printf("pwm_main: ioctl(PWMIOC_START) failed: %d\n", errno);
        close(fd2);
      }

    return OK;
}
int pwm_close2()
{
    close(fd2);
    fflush(stdout);
    return 1;
}




