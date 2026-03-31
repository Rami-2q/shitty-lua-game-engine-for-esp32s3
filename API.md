# Lua API Reference — ESP32-S3 Lua Engine

API available in every `.lua` script launched from the file browser.  
Screen: **128×128 px**, color format **RGB565**.  
Button indices: `0=UP  1=DOWN  2=LEFT  3=RIGHT  4=A  5=B  6=HOME`

---

## Table of Contents

- [Graphics](#graphics)
- [Sprites](#sprites)
- [Camera](#camera)
- [Input](#input)
- [Time](#time)
- [Math](#math)
- [SD / File System](#sd--file-system)
- [System](#system)
- [WiFi](#wifi)
- [HTTP](#http)
- [JSON](#json)
- [PSRAM Buffers](#psram-buffers)
- [Game Loop Template](#game-loop-template)

---

## Graphics

### `cls([color])`
Clears the canvas with the given color. Defaults to black.
```lua
cls()
cls(rgb(30, 0, 60))
```

---

### `flip()`
Pushes the canvas to the display. Call once at the end of every frame.
```lua
flip()
```

---

### `pset(x, y, color)`
Draws a single pixel.
```lua
pset(64, 64, rgb(255, 0, 0))
```

---

### `pget(x, y)` → `color`
Reads the color of a pixel from the canvas.
```lua
local c = pget(10, 10)
```

---

### `print(x, y, str, color [, size])`
Draws text using the built-in pixel font. `size` is an integer scale multiplier (default `1`).  
Only uppercase letters and basic ASCII characters are supported.
```lua
print(2, 2, "HELLO", rgb(255, 255, 0))
print(2, 20, "BIG", rgb(0, 255, 0), 2)
```

---

### `measure_text(str [, size])` → `width`
Returns the pixel width the string would occupy when drawn.
```lua
local w = measure_text("SCORE", 1)
print(64 - w / 2, 10, "SCORE", 0xFFFF)
```

---

### `line(x0, y0, x1, y1, color)`
Draws a line between two points.
```lua
line(0, 0, 127, 127, rgb(255, 255, 255))
```

---

### `rect(x, y, w, h, color)`
Draws a rectangle outline.
```lua
rect(10, 10, 50, 30, rgb(0, 200, 255))
```

---

### `frect(x, y, w, h, color)`
Draws a filled rectangle.
```lua
frect(0, 0, 128, 128, rgb(0, 0, 0))
```

---

### `circ(x, y, r, color)`
Draws a circle outline.
```lua
circ(64, 64, 20, rgb(255, 100, 0))
```

---

### `fcirc(x, y, r, color)`
Draws a filled circle.
```lua
fcirc(64, 64, 20, rgb(255, 100, 0))
```

---

### `tri(x0,y0, x1,y1, x2,y2, color)`
Draws a triangle outline.
```lua
tri(64, 10, 20, 100, 108, 100, rgb(0, 255, 128))
```

---

### `ftri(x0,y0, x1,y1, x2,y2, color)`
Draws a filled triangle.
```lua
ftri(64, 10, 20, 100, 108, 100, rgb(0, 255, 128))
```

---

### `rgb(r, g, b)` → `color`
Converts 0–255 RGB components to a packed RGB565 color value.
```lua
local red = rgb(255, 0, 0)
```

---

### `rgb_split(color)` → `r, g, b`
Splits an RGB565 color into its 5/6/5-bit components.
```lua
local r, g, b = rgb_split(0xF800)
```

---

## Sprites

Each sprite is a **16×16 pixel** image stored as RGB565 values.  
Up to **32 slots** are available (IDs `0`–`31`).

---

### `spr_define(id, table)` → `bool`
Loads a sprite from a Lua table of 256 RGB565 values (row-major, left to right).
```lua
local px = {}
for i = 1, 256 do px[i] = rgb(255, 0, 0) end  -- solid red sprite
spr_define(0, px)
```

---

### `spr_load(id, path)` → `bool`
Loads a sprite from a binary file on the SD card (256 × uint16_t, little-endian).
```lua
local ok = spr_load(0, "/sprites/player.bin")
```

---

### `spr(id, x, y [, transparent [, flipH [, flipV]]])`
Draws a sprite. Pixels matching `transparent` are skipped (default `0x0000`).
```lua
spr(0, px, py)                             -- no transparency
spr(0, px, py, rgb(0, 0, 0))              -- black = transparent
spr(0, px, py, rgb(0, 0, 0), true)        -- flip horizontal
spr(0, px, py, rgb(0, 0, 0), false, true) -- flip vertical
```

---

### `spr_scale(id, x, y, scale [, transparent])`
Draws a sprite scaled by an integer factor (`scale >= 1`).
```lua
spr_scale(0, 56, 56, 4)   -- renders as 64×64 px
```

---

## Camera

Stores a scroll offset that scripts can read to apply manual camera translation.  
It does **not** automatically offset draw calls — apply `cx`/`cy` yourself when drawing world objects.

### `camera(x, y)`
Sets the camera offset.
```lua
camera(camX, camY)
```

---

### `camera_get()` → `x, y`
Returns the current camera offset.
```lua
local cx, cy = camera_get()
```

---

## Input

Buttons are updated automatically on every `tick()` call.

| Index | Button |
|-------|--------|
| `0`   | UP     |
| `1`   | DOWN   |
| `2`   | LEFT   |
| `3`   | RIGHT  |
| `4`   | A      |
| `5`   | B      |
| `6`   | HOME   |

---

### `btn(b)` → `bool`
Returns `true` while button `b` is held down.
```lua
if btn(0) then y = y - 1 end
```

---

### `btnp(b)` → `bool`
Returns `true` only on the first frame the button is pressed.
```lua
if btnp(4) then jump() end
```

---

### `btnr(b)` → `bool`
Returns `true` only on the frame the button is released.
```lua
if btnr(4) then land() end
```

---

## Time

### `millis()` → `number`
Returns milliseconds elapsed since boot.
```lua
local t = millis()
```

---

### `delay(ms)`
Blocks execution for the given number of milliseconds.
```lua
delay(500)
```

---

### `dt()` → `number`
Returns the time in seconds between the last two `tick()` calls. Use for frame-independent movement.
```lua
x = x + speed * dt()
```

---

### `tick()` → `bool`
Must be called once per frame. Updates `dt()`, reads buttons, and checks for the HOME exit signal.  
Returns `true` when the script should exit (HOME held or exit flag set).
```lua
while not tick() do
  cls()
  -- game logic here
  flip()
end
```

---

### `shouldExit()` → `bool`
Returns `true` if the exit flag has been raised (HOME button pressed).
```lua
if shouldExit() then return end
```

---

## Math

### `rnd([lo [, hi]])` → `float`
Returns a random float in `[lo, hi]`. Defaults to `[0, 1]`.
```lua
local x = rnd(0, 128)
```

---

### `rnd_int(lo, hi)` → `int`
Returns a random integer in `[lo, hi]` inclusive.
```lua
local n = rnd_int(1, 6)
```

---

### `clamp(v, lo, hi)` → `number`
Clamps `v` between `lo` and `hi`.
```lua
x = clamp(x, 0, 112)
```

---

### `lerp(a, b, t)` → `number`
Linear interpolation between `a` and `b` by factor `t` (0–1).
```lua
local cx = lerp(cx, tx, 0.1)
```

---

### `sin_deg(degrees)` → `number`
Sine of an angle given in degrees.
```lua
local y = 64 + sin_deg(angle) * 30
```

---

### `cos_deg(degrees)` → `number`
Cosine of an angle given in degrees.
```lua
local x = 64 + cos_deg(angle) * 30
```

---

### `atan2_deg(y, x)` → `number`
Arctangent of `y/x` returned in degrees.
```lua
local angle = atan2_deg(dy, dx)
```

---

### `dist(x1, y1, x2, y2)` → `number`
Euclidean distance between two points.
```lua
if dist(px, py, ex, ey) < 8 then hit() end
```

---

### `aabb(x1,y1,w1,h1, x2,y2,w2,h2)` → `bool`
Returns `true` if two axis-aligned rectangles overlap.
```lua
if aabb(px, py, 16, 16, ex, ey, 16, 16) then collide() end
```

---

## SD / File System

### `file_exists(path)` → `bool`
Returns `true` if the file or directory exists on the SD card.
```lua
if file_exists("/save.dat") then load() end
```

---

### `file_read(path)` → `string | nil`
Reads the entire file and returns its contents as a string. Returns `nil` on error.
```lua
local data = file_read("/config.txt")
```

---

### `file_write(path, data [, append])` → `bool`
Writes a string to a file. Pass `true` as the third argument to append instead of overwrite.
```lua
file_write("/log.txt", "score=100\n", true)
```

---

### `file_delete(path)` → `bool`
Deletes a file from the SD card.
```lua
file_delete("/temp.dat")
```

---

### `dir([path])` → `table`
Returns a table of filenames (full paths) in the given directory. Defaults to `"/"`.
```lua
local files = dir("/maps")
for _, name in ipairs(files) do
  log(name)
end
```

---

## System

### `log(str)`
Prints a string to the Serial monitor (115200 baud).
```lua
log("player x = " .. px)
```

---

### `free_heap()` → `number`
Returns available internal heap memory in bytes.
```lua
log("heap: " .. free_heap())
```

---

## WiFi

### `wifi_connect(ssid, password [, timeout_ms])` → `bool`
Connects to a WiFi network. `timeout_ms` defaults to `10000`. Returns `true` on success.
```lua
local ok = wifi_connect("MyNet", "secret", 8000)
```

---

### `wifi_disconnect()`
Disconnects from the current WiFi network.
```lua
wifi_disconnect()
```

---

### `wifi_status()` → `string`
Returns the current connection status: `"connected"`, `"connecting"`, `"failed"`, or `"disconnected"`.
```lua
if wifi_status() == "connected" then ... end
```

---

### `wifi_ip()` → `string`
Returns the device IP address as a string, or `""` if not connected.
```lua
log(wifi_ip())
```

---

### `wifi_rssi()` → `number`
Returns the signal strength in dBm.
```lua
log("signal: " .. wifi_rssi() .. " dBm")
```

---

### `wifi_scan()` → `table`
Scans for nearby networks and returns a table of `{ssid, rssi, secure}` entries.
```lua
local nets = wifi_scan()
for _, n in ipairs(nets) do
  log(n.ssid .. "  " .. n.rssi .. " dBm")
end
```

---

## HTTP

> WiFi must be connected before making HTTP requests.

### `http_get(url [, headers])` → `body, status_code`
Performs an HTTP GET request. `headers` is an optional table of key/value strings.
```lua
local body, code = http_get("http://example.com/api")
if code == 200 then log(body) end
```

With custom headers:
```lua
local body, code = http_get("http://api.example.com/data", {
  ["Authorization"] = "Bearer token123"
})
```

---

### `http_post(url, body [, content_type [, headers]])` → `response, status_code`
Performs an HTTP POST request. `content_type` defaults to `"application/json"`.
```lua
local resp, code = http_post(
  "http://api.example.com/score",
  '{"score":100}')
```

---

## JSON

### `json_decode(str)` → `table | nil, errmsg`
Parses a JSON string into a Lua table. Returns `nil` and an error message on failure.
```lua
local data, err = json_decode('{"x":10,"y":20}')
if data then log(data.x) end
```

---

### `json_encode(table)` → `string`
Serializes a Lua table to a JSON string. Arrays are detected automatically.
```lua
local s = json_encode({score = 100, level = 3})
file_write("/save.json", s)
```

---

## PSRAM Buffers

Up to **8 large byte buffers** can be allocated in PSRAM (falls back to heap if unavailable).  
Useful for level maps, large assets, or any binary data that would otherwise exhaust internal RAM.  
A **handle** is the integer slot index (`0`–`7`) returned when a buffer is created.  
All buffers left open by a script are freed automatically when the script exits.

---

### `psram_alloc(size)` → `handle | nil, errmsg`
Allocates a zero-filled buffer of `size` bytes.
```lua
local buf, err = psram_alloc(4096)
if not buf then log(err) end
```

---

### `psram_load(path)` → `handle | nil, errmsg`
Allocates a buffer and fills it with the contents of a file from the SD card.
```lua
local buf = psram_load("/maps/level1.bin")
```

---

### `psram_free(handle)`
Releases the buffer and frees its memory immediately.
```lua
psram_free(buf)
```

---

### `psram_size(handle)` → `number`
Returns the size of the buffer in bytes.
```lua
log("buffer size: " .. psram_size(buf))
```

---

### `psram_get(handle, offset)` → `byte`
Reads a single byte at the given offset (0-based).
```lua
local b = psram_get(buf, 0)
```

---

### `psram_set(handle, offset, byte)`
Writes a single byte at the given offset.
```lua
psram_set(buf, 0, 255)
```

---

### `psram_save(handle, path)` → `bool`
Writes the entire buffer to a file on the SD card.
```lua
local ok = psram_save(buf, "/maps/level1.bin")
```

---

### `free_psram()` → `number`
Returns the number of free PSRAM bytes.
```lua
log("free psram: " .. free_psram())
```

---

## Game Loop Template

```lua
local x, y = 56, 56
local speed = 60  -- pixels per second

while not tick() do
  cls()

  if btn(0) then y = y - speed * dt() end
  if btn(1) then y = y + speed * dt() end
  if btn(2) then x = x - speed * dt() end
  if btn(3) then x = x + speed * dt() end

  x = clamp(x, 0, 112)
  y = clamp(y, 0, 112)

  spr(0, x, y, rgb(0, 0, 0))

  flip()
end
```

---

## Button Index Quick Reference

```
0 = UP
1 = DOWN
2 = LEFT
3 = RIGHT
4 = A
5 = B
6 = HOME  ← also triggers script exit
```
