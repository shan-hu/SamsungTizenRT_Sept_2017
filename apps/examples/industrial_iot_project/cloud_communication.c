#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <artik_module.h>
#include <artik_error.h>
#include <artik_cloud.h>
#include <artik_gpio.h>
#include <artik_platform.h>
#include <cJSON.h>

#include "industrial_iot_utils.h"
#include "cloud_communication.h"

#define AKC_DEFAULT_DTID    "dt6e3d090c362146e5a5e5704881a347e1"

struct ArtikCloudConfig cloud_config;

static artik_websocket_handle g_ws_handle = NULL;

// default vibration threshold
int motor_control_vib_global = 75;

// default vibration normalization factor
float vibration_normalization_factor = 0.8;

void CloudResetConfig(void)
{
    memset(&cloud_config, 0, sizeof(cloud_config));
    strncpy(cloud_config.device_id, "null", AKC_DID_LEN);
    strncpy(cloud_config.device_token, "null", AKC_TOKEN_LEN);
    strncpy(cloud_config.device_pin, "null", AKC_PIN_LEN);
    strncpy(cloud_config.device_type_id, AKC_DEFAULT_DTID, AKC_DTID_LEN);
}

static void set_led_state(bool state)
{
    artik_gpio_config config;
    artik_gpio_handle handle;
    artik_gpio_module *gpio = (artik_gpio_module *)artik_request_api_module("gpio");
    char name[16] = "";

    if (!gpio)
        return;

    memset(&config, 0, sizeof(config));
    config.dir = GPIO_OUT;
    config.id = ARTIK_A051_XGPIO26;
    snprintf(name, 16, "gpio%d", config.id);
    config.name = name;

    if (gpio->request(&handle, &config) != S_OK)
        return;

    gpio->write(handle, state ? 1 : 0);
    gpio->release(handle);

    artik_release_api_module(gpio);
}

static void cloud_websocket_rx_cb(void *user_data, void *result)
{
    cJSON *msg, *error, *code, *type, *data, *actions, *action, *parameters;
    int speed_control;
    double vibration_factor;

    if (!result)
        return;

    fprintf(stderr, "CLOUD RX: %s\n", (char*)result);

    /* Parse JSON and look for actions */
    msg = cJSON_Parse((const char *)result);
    if (!msg)
        goto exit;

    /* Check if error */
    error = cJSON_GetObjectItem(msg, "error");
    if (error && (error->type == cJSON_Object)) {
        code = cJSON_GetObjectItem(error, "code");
        if (code && (code->type == cJSON_Number)) {
            data = cJSON_GetObjectItem(error, "message");
            if (data && (data->type == cJSON_String)) {
                printf("Websocket error %d - %s\n", code->valueint,
                        data->valuestring);
                if (code->valueint == 404) {
                    /*
                     * Device no longer exists, going back
                     * to onboarding service.
                     */
                    ResetConfiguration();
                    StartCloudWebsocket(false);
                }
            }
        }

        goto error;
    }

    type = cJSON_GetObjectItem(msg, "type");
    if (!type || (type->type != cJSON_String))
        goto error;

    if (strncmp(type->valuestring, "action", strlen("action")))
        goto error;

    data = cJSON_GetObjectItem(msg, "data");
    if (!data || (data->type != cJSON_Object))
        goto error;

    actions = cJSON_GetObjectItem(data, "actions");
    if (!actions || (actions->type != cJSON_Array))
        goto error;

    /* Browse through actions */
    cJSON_ArrayForEach(action, actions) {
        cJSON *name;

        if (action->type != cJSON_Object)
            continue;

        name = cJSON_GetObjectItem(action, "name");

        if (!name || (name->type != cJSON_String))
            continue;

        if(!strncmp(name->valuestring, "setThresholdVibration", strlen("setThresholdVibration"))) {
            fprintf(stderr, "CLOUD ACTION: setThresholdVibration. \n");
            parameters = cJSON_GetObjectItem(action, "parameters");
            if (!parameters || (parameters->type != cJSON_Object))
                continue;
            speed_control = cJSON_GetObjectItem(parameters, "vibrationThreshold")->valueint;
            printf("*** Vib threshold control = %d\n", speed_control);
            motor_control_vib_global = speed_control;
        }
        else if(!strncmp(name->valuestring, "setVibrationReportingFactor", strlen("setVibrationReportingFactor"))) {
            fprintf(stderr, "CLOUD ACTION: setVibrationReportingFactor. \n");
            parameters = cJSON_GetObjectItem(action, "parameters");
            if (!parameters || (parameters->type != cJSON_Object))
                continue;
            vibration_factor = cJSON_GetObjectItem(parameters, "vibrationFactor")->valuedouble;
            vibration_normalization_factor = (float) vibration_factor;

            char str[10];   // --- debug --- //
            sprintf(str, "%.4g", vibration_normalization_factor);  // --- debug --- //
            printf("*** Vib threshold control = %s\n", str);

        }
    }

error:
    cJSON_Delete(msg);
exit:
    free(result);
}

static pthread_addr_t websocket_start_cb(void *arg)
{
    artik_cloud_module *cloud = (artik_cloud_module*)artik_request_api_module("cloud");
    artik_error ret = S_OK;

    if (!cloud) {
        printf("Cloud module is not available\n");
        return NULL;
    }

    ret = cloud->websocket_open_stream(&g_ws_handle, cloud_config.device_token,
            cloud_config.device_id, false);
    if (ret != S_OK) {
        printf("Failed to open websocket to cloud (err=%d)\n", ret);
        goto exit;
    }

    cloud->websocket_set_receive_callback(g_ws_handle, cloud_websocket_rx_cb, NULL);

exit:
    artik_release_api_module(cloud);
    return NULL;
}

static pthread_addr_t websocket_stop_cb(void *arg)
{
    int ret = 0;
    artik_cloud_module *cloud = (artik_cloud_module*)artik_request_api_module("cloud");

    if (!cloud) {
        printf("Cloud module is not available\n");
        ret = E_NOT_SUPPORTED;
        goto exit;
    }

    cloud->websocket_set_receive_callback(g_ws_handle, NULL, NULL);

    ret = cloud->websocket_close_stream(g_ws_handle);
    if (ret != S_OK) {
        artik_release_api_module(cloud);
        printf("Failed to close websocket to cloud (err=%d)\n", ret);
        goto exit;
    }

    g_ws_handle = NULL;
    artik_release_api_module(cloud);

 exit:
    return NULL;
}

artik_error StartCloudWebsocket(bool start)
{
    artik_error ret = S_OK;

    if (start) {
        static pthread_t tid;
        pthread_attr_t attr;
        int status;
        struct sched_param sparam;

        if (g_ws_handle) {
            printf("Websocket is already open, close it first\n");
            goto exit;
        }

        pthread_attr_init(&attr);
        sparam.sched_priority = 100;
        status = pthread_attr_setschedparam(&attr, &sparam);
        status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        status = pthread_attr_setstacksize(&attr, 1024 * 8);
        status = pthread_create(&tid, &attr, websocket_start_cb, NULL);
        if (status) {
            printf("Failed to create thread for websocket\n");
            goto exit;
        }

        pthread_setname_np(tid, "cloud-websocket");
        pthread_join(tid, NULL);
    } else {
        static pthread_t tid;
        pthread_attr_t attr;
        int status;
        struct sched_param sparam;

        if (!g_ws_handle) {
            printf("Websocket is not open, cannot close it\n");
            goto exit;
        }

        pthread_attr_init(&attr);
        sparam.sched_priority = 100;
        status = pthread_attr_setschedparam(&attr, &sparam);
        status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        status = pthread_attr_setstacksize(&attr, 1024 * 8);
        status = pthread_create(&tid, &attr, websocket_stop_cb, NULL);
        if (status) {
            printf("Failed to create thread for closing websocket\n");
            goto exit;
        }
    }

exit:
    return ret;
}

artik_error SendMessageToCloud(char *message)
{
    artik_error ret = S_OK;
    artik_cloud_module *cloud = (artik_cloud_module*)artik_request_api_module("cloud");

    if (!cloud) {
        printf("Cloud module is not available\n");
        return E_NOT_SUPPORTED;
    }

    if (!g_ws_handle) {
        printf("Websocket is not open, cannot send message\n");
        goto exit;
    }

    ret = cloud->websocket_send_message(g_ws_handle, message);
    if (ret != S_OK) {
        printf("Failed to send message to cloud (err=%d)\n", ret);
        goto exit;
    }

exit:
    artik_release_api_module(cloud);
    return ret;
}

static pthread_addr_t get_device_cb(void *arg)
{
    artik_error ret = S_OK;
    char *response = NULL;
    artik_cloud_module *cloud = (artik_cloud_module*)artik_request_api_module("cloud");
    int res = 0;

    if (!cloud) {
        printf("Cloud module is not available\n");
        return NULL;
    }

    ret = cloud->get_device(cloud_config.device_token, cloud_config.device_id, false, &response);
    if (ret != S_OK) {
        printf("Failed to call get_device Cloud API (err=%d)\n", ret);
        goto exit;
    }

    if (response) {
        fprintf(stdout, "Response: %s\n", response);
        res = 1;
        free(response);
    }

exit:
    artik_release_api_module(cloud);

    pthread_exit((void *)res);

    return NULL;
}

bool ValidateCloudDevice(void)
{
    int res = 0;
    static pthread_t tid;
    pthread_attr_t attr;
    int status;
    struct sched_param sparam;

    pthread_attr_init(&attr);
    sparam.sched_priority = 100;
    status = pthread_attr_setschedparam(&attr, &sparam);
    status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    status = pthread_attr_setstacksize(&attr, 1024 * 8);
    status = pthread_create(&tid, &attr, get_device_cb, NULL);
    if (status) {
        printf("Failed to create thread for closing websocket\n");
        goto exit;
    }

    pthread_join(tid, (void**)&res);

exit:
    return  res;
}




