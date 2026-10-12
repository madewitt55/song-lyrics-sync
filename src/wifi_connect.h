#pragma once

typedef enum {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
} wifi_connect_state_t;

/**
 * @brief Bring up the WiFi stack (on first call) and request a station connection; non-blocking
 *
 * @note The first call initializes the WiFi driver and event loop, which triggers the initial
 *       connection attempt once the driver reports ready. Subsequent calls (e.g. after a
 *       disconnect) skip driver init and just request a new connection attempt directly.
 *       Callers are responsible for deciding when to retry (see wifi_connect_get_state()).
 */
void wifi_connect_start(void);

/**
 * @brief Get the current Wi-Fi station connection state
 */
wifi_connect_state_t wifi_connect_get_state(void);

/**
 * @brief Get the reason code from the most recent Wi-Fi disconnect
 *
 * @return A wifi_err_reason_t value, or 0 if the station has yet to disconnect
 */
int wifi_connect_get_last_error(void);
