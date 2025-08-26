#pragma once
#include <stdbool.h>
#include <esp_err.h>


esp_err_t wifi_setup_start(void);
bool wifi_is_connected(void);
