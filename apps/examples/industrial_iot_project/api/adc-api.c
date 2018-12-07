#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_error.h>
#include <artik_adc.h>

static void usage(void)
{
    fprintf(stderr, "usage: adc read <pin num>\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int adc_main(int argc, char *argv[])
#endif
{
    artik_adc_module *adc = NULL;
    artik_adc_handle handle = NULL;
    artik_adc_config config;
    artik_error err = S_OK;
    int val = 0;
    char name[16] = "";
    int ret = 0;

    if ((argc < 3) || (strcmp(argv[1], "read"))) {
        fprintf(stderr, "Wrong arguments\n");
        usage();
        return -1;
    }

    adc = (artik_adc_module *)artik_request_api_module("adc");
    if (!adc) {
        fprintf(stderr, "ADC module is not available\n");
        return -1;
    }

    memset(&config, 0, sizeof(config));
    config.pin_num = atoi(argv[2]);
    snprintf(name, 16, "adc%d", config.pin_num);
    config.name = name;

    err = adc->request(&handle, &config);
    if (err != S_OK) {
        fprintf(stderr, "Failed to request ADC %d (%d)\n", config.pin_num, err);
        ret = -1;
        goto exit;
    }

    err = adc->get_value(handle, &val);
    if (err != S_OK) {
        fprintf(stderr, "Failed to read ADC %d value (%d)\n", config.pin_num, err);
        adc->release(handle);
        ret = -1;
        goto exit;
    }

    fprintf(stdout, "ADC%d=%d\n", config.pin_num, val);
    adc->release(handle);

exit:
    artik_release_api_module(adc);
    return ret;
}
