#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <artik_module.h>
#include <artik_wifi.h>

#define WIFI_SCAN_TIMEOUT       15
#define WIFI_CONNECT_TIMEOUT    30
#define WIFI_DISCONNECT_TIMEOUT 10
#define	FAIL_AND_USAGE(x)		{ fprintf(stderr, x); usage(); ret = -1; goto exit; }
#define FAIL_AND_EXIT(x)        { fprintf(stderr, x); ret = -1; goto exit; }

struct callback_result {
    sem_t sem;
    artik_wifi_connection_info info;
    artik_error error;
};

static void wifi_scan_callback(void *result, void *user_data)
{
    struct callback_result *res = (struct callback_result *)user_data;

    res->error = *((artik_error *)result);
    sem_post(&(res->sem));
}

static void wifi_connect_callback(void *result, void *user_data)
{
    struct callback_result *res = (struct callback_result *)user_data;

    memcpy(&(res->info), (artik_wifi_connection_info *)result, sizeof(artik_wifi_connection_info));
    sem_post(&(res->sem));
}

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\twifi startsta\n");
    fprintf(stderr, "\twifi startap <ssid> <channel> [<passphrase>]\n");
    fprintf(stderr, "\twifi scan\n");
    fprintf(stderr, "\twifi connect <ssid> <passphrase> [persistent]\n");
    fprintf(stderr, "\twifi disconnect\n");
    fprintf(stderr, "\twifi stop\n");
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int wifi_main(int argc, char *argv[])
#endif
{
	artik_wifi_module *wifi = NULL;
	int ret = 0;

	if (argc < 2) {
		usage();
		ret = -1;
		goto exit;
	}

	if (!strcmp(argv[1], "startsta")) {
		artik_error err = S_OK;

		wifi = (artik_wifi_module *)artik_request_api_module("wifi");
		if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        err = wifi->init(ARTIK_WIFI_MODE_STATION);
		if (err != S_OK) {
		    fprintf(stderr, "Failed to initialize wifi (%s)\n", error_msg(err));
		    ret = -1;
		    goto exit;
		}

	} else if (!strcmp(argv[1], "startap")) {
	    artik_error err = S_OK;
	    char *passphrase = NULL;

        /* Check number of arguments */
        if (argc < 4) FAIL_AND_USAGE("Wrong number of arguments\n")

        if (argc == 5)
            passphrase = argv[4];

        wifi = (artik_wifi_module *)artik_request_api_module("wifi");
        if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        err = wifi->init(ARTIK_WIFI_MODE_AP);
        if (err != S_OK) {
            fprintf(stderr, "Failed to initialize wifi (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        err = wifi->start_ap(argv[2], passphrase, atoi(argv[3]));
        if (err != S_OK) {
            fprintf(stderr, "Failed to start Access Point (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

	} else if (!strcmp(argv[1], "stop")) {
	    artik_error err = S_OK;

        wifi = (artik_wifi_module *)artik_request_api_module("wifi");
        if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        err = wifi->deinit();
        if (err != S_OK) {
            fprintf(stderr, "Failed to deinitialize wifi (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

    } else if (!strcmp(argv[1], "scan")) {
        artik_error err = S_OK;
        artik_wifi_ap *list = NULL;
        int count = 0, i = 0;
        struct callback_result scan_res;
        struct timespec timeout;

        wifi = (artik_wifi_module *)artik_request_api_module("wifi");
        if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        err = wifi->set_scan_result_callback(wifi_scan_callback, (void*)&scan_res);
        if (err != S_OK) {
            fprintf(stderr, "Failed to set scan result callback (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        sem_init(&(scan_res.sem), 0, 0);

        err = wifi->scan_request();
        if (err != S_OK) {
            sem_destroy(&(scan_res.sem));
            wifi->set_scan_result_callback(NULL, NULL);
            fprintf(stderr, "Failed to launch scan request (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += WIFI_SCAN_TIMEOUT;
        if (sem_timedwait(&(scan_res.sem), &timeout) == ETIMEDOUT) {
            sem_destroy(&(scan_res.sem));
            wifi->set_scan_result_callback(NULL, NULL);
            fprintf(stderr, "Timed out while waiting for scan result\n");
            ret = -1;
            goto exit;
        }

        sem_destroy(&(scan_res.sem));
        wifi->set_scan_result_callback(NULL, NULL);

        if (scan_res.error != S_OK) FAIL_AND_EXIT("Failure while scanning the networks\n");

        err = wifi->get_scan_result(&list, &count);
        if (err != S_OK) {
            fprintf(stderr, "Failed to get scan results (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        for (i=0; i<count; i++) {
            fprintf(stderr, "%s\t%d\t%d\t%08x\t%s\n", list[i].bssid, list[i].frequency,
                    list[i].signal_level, list[i].encryption_flags, list[i].name);
        }

        free(list);

    } else if (!strcmp(argv[1], "connect")) {
        artik_error err = S_OK;
        struct callback_result conn_res;
        struct timespec timeout;
        bool persistent = false;

        /* Check number of arguments */
        if (argc < 4) FAIL_AND_USAGE("Wrong number of arguments\n")

        if ((argc == 5) && !strncmp(argv[4], "persistent", strlen("persistent")))
            persistent = true;

        wifi = (artik_wifi_module *)artik_request_api_module("wifi");
        if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        sem_init(&(conn_res.sem), 0, 0);

        err = wifi->set_connect_callback(wifi_connect_callback, &(conn_res.sem));
        if (err != S_OK) {
            sem_destroy(&(conn_res.sem));
            fprintf(stderr, "Failed to set connect callback (%)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        err = wifi->connect(argv[2], argv[3], persistent);
        if (err != S_OK) {
            sem_destroy(&(conn_res.sem));
            wifi->set_connect_callback(NULL, NULL);
            fprintf(stderr, "Failed to start connection to AP (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += WIFI_CONNECT_TIMEOUT;
        if ((sem_timedwait(&(conn_res.sem), &timeout) < 0) && (errno == ETIMEDOUT)) {
            sem_destroy(&(conn_res.sem));
            wifi->set_connect_callback(NULL, NULL);
            fprintf(stderr, "Timed out while waiting for connection\n");
            ret = -1;
            goto exit;
        }

        sem_destroy(&(conn_res.sem));
        wifi->set_connect_callback(NULL, NULL);

        if (!conn_res.info.connected) {
            fprintf(stderr, "Failed to join the network (err=%d)\n", conn_res.info.error);
            ret = -1;
            goto exit;
        }

        fprintf(stderr, "Connected to %s\n", argv[2]);

    } else if (!strcmp(argv[1], "disconnect")) {
        artik_error err = S_OK;
        struct callback_result conn_res;
        struct timespec timeout;

        wifi = (artik_wifi_module *)artik_request_api_module("wifi");
        if (!wifi) FAIL_AND_EXIT("Failed to request wifi module\n")

        sem_init(&(conn_res.sem), 0, 0);

        err = wifi->set_connect_callback(wifi_connect_callback, &(conn_res.sem));
        if (err != S_OK) {
            sem_destroy(&(conn_res.sem));
            fprintf(stderr, "Failed to set connect callback (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        err = wifi->disconnect();
        if (err != S_OK) {
            sem_destroy(&(conn_res.sem));
            wifi->set_connect_callback(NULL, NULL);
            fprintf(stderr, "Failed to disconnect from AP (%s)\n", error_msg(err));
            ret = -1;
            goto exit;
        }

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += WIFI_DISCONNECT_TIMEOUT;
        if ((sem_timedwait(&(conn_res.sem), &timeout) < 0) && (errno == ETIMEDOUT)) {
            sem_destroy(&(conn_res.sem));
            wifi->set_connect_callback(NULL, NULL);
            fprintf(stderr, "Timed out while waiting for disconnection\n");
            ret = -1;
            goto exit;
        }

        sem_destroy(&(conn_res.sem));
        wifi->set_connect_callback(NULL, NULL);

        if (conn_res.info.connected) {
            fprintf(stderr, "Failed to disconnect from the network (err=%d)\n", conn_res.info.error);
            ret = -1;
            goto exit;
        }
	} else {
		usage();
		ret = -1;
		goto exit;
	}

exit:
	if (wifi)
		artik_release_api_module(wifi);

	return ret;
}
