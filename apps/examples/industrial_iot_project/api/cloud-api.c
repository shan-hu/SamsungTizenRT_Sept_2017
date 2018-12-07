#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_cloud.h>

#define	FAIL_AND_EXIT(x)		{ fprintf(stderr, x); usage(); ret = -1; goto exit; }

static artik_websocket_handle ws_handle = NULL;

static void websocket_rx_callback(void *user_data, void *result)
{
    if (result) {
        fprintf(stderr, "RX: %s\n", (char*)result);
        free(result);
    }
}

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\tcloud device <token> <device id> [<properties>]\n");
    fprintf(stderr, "\tcloud devices <token> <user id> [<count> <offset> <properties>]\n");
    fprintf(stderr, "\tcloud message <token> <device id> <message>\n");
    fprintf(stderr, "\tcloud connect <token> <device id> [use_se]\n");
    fprintf(stderr, "\tcloud disconnect\n");
    fprintf(stderr, "\tcloud send <message>\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int cloud_main(int argc, char *argv[])
#endif
{
	artik_cloud_module *cloud = NULL;
	int ret = 0;

	if (argc < 2) {
		usage();
		ret = -1;
		goto exit;
	}

    if (!strcmp(argv[1], "device")) {
        char *response = NULL;
        artik_error err = S_OK;
        bool properties = false;

        /* Check number of arguments */
        if (argc < 4) FAIL_AND_EXIT("Wrong number of arguments\n")

        /* Parse optional arguments */
        if (argc > 4) {
            properties = (atoi(argv[4]) > 0);
        }

        cloud = (artik_cloud_module *)artik_request_api_module("cloud");
        if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

        err = cloud->get_device(argv[2], argv[3], properties, &response);
        if (err != S_OK) FAIL_AND_EXIT("Failed to get user devices\n")

        if (response) {
            fprintf(stdout, "Response: %s\n", response);
            free(response);
        }
    } else if (!strcmp(argv[1], "devices")) {
		char *response = NULL;
		artik_error err = S_OK;
		int count = 10;
		bool properties = false;
		int offset = 0;

		/* Check number of arguments */
		if (argc < 4) FAIL_AND_EXIT("Wrong number of arguments\n")

		/* Parse optional arguments */
		if (argc > 4) {
			count = atoi(argv[4]);
			if (argc > 5) {
				offset = atoi(argv[5]);
				if (argc > 6)
					properties = (atoi(argv[6]) > 0);
			}
		}

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

		err = cloud->get_user_devices(argv[2], count, properties, 0, argv[3], &response);
		if (err != S_OK) FAIL_AND_EXIT("Failed to get user devices\n")

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
    } else if (!strcmp(argv[1], "message")) {
        char *response = NULL;
		artik_error err = S_OK;

        /* Check number of arguments */
		if (argc < 5) FAIL_AND_EXIT("Wrong number of arguments\n")

        cloud = (artik_cloud_module *)artik_request_api_module("cloud");
        if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

        err = cloud->send_message(argv[2], argv[3], argv[4], &response);
        if (err != S_OK) FAIL_AND_EXIT("Failed to send message\n")

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}

    } else if (!strcmp(argv[1], "connect")) {
        artik_error err = S_OK;
        bool use_se = false;

        if (ws_handle) {
            fprintf(stderr, "Websocket to cloud is already connected\n");
            goto exit;
        }

        /* Check number of arguments */
        if (argc < 4) FAIL_AND_EXIT("Wrong number of arguments\n")

        if ((argc == 5) && !strncmp(argv[4], "use_se", strlen("use_se")))
            use_se = true;

        cloud = (artik_cloud_module *)artik_request_api_module("cloud");
        if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

        err = cloud->websocket_open_stream(&ws_handle, argv[2], argv[3], use_se);
        if (err != S_OK) FAIL_AND_EXIT("Failed to connect websocket\n")

        err = cloud->websocket_set_receive_callback(ws_handle, websocket_rx_callback, NULL);
        if (err != S_OK) FAIL_AND_EXIT("Failed to set websocket receive callback\n")

    } else if (!strcmp(argv[1], "disconnect")) {

        if (!ws_handle) {
            fprintf(stderr, "Websocket to cloud is not connected\n");
            goto exit;
        }

        cloud = (artik_cloud_module *)artik_request_api_module("cloud");
        if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

        cloud->websocket_close_stream(ws_handle);

    } else if (!strcmp(argv[1], "send")) {
        artik_error err = S_OK;

        if (!ws_handle) {
            fprintf(stderr, "Websocket to cloud is not connected\n");
            goto exit;
        }

        /* Check number of arguments */
        if (argc < 3) FAIL_AND_EXIT("Wrong number of arguments\n")

        cloud = (artik_cloud_module *)artik_request_api_module("cloud");
        if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

        err = cloud->websocket_send_message(ws_handle, argv[2]);
        if (err != S_OK) FAIL_AND_EXIT("Failed to send message to cloud through websocket\n")

	} else {
		usage();
		ret = -1;
		goto exit;
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}
