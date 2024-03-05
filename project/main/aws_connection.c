#include "aws_connection.h"

const int CONNECTED_BIT = BIT0;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    }
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                                const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    shadowUpdateInProgress = false;

    if(SHADOW_ACK_TIMEOUT == status) {
        ESP_LOGE(TAG, "Update timed out");
    } else if(SHADOW_ACK_REJECTED == status) {
        ESP_LOGE(TAG, "Update rejected");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        ESP_LOGI(TAG, "Update accepted");
    }
}

void ledtActuate_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);

    if(pContext != NULL) {
        ESP_LOGI(TAG, "Delta - LED state changed to %d", *(int *) (pContext->pData));
    }
}

void aws_iot_task(void *param)
{
	int led_state_recived[4] = {0, 0, 0, 0};
	int led_state_send[4] = {1, 1, 1, 1};
	for (int i = 0; i < 4; i++)
		{
			led_states[i] = 0;
		}
	bool update_needed = false;
	data_recived = false;

	while (1)
	{
		IoT_Error_t rc = FAILURE;
		char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
		size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);

		jsonStruct_t led_actuator[4] = {
				{
				.cb = ledtActuate_Callback,
				.pData = &led_state_recived[0],
				.pKey = "R",
				.type = SHADOW_JSON_UINT16,
				.dataLength = sizeof(int)
				},
				{
				.cb = ledtActuate_Callback,
				.pData = &led_state_recived[1],
				.pKey = "G",
				.type = SHADOW_JSON_UINT16,
				.dataLength = sizeof(int)
				},
				{
				.cb = ledtActuate_Callback,
				.pData = &led_state_recived[2],
				.pKey = "B",
				.type = SHADOW_JSON_UINT16,
				.dataLength = sizeof(int)
				},
				{
				.cb = ledtActuate_Callback,
				.pData = &led_state_recived[3],
				.pKey = "T",
				.type = SHADOW_JSON_UINT16,
				.dataLength = sizeof(int)
				}
		};

		AWS_IoT_Client mqttClient;
		ShadowInitParameters_t sp = ShadowInitParametersDefault;
		sp.pHost = AWS_IOT_MQTT_HOST;
		sp.port = AWS_IOT_MQTT_PORT;
		sp.pClientCRT = (const char *)certificate_pem_crt_start;
		sp.pClientKey = (const char *)private_pem_key_start;
		sp.pRootCA = (const char *)aws_root_ca_pem_start;
		sp.enableAutoReconnect = false;
		sp.disconnectHandler = NULL;
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
							false, true, portMAX_DELAY);
		ESP_LOGI(TAG, "Shadow Init");
		rc = aws_iot_shadow_init(&mqttClient, &sp);
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "aws_iot_shadow_init returned error %d, aborting...", rc);
			abort();
		}
		ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
		scp.pMyThingName = CONFIG_AWS_EXAMPLE_THING_NAME;
		scp.pMqttClientId = CONFIG_AWS_EXAMPLE_CLIENT_ID;
		scp.mqttClientIdLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
		ESP_LOGI(TAG, "Shadow Connect");
		rc = aws_iot_shadow_connect(&mqttClient, &scp);
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "aws_iot_shadow_connect returned error %d, aborting...", rc);
			abort();
		}
		rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d, aborting...", rc);
			abort();
		}
		for (int i = 0; i < 4 ; i++)
		{
			rc = aws_iot_shadow_register_delta(&mqttClient, &led_actuator[i]);
		}
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "Shadow Register Delta Error");
		}
		while(NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) {
			rc = aws_iot_shadow_yield(&mqttClient, 200);
			for (int i = 0; i < 4 ; i++)
				{
					led_states[i] = led_state_recived[i];
				}
			if(NETWORK_ATTEMPTING_RECONNECT == rc || shadowUpdateInProgress) {
				rc = aws_iot_shadow_yield(&mqttClient, 1000);
				continue;
			}
			ESP_LOGI(TAG, "=======================================================================================");
			update_needed = false;
			for (int i = 0; i < 4; i++)
			{
				if(led_state_send[i] != led_state_recived[i])
				{
					update_needed = true;
				}
			}
			rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
			if(SUCCESS == rc)
			{
				rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 4, &led_actuator[0], &led_actuator[1], &led_actuator[2], &led_actuator[3]);
				if(SUCCESS == rc)
				{
					rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
					if(SUCCESS == rc && update_needed)
					{
						ESP_LOGI(TAG, "Update Shadow: %s", JsonDocumentBuffer);
						rc = aws_iot_shadow_update(&mqttClient, CONFIG_AWS_EXAMPLE_THING_NAME, JsonDocumentBuffer,
												   ShadowUpdateStatusCallback, NULL, 4, true);
						data_recived = true;
						for (int i = 0; i < 4; i++)
						{
							led_state_send[i] = led_state_recived[i];
						}
						shadowUpdateInProgress = true;
					}
				}
			}
			ESP_LOGI(TAG, "*****************************************************************************************");
			ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(NULL), uxTaskGetStackHighWaterMark(NULL));
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "An error occurred in the loop %d", rc);
		}
		ESP_LOGI(TAG, "Disconnecting");
		rc = aws_iot_shadow_disconnect(&mqttClient);
		if(SUCCESS != rc)
		{
			ESP_LOGE(TAG, "Disconnect error %d", rc);
		}
		vTaskDelete(NULL);
	}
}

static void initialise_wifi(void)
{
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_create_default_wifi_sta();
#endif
    wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void aws_connect()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    initialise_wifi();
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL, 1);
}
