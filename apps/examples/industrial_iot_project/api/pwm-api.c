#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_pwm.h>

static artik_pwm_handle pwm_handle = NULL;

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\tpwm start <pin num> <period> <duty cycle> [invert]\n");
    fprintf(stderr, "\tpwm stop\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int pwm_main(int argc, char *argv[])
#endif
{
	artik_pwm_module *pwm = NULL;
	int ret = 0;

	if ((argc < 2) || (strcmp(argv[1], "start") && strcmp(argv[1], "stop"))) {
	    fprintf(stderr, "Wrong arguments\n");
		usage();
		return -1;
	}

	pwm = (artik_pwm_module *)artik_request_api_module("pwm");
	if (!pwm) {
	    fprintf(stderr, "PWM module is not available\n");
	    return -1;
	}

	if (!strcmp(argv[1], "start")) {
	    artik_pwm_config config;
	    char name[16] = "";

	    if (argc < 5) {
	        fprintf(stderr, "Wrong number of arguments\n");
	        usage();
            ret = -1;
            goto exit;
	    }

	    if (pwm_handle) {
	        /* PWM was already started, stop it first */
	        pwm->disable(pwm_handle);
	        pwm->release(pwm_handle);
	        pwm_handle = NULL;
	    }

	    memset(&config, 0, sizeof(config));
	    config.pin_num = atoi(argv[2]);
	    config.period = atoi(argv[3]);
	    config.duty_cycle = atoi(argv[4]);
	    if ((argc > 5) && !strcmp(argv[5], "invert")) {
	        config.polarity = ARTIK_PWM_POLR_INVERT;
	    }
        snprintf(name, 16, "pwm%d", config.pin_num);
        config.name = name;

	    if (pwm->request(&pwm_handle, &config) != S_OK) {
	        fprintf(stderr, "Failed to request PWM %d\n", config.pin_num);
	        ret = -1;
	        goto exit;
	    }
	} else {
	    if (!pwm_handle) {
	        fprintf(stderr, "PWM was not started, ignoring request\n");
	        ret = -1;
	        goto exit;
	    }

	    pwm->release(pwm_handle);
	    pwm_handle = NULL;
	}

exit:
	artik_release_api_module(pwm);
	return ret;
}
