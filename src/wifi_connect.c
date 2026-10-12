#include "wifi_connect.h"

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "wifi_credentials.h"

#define TAG "wifi_connect" // Module tag for logging

static bool driver_started = false; // Has Wi-Fi driver started
static wifi_connect_state_t state = DISCONNECTED; // Is Wi-Fi connected
static int last_error = 0; // Most recent wifi_err_reason_t from a disconnect; 0 if none yet

/**
 * @brief Handle Wi-Fi/IP events to drive the station connection and track its state
 *
 * @note Runs on the default event loop's task, not app_main()'s task
 * 
 * @param arg Unused (registered as NULL)
 * @param event_base `WIFI_EVENT` or `IP_EVENT`
 * @param event_id Specific event within `event_base`
 * @param event_data Event payload; cast based on `event_base`/`event_id`
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Driver finished starting Wi-Fi station interface
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to %s...", WIFI_SSID);
        state = CONNECTING;
        esp_wifi_connect(); // Attempt to join
    // Working Wi-Fi connection dropped or join attempt failed
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data; // Get disconnect reason
        last_error = event->reason;
        state = DISCONNECTED;
        ESP_LOGI(TAG, "WiFi disconnected (reason %d)", event->reason);
    // IP address assigned to device, connection successful
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip)); // IPSTR/IP2STR format a raw ip4_addr_t for printf
        state = CONNECTED;
    }
}

void wifi_connect_start(void)
{
    // Driver already started Wi-Fi station interface
    // This is a reconnect attempt after disconnecting
    if (driver_started) {
        state = CONNECTING;
        esp_wifi_connect(); // Attempt to join
        return;
    }

    // Initialize non-volatile storage for radio calibration data
    // Key-value pairs stored survive reboots
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init()); // Starts the TCP/IP stack (lwIP); required before any network interface exists
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Creates the task that dispatches WIFI_EVENT/IP_EVENT to handlers
    esp_netif_create_default_wifi_sta(); // Allocates the netif (virtual network interface) the Wi-Fi driver attaches to

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT(); // Default buffer counts/sizes for the Wi-Fi driver's internal task
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg)); // Allocates driver resources

    // Route Wi-Fi and IP events to the same handler for all event IDs
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,         // Defined in wifi_credentials.h, which is gitignored to keep credentials out of version control
            .password = WIFI_PASSWORD, // ditto
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Station mode: join an existing network rather than hosting one
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg)); // Attach the SSID/password above to the station interface
    ESP_ERROR_CHECK(esp_wifi_start()); // Starts the driver task, triggers WIFI_EVENT_STA_START once ready
    driver_started = true;
}

wifi_connect_state_t wifi_connect_get_state(void)
{
    return state;
}

int wifi_connect_get_last_error(void)
{
    return last_error;
}
