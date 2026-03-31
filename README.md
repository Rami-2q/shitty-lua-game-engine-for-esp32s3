# shitty-lua-game-engine-for-esp32s3
A game console firmware for the ESP32-S3. It runs Lua scripts directly from an SD card on a 128×128 ST7735 display, with a D-pad and buttons wired up as a controller. 


made with ai do not expect much.

 The idea is simple: drop a .lua file on the SD card, browse to it, press A, and it runs. No flashing required. The Lua API gives scripts access to graphics, input, file I/O, WiFi, HTTP, JSON, and PSRAM — enough to build small games, tools, or networked apps.
There's also a built-in WiFi file server. Hold the Home button from the root directory, enter your credentials on the on-screen keyboard, and the ESP32 serves a web interface over your local network where you can upload, download, copy, and delete files from the SD card — all from a browser.


Hardware
ESP32-S3
ST7735 128×128 TFT                 HSPI (MOSI=8, SCLK=7, CS=6, DC=5)
SD Card module                        FSPI (MISO=1, SCK=2, MOSI=3, CS=4)
D-pad + A/B/Home                   GPIO 9–13, 43, 44 (INPUT_PULLUP)


Lua API
Graphics — cls, flip, pset, pget, line, rect, frect, circ, fcirc, tri, ftri, print, rgb, spr, spr_scale, camera
Input — btn, btnp, btnr (held / just pressed / just released)
Time — tick, dt, millis, delay
Math — rnd, rnd_int, clamp, lerp, sin_deg, cos_deg, dist, aabb
Files — file_read, file_write, file_exists, file_delete, dir
WiFi / HTTP — wifi_connect, http_get, http_post, json_decode, json_encode
PSRAM — psram_alloc, psram_load, psram_get, psram_set, psram_save (for large buffers)

Dependencies
TFT_eSPI                                      https://github.com/Bodmer/TFT_eSPI
LuaWrapper                                https://github.com/sfranzyshen/ESP-Arduino-Lua
