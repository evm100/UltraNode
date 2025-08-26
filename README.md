# ==== README.md (overview) =====================================================
# ESP32-C3 MQTT WS2812B Controller with OTA (ESP-IDF)
#
# Features
# - Drives up to 300 WS2812B (a.k.a. NeoPixel) LEDs using RMT v2
# - MQTT control for:
# • Static color (RGB, optional brightness)
# • Effect engine scaffold with pluggable animations (1 sample effect included: "wipe")
# • External command to trigger OTA with a provided URL
# - Scheduled OTA updates (daily at configured HH:MM) — updates can add new effects
# - Wi‑Fi + SNTP time sync; NVS persistence for OTA schedule
# - Clean module separation: ws2812, effects, mqtt, ota, time_sync
#
# Build
# idf.py set-target esp32c3
# idf.py menuconfig # set Wi-Fi, MQTT broker, LED GPIO, etc.
# idf.py flash monitor
#
# MQTT Topics (base topic is configurable; default: ultralights/strip)
# - <base>/cmd/color payload JSON: {"r":0-255, "g":0-255, "b":0-255, "brightness":0.0-1.0}
# - <base>/cmd/effect payload JSON: {"name":"effectName", "params":{...}}
# - <base>/cmd/ota payload JSON: {"url":"https://host/fw.bin"}
# - <base>/cmd/ota_schedule payload JSON: {"hour":0-23, "minute":0-59, "url":"https://host/fw.bin"}
# - <base>/status/online payload: "1" on connect, "0" on disconnect (LWT)
# - <base>/status/info payload JSON: {firmware, build_time, led_count, ...}
# - <base>/status/error payload text on errors
#
# Hardware
# - ESP32-C3
# - WS2812B data on GPIO (default: GPIO0; change in menuconfig). Use 5V power w/ common ground.
#
# Notes
# - RMT clock is configured for WS2812 timings; uses a single channel and DMA-like ring buffer.
# - Effects engine exposes a registry to add more animations later.
#
# ===============================================================================
