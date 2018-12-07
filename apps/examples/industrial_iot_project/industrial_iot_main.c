#include <stdio.h>
#include <tinyara/config.h>
#include <tinyara/shell/tash.h>
#include <tinyara/fs/fs_utils.h>
#include <sys/stat.h>
#include <artik_error.h>
#include <artik_cloud.h>
#include <errno.h>

#include "industrial_iot_utils.h"
#include "industrial_iot_routine.h"

static int iiot_main(int argc, char *argv[])
{
    int ret = 0;

    if ((argc > 1) && !strcmp(argv[1], "reset")) {
        ResetConfiguration();
        printf("Onboarding configuration was reset. Reboot the board"
                " to return to onboarding mode\n");
        goto exit;
    }

    if (InitConfiguration() != S_OK) {
        ret = -1;
        goto exit;
    }

    //iiot_start();

    /* If we already have Wifi credentials, try to connect to the hotspot */
    if (strlen(wifi_config.ssid) > 0) {
        if (StartStationConnection(true) == S_OK) {
            /* Check if we have valid ARTIK Cloud credentials */
            if ((strlen(cloud_config.device_id) == AKC_DID_LEN) &&
                (strlen(cloud_config.device_token) == AKC_TOKEN_LEN)) {
                if (ValidateCloudDevice()) {
                    if (StartCloudWebsocket(true) == S_OK) {
                        printf("ARTIK Cloud connection started\n");
                        //start to excute inudstrial IoT routine
                   

                        if(iiot_start() == 0)
                            goto exit;
                    }
                }
            }
        }
    }

    /* If Cloud connection failed, start the onboarding service */
    if (StartSoftAP(true) != S_OK) {
        ret = -1;
        goto exit;
    }

    if (StartWebServer(true, API_SET_WIFI) != S_OK) {
        StartSoftAP(false);
        ret = -1;
        goto exit;
    }

    printf("ARTIK Onboarding Service started\n");

exit:
    return ret;
}

extern int module_main(int argc, char *argv[]);
extern int cloud_main(int argc, char *argv[]);
extern int gpio_main(int argc, char *argv[]);
extern int pwm_main(int argc, char *argv[]);
extern int adc_main(int argc, char *argv[]);
extern int http_main(int argc, char *argv[]);
extern int wifi_main(int argc, char *argv[]);
extern int websocket_main(int argc, char *argv[]);
extern int demo_buzz_test(int argc, char *argv[]);

static tash_cmdlist_t atk_cmds[] = {
    {"sdk", module_main, TASH_EXECMD_SYNC},
    {"gpio", gpio_main, TASH_EXECMD_SYNC},
    {"pwm", pwm_main, TASH_EXECMD_SYNC},
    {"adc", adc_main, TASH_EXECMD_SYNC},
    {"cloud", cloud_main, TASH_EXECMD_SYNC},
    {"http", http_main, TASH_EXECMD_SYNC},
    {"wifi", wifi_main, TASH_EXECMD_SYNC},
    {"websocket", websocket_main, TASH_EXECMD_SYNC},
    {"iiot", iiot_main, TASH_EXECMD_SYNC},
    {"pwm2",  demo_buzz_test, TASH_EXECMD_SYNC},
    {NULL, NULL, 0}
};

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int industrial_iot_main(int argc, char *argv[])
#endif
{
    int ret = 0;

#ifdef CONFIG_TASH
    /* add tash command */
    tash_cmdlist_install(atk_cmds);
#endif

#ifdef CONFIG_CTRL_IFACE_FIFO
    /* Ensure the fifo's are initialized before attempting to open */
    ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_REQ, CONFIG_WPA_CTRL_FIFO_MK_MODE);
    if(ret != 0 && ret != -EEXIST)
    {
        printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_REQ, strerror(errno));
    }
    ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_CFM, CONFIG_WPA_CTRL_FIFO_MK_MODE);
    if(ret != 0 && ret != -EEXIST)
    {
        printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_CFM, strerror(errno));
    }

    ret = mkfifo(CONFIG_WPA_MONITOR_FIFO_DEV, CONFIG_WPA_CTRL_FIFO_MK_MODE);
    if(ret != 0 && ret != -EEXIST)
    {
        printf("mkfifo error for %s: %s", CONFIG_WPA_MONITOR_FIFO_DEV, strerror(errno));
    }
#endif

#ifdef CONFIG_EXAMPLES_MOUNT
    mount_app_main(0, NULL);
#endif

    //iiot_start();  // --- debug --- //
    iiot_main(argc, argv);

    return ret;
}
