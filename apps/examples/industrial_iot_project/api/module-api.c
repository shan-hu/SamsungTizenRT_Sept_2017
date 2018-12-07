#include <stdio.h>
#include <string.h>

#include <artik_module.h>

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\tsdk version - Display the version of the ARTIK SDK\n");
    fprintf(stderr, "\tsdk platform - Display the version of the ARTIK SDK\n");
    fprintf(stderr, "\tsdk info - Display information about the ARTIK SDK\n");
    fprintf(stderr, "\tsdk modules - Lists all the SDK modules available for this platform\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int module_main(int argc, char *argv[])
#endif
{
	int ret = 0;

	if (argc < 2) {
		usage();
		ret = -1;
		goto exit;
	}

	if (!strcmp(argv[1], "version")) {
		artik_api_version version;
		artik_get_api_version(&version);
		fprintf(stdout, "ARTIK SDK version: %s\n", version.version);
	} else if (!strcmp(argv[1], "platform")) {
		char platname[MAX_PLATFORM_NAME];
		if (artik_get_platform_name(platname) != S_OK) {
			fprintf(stderr, "Failed to get platform name\n");
			usage();
			ret = -1;
			goto exit;
		}
		fprintf(stdout, "Platform name: %s\n", platname);
	} else if (!strcmp(argv[1], "info")) {
		char *info = artik_get_device_info();
		if (!info) {
			fprintf(stderr, "Failed to get device info\n");
			usage();
			ret = -1;
			goto exit;
		}
		fprintf(stdout, "Device info: %s\n", info);
	} else if (!strcmp(argv[1], "modules")) {
		artik_api_module *modules = NULL;
		int num_modules = 0;
		int i = 0;

		if (artik_get_available_modules(&modules, &num_modules) != S_OK) {
			fprintf(stderr, "Failed to get available modules\n");
			usage();
			ret = -1;
			goto exit;
		}

		fprintf(stdout, "Available modules:\n");
		for (i = 0; i < num_modules; i++)
			fprintf(stdout, "\t%d: %s\n", modules[i].id, modules[i].name);

	} else {
		fprintf(stderr, "Unknown parameters\n");
		usage();
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}