#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_gpio.h>

static void usage(void)
{
    fprintf(stderr, "usage: gpio read|write <num> [state]\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int gpio_main(int argc, char *argv[])
#endif
{
	artik_gpio_module *gpio;
	artik_gpio_config config;
	artik_gpio_handle handle;
	char name[16] = "";
	int ret = 0;

	if ((argc < 3) || (strcmp(argv[1], "write") && strcmp(argv[1], "read"))) {
		usage();
		return -1;
	}

	gpio = (artik_gpio_module *)artik_request_api_module("gpio");
	if (!gpio) {
	    fprintf(stderr, "GPIO module is not available\n");
	    return -1;
	}

	memset(&config, 0, sizeof(config));
	config.dir = strcmp(argv[1], "write") ? GPIO_IN : GPIO_OUT;
	config.id = atoi(argv[2]);
	snprintf(name, 16, "gpio%d", config.id);
	config.name = name;

	if (gpio->request(&handle, &config) != S_OK) {
		fprintf(stderr, "Failed to request GPIO %d\n", config.id);
		ret = -1;
		goto exit;
	}

	if (!strcmp(argv[1], "write"))
		gpio->write(handle, atoi(argv[3]));
	else
		fprintf(stdout, "%d\n", gpio->read(handle));

	gpio->release(handle);

exit:
	artik_release_api_module(gpio);
	return ret;
}
