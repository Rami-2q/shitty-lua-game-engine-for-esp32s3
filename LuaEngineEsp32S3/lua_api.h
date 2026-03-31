#pragma once

#include <LuaWrapper.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>   // https://arduinojson.org/

// Globals from main sketch
extern TFT_eSPI     tft;
extern TFT_eSprite  canvas;

// Button state struct
struct Button { uint8_t pin; bool prev, pressed, held; };
extern Button btnUp, btnDown, btnLeft, btnRight, btnA, btnB, btnHome;
void readButtons();

// Custom pixel font renderer
void drawText(int x, int y, const char* str, uint16_t color, int sz);

// Sprite bank settings
#define MAX_SPRITES  32
#define SPR_W        16
#define SPR_H        16
#define SPR_PIXELS   (SPR_W * SPR_H)

static uint16_t (*spriteBank)[SPR_PIXELS] = nullptr;
static bool     spriteDefined[MAX_SPRITES];

// Lua VM state
static LuaWrapper lua;
static bool luaRunning  = false;
static bool luaExitFlag = false;

// Delta time for frame-independent movement
static uint32_t _lastFrameMs = 0;
static float    _deltaTime   = 0.0f;

// Button state for press/release detection
static bool _btnPrev[7]    = {};
static bool _btnCurrent[7] = {};

// Returns true if button index b is held down
static inline bool _btnHeld(int b) {
    switch(b){
        case 0: return btnUp.held;
        case 1: return btnDown.held;
        case 2: return btnLeft.held;
        case 3: return btnRight.held;
        case 4: return btnA.held;
        case 5: return btnB.held;
        case 6: return btnHome.held;
    }
    return false;
}

// Reads buttons and updates prev/current state arrays
static void updateInputState() {
    readButtons();
    for(int i = 0; i < 7; i++) {
        _btnPrev[i]    = _btnCurrent[i];
        _btnCurrent[i] = _btnHeld(i);
    }
}

// --- Graphics ---

// cls([color]) — clear screen
static int lua_cls(lua_State* L) {
    uint16_t c = luaL_optnumber(L, 1, TFT_BLACK);
    canvas.fillSprite(c);
    return 0;
}

// flip() — push canvas to screen
static int lua_flip(lua_State* L) {
    canvas.pushSprite(0, 0); return 0;
}

// pset(x, y, color) — draw a single pixel
static int lua_pset(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    uint16_t c=(uint16_t)luaL_checknumber(L,3);
    canvas.drawPixel(x,y,c); return 0;
}

// pget(x, y) — read pixel color
static int lua_pget(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    lua_pushnumber(L, (lua_Number)canvas.readPixel(x,y)); return 1;
}

// line(x0, y0, x1, y1, color) — draw a line
static int lua_line(lua_State* L) {
    int x0=(int)luaL_checknumber(L,1), y0=(int)luaL_checknumber(L,2);
    int x1=(int)luaL_checknumber(L,3), y1=(int)luaL_checknumber(L,4);
    uint16_t c=(uint16_t)luaL_checknumber(L,5);
    canvas.drawLine(x0,y0,x1,y1,c); return 0;
}

// rect(x, y, w, h, color) — draw rectangle outline
static int lua_rect(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    int w=(int)luaL_checknumber(L,3), h=(int)luaL_checknumber(L,4);
    uint16_t c=(uint16_t)luaL_checknumber(L,5);
    canvas.drawRect(x,y,w,h,c); return 0;
}

// frect(x, y, w, h, color) — draw filled rectangle
static int lua_frect(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    int w=(int)luaL_checknumber(L,3), h=(int)luaL_checknumber(L,4);
    uint16_t c=(uint16_t)luaL_checknumber(L,5);
    canvas.fillRect(x,y,w,h,c); return 0;
}

// circ(x, y, r, color) — draw circle outline
static int lua_circ(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    int r=(int)luaL_checknumber(L,3);
    uint16_t c=(uint16_t)luaL_checknumber(L,4);
    canvas.drawCircle(x,y,r,c); return 0;
}

// fcirc(x, y, r, color) — draw filled circle
static int lua_fcirc(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    int r=(int)luaL_checknumber(L,3);
    uint16_t c=(uint16_t)luaL_checknumber(L,4);
    canvas.fillCircle(x,y,r,c); return 0;
}

// tri(x0,y0,x1,y1,x2,y2,color) — draw triangle outline
static int lua_tri(lua_State* L) {
    int x0=(int)luaL_checknumber(L,1), y0=(int)luaL_checknumber(L,2);
    int x1=(int)luaL_checknumber(L,3), y1=(int)luaL_checknumber(L,4);
    int x2=(int)luaL_checknumber(L,5), y2=(int)luaL_checknumber(L,6);
    uint16_t c=(uint16_t)luaL_checknumber(L,7);
    canvas.drawTriangle(x0,y0,x1,y1,x2,y2,c); return 0;
}

// ftri(x0,y0,x1,y1,x2,y2,color) — filled triangle
static int lua_ftri(lua_State* L) {
    int x0=(int)luaL_checknumber(L,1), y0=(int)luaL_checknumber(L,2);
    int x1=(int)luaL_checknumber(L,3), y1=(int)luaL_checknumber(L,4);
    int x2=(int)luaL_checknumber(L,5), y2=(int)luaL_checknumber(L,6);
    uint16_t c=(uint16_t)luaL_checknumber(L,7);
    canvas.fillTriangle(x0,y0,x1,y1,x2,y2,c); return 0;
}

// print(x, y, str, color, [size]) — draw text
static int lua_print(lua_State* L) {
    int x=(int)luaL_checknumber(L,1), y=(int)luaL_checknumber(L,2);
    const char* str=luaL_checkstring(L,3);
    uint16_t c=(uint16_t)luaL_checknumber(L,4);
    int sz=(int)luaL_optnumber(L,5,1);
    drawText(x,y,str,c,sz); return 0;
}

// measure_text(str, [size]) -> pixel width
static int lua_measure_text(lua_State* L) {
    const char* str=luaL_checkstring(L,1);
    int sz=(int)luaL_optnumber(L,2,1);
    canvas.setTextSize(sz);
    int16_t w = canvas.textWidth(str);
    lua_pushnumber(L,(lua_Number)w); return 1;
}

// rgb(r, g, b) -> 565 color value
static int lua_rgb(lua_State* L) {
    int r=(int)luaL_checknumber(L,1);
    int g=(int)luaL_checknumber(L,2);
    int b=(int)luaL_checknumber(L,3);
    lua_pushnumber(L,(lua_Number)tft.color565(r,g,b)); return 1;
}

// rgb_split(color) -> r, g, b (5/6/5 bit components)
static int lua_rgb_split(lua_State* L) {
    uint16_t c=(uint16_t)luaL_checknumber(L,1);
    lua_pushnumber(L, (c >> 11) & 0x1F);
    lua_pushnumber(L, (c >> 5)  & 0x3F);
    lua_pushnumber(L,  c        & 0x1F);
    return 3;
}

// spr_define(id, pixels_table) — load sprite from Lua table
static int lua_spr_define(lua_State* L) {
    int id = (int)luaL_checknumber(L, 1);
    if (id < 0 || id >= MAX_SPRITES || spriteBank == nullptr) {
        lua_pushboolean(L, 0);
        return 1;
    }
    luaL_checktype(L, 2, LUA_TTABLE);
    for (int i = 0; i < SPR_PIXELS; i++) {
        lua_rawgeti(L, 2, i + 1);
        spriteBank[id][i] = (uint16_t)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    spriteDefined[id] = true;
    lua_pushboolean(L, 1);
    return 1;
}

// spr_load(id, path) — load sprite from binary file on SD
static int lua_spr_load(lua_State* L) {
    int id=(int)luaL_checknumber(L,1);
    const char* path=luaL_checkstring(L,2);
    if(id<0||id>=MAX_SPRITES){lua_pushboolean(L,0);return 1;}
    File f=SD.open(path);
    if(!f){lua_pushboolean(L,0);return 1;}
    for(int i=0;i<SPR_PIXELS;i++){
        uint8_t lo=f.read(), hi=f.read();
        spriteBank[id][i]=(uint16_t)(lo|(hi<<8));
    }
    f.close();
    spriteDefined[id]=true;
    lua_pushboolean(L,1); return 1;
}

// spr(id, x, y, [transparent], [flipH], [flipV]) — draw sprite
static int lua_spr(lua_State* L) {
    int id=(int)luaL_checknumber(L,1);
    int x=(int)luaL_checknumber(L,2);
    int y=(int)luaL_checknumber(L,3);
    uint16_t transparent=(uint16_t)luaL_optnumber(L,4,0x0000);
    bool flipH=(bool)luaL_optnumber(L,5,0);
    bool flipV=(bool)luaL_optnumber(L,6,0);
    if(id<0||id>=MAX_SPRITES||!spriteDefined[id]) return 0;
    // Fast path when no flip needed — write directly to framebuffer
    if(!flipH && !flipV) {
        uint16_t* dst = (uint16_t*)canvas.getPointer();
        if(dst) {
            int sw = canvas.width();
            for(int row=0; row<SPR_H; row++) {
                uint16_t* src = &spriteBank[id][row * SPR_W];
                uint16_t* d   = dst + (y + row) * sw + x;
                for(int col=0; col<SPR_W; col++) {
                    if(src[col] != transparent) d[col] = (src[col] >> 8) | (src[col] << 8);
                }
            }
            return 0;
        }
    }
    // Slow path with optional flip
    for(int row=0;row<SPR_H;row++) {
        int srcRow = flipV ? (SPR_H-1-row) : row;
        for(int col=0;col<SPR_W;col++){
            int srcCol = flipH ? (SPR_W-1-col) : col;
            uint16_t c=spriteBank[id][srcRow*SPR_W+srcCol];
            if(c!=transparent) canvas.drawPixel(x+col,y+row,c);
        }
    }
    return 0;
}

// spr_scale(id, x, y, scale, [transparent]) — draw sprite scaled up
static int lua_spr_scale(lua_State* L) {
    int id=(int)luaL_checknumber(L,1);
    int x=(int)luaL_checknumber(L,2);
    int y=(int)luaL_checknumber(L,3);
    int sc=(int)luaL_checknumber(L,4);
    uint16_t transparent=(uint16_t)luaL_optnumber(L,5,0x0000);
    if(id<0||id>=MAX_SPRITES||!spriteDefined[id]||sc<1) return 0;
    for(int row=0;row<SPR_H;row++)
        for(int col=0;col<SPR_W;col++){
            uint16_t c=spriteBank[id][row*SPR_W+col];
            if(c!=transparent)
                canvas.fillRect(x+col*sc, y+row*sc, sc, sc, c);
        }
    return 0;
}

// Camera scroll offset
static int _camX = 0, _camY = 0;

// camera(x, y) — set scroll offset
static int lua_camera(lua_State* L) {
    _camX=(int)luaL_checknumber(L,1);
    _camY=(int)luaL_checknumber(L,2);
    return 0;
}

// camera_get() -> x, y
static int lua_camera_get(lua_State* L) {
    lua_pushnumber(L,_camX);
    lua_pushnumber(L,_camY);
    return 2;
}

// --- Input ---

// btn(b) — true while button b is held
static int lua_btn(lua_State* L) {
    int b=(int)luaL_checknumber(L,1);
    lua_pushboolean(L, _btnCurrent[b] ? 1 : 0); return 1;
}

// btnp(b) — true only on the first frame the button is pressed
static int lua_btnp(lua_State* L) {
    int b=(int)luaL_checknumber(L,1);
    bool pressed = _btnCurrent[b] && !_btnPrev[b];
    lua_pushboolean(L, pressed ? 1 : 0); return 1;
}

// btnr(b) — true only on the frame the button is released
static int lua_btnr(lua_State* L) {
    int b=(int)luaL_checknumber(L,1);
    bool released = !_btnCurrent[b] && _btnPrev[b];
    lua_pushboolean(L, released ? 1 : 0); return 1;
}

// --- Time ---

// millis() -> ms since boot
static int lua_millis(lua_State* L) {
    lua_pushnumber(L,(lua_Number)millis()); return 1;
}

// delay(ms)
static int lua_delay(lua_State* L) {
    delay((int)luaL_checknumber(L,1)); return 0;
}

// dt() -> seconds since last tick
static int lua_dt(lua_State* L) {
    lua_pushnumber(L,(lua_Number)_deltaTime); return 1;
}

// tick() — call once per frame; updates dt, buttons, returns true if should exit
static int lua_tick(lua_State* L) {
    uint32_t now = millis();
    _deltaTime = (_lastFrameMs == 0) ? 0.0f : (now - _lastFrameMs) / 1000.0f;
    _lastFrameMs = now;
    updateInputState();
    if(_btnCurrent[6]) luaExitFlag = true;
    lua_pushboolean(L, luaExitFlag ? 1 : 0);
    return 1;
}

// shouldExit() — true if the script should stop
static int lua_shouldExit(lua_State* L) {
    lua_pushboolean(L, luaExitFlag ? 1 : 0); return 1;
}

// --- Math ---

// rnd([lo], [hi]) -> random float
static int lua_rnd(lua_State* L) {
    float lo=(float)luaL_optnumber(L,1,0);
    float hi=(float)luaL_optnumber(L,2,1);
    float r = lo + (float)random(100000) / 100000.0f * (hi - lo);
    lua_pushnumber(L,(lua_Number)r); return 1;
}

// rnd_int(lo, hi) -> random integer in [lo, hi]
static int lua_rnd_int(lua_State* L) {
    int lo=(int)luaL_checknumber(L,1);
    int hi=(int)luaL_checknumber(L,2);
    lua_pushnumber(L,(lua_Number)random(lo, hi+1)); return 1;
}

// clamp(v, lo, hi) -> clamped value
static int lua_clamp(lua_State* L) {
    float v=(float)luaL_checknumber(L,1);
    float lo=(float)luaL_checknumber(L,2);
    float hi=(float)luaL_checknumber(L,3);
    if(v<lo) v=lo; if(v>hi) v=hi;
    lua_pushnumber(L,(lua_Number)v); return 1;
}

// lerp(a, b, t) -> linear interpolation
static int lua_lerp(lua_State* L) {
    float a=(float)luaL_checknumber(L,1);
    float b=(float)luaL_checknumber(L,2);
    float t=(float)luaL_checknumber(L,3);
    lua_pushnumber(L,(lua_Number)(a+(b-a)*t)); return 1;
}

// sin_deg(degrees) -> sine
static int lua_sin_deg(lua_State* L) {
    float d=(float)luaL_checknumber(L,1);
    lua_pushnumber(L,(lua_Number)sin(d*DEG_TO_RAD)); return 1;
}

// cos_deg(degrees) -> cosine
static int lua_cos_deg(lua_State* L) {
    float d=(float)luaL_checknumber(L,1);
    lua_pushnumber(L,(lua_Number)cos(d*DEG_TO_RAD)); return 1;
}

// atan2_deg(y, x) -> angle in degrees
static int lua_atan2_deg(lua_State* L) {
    float y=(float)luaL_checknumber(L,1);
    float x=(float)luaL_checknumber(L,2);
    lua_pushnumber(L,(lua_Number)(atan2(y,x)*RAD_TO_DEG)); return 1;
}

// dist(x1, y1, x2, y2) -> Euclidean distance
static int lua_dist(lua_State* L) {
    float x1=(float)luaL_checknumber(L,1), y1=(float)luaL_checknumber(L,2);
    float x2=(float)luaL_checknumber(L,3), y2=(float)luaL_checknumber(L,4);
    float dx=x2-x1, dy=y2-y1;
    lua_pushnumber(L,(lua_Number)sqrt(dx*dx+dy*dy)); return 1;
}

// aabb(x1,y1,w1,h1, x2,y2,w2,h2) -> true if rects overlap
static int lua_aabb(lua_State* L) {
    float x1=(float)luaL_checknumber(L,1), y1=(float)luaL_checknumber(L,2);
    float w1=(float)luaL_checknumber(L,3), h1=(float)luaL_checknumber(L,4);
    float x2=(float)luaL_checknumber(L,5), y2=(float)luaL_checknumber(L,6);
    float w2=(float)luaL_checknumber(L,7), h2=(float)luaL_checknumber(L,8);
    bool hit = !(x1+w1<=x2 || x2+w2<=x1 || y1+h1<=y2 || y2+h2<=y1);
    lua_pushboolean(L, hit?1:0); return 1;
}

// --- SD / File system ---

// file_exists(path) -> bool
static int lua_file_exists(lua_State* L) {
    const char* path=luaL_checkstring(L,1);
    lua_pushboolean(L, SD.exists(path)?1:0); return 1;
}

// file_read(path) -> string or nil
static int lua_file_read(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    File f = SD.open(path);
    if (!f) { lua_pushnil(L); return 1; }

    size_t sz = f.size();
    // Try PSRAM first to save internal RAM
    char* tempBuf = nullptr;
    if (ESP.getPsramSize() > 0)
        tempBuf = (char*)ps_malloc(sz + 1);
    if (!tempBuf)
        tempBuf = (char*)malloc(sz + 1);
    if (!tempBuf) { f.close(); lua_pushnil(L); return 1; }

    f.read((uint8_t*)tempBuf, sz);
    tempBuf[sz] = '\0';
    f.close();

    lua_pushstring(L, tempBuf);
    free(tempBuf);
    return 1;
}

// file_write(path, data, [append]) -> bool
static int lua_file_write(lua_State* L) {
    const char* path=luaL_checkstring(L,1);
    const char* data=luaL_checkstring(L,2);
    bool append=(bool)luaL_optnumber(L,3,0);
    File f=SD.open(path, append ? FILE_APPEND : FILE_WRITE);
    if(!f){ lua_pushboolean(L,0); return 1; }
    f.print(data);
    f.close();
    lua_pushboolean(L,1); return 1;
}

// file_delete(path) -> bool
static int lua_file_delete(lua_State* L) {
    const char* path=luaL_checkstring(L,1);
    lua_pushboolean(L, SD.remove(path)?1:0); return 1;
}

// dir([path]) -> table of filenames
static int lua_dir(lua_State* L) {
    const char* path=luaL_optstring(L,1,"/");
    File dir=SD.open(path);
    lua_newtable(L);
    int idx=1;
    if(dir && dir.isDirectory()){
        File entry;
        while((entry=dir.openNextFile())){
            lua_pushstring(L,entry.name());
            lua_rawseti(L,-2,idx++);
            entry.close();
        }
        dir.close();
    }
    return 1;
}

// --- System ---

// log(str) — print to serial
static int lua_log(lua_State* L) {
    Serial.println(luaL_checkstring(L,1)); return 0;
}

// free_heap() -> bytes of free heap
static int lua_free_heap(lua_State* L) {
    lua_pushnumber(L,(lua_Number)ESP.getFreeHeap()); return 1;
}

// --- WiFi ---

// wifi_connect(ssid, pass, [timeout_ms]) -> bool
static int lua_wifi_connect(lua_State* L) {
    const char* ssid=luaL_checkstring(L,1);
    const char* pass=luaL_checkstring(L,2);
    int timeout=(int)luaL_optnumber(L,3,10000);
    WiFi.begin(ssid, pass);
    uint32_t t0=millis();
    while(WiFi.status()!=WL_CONNECTED && (millis()-t0)<(uint32_t)timeout)
        delay(100);
    lua_pushboolean(L, WiFi.status()==WL_CONNECTED ? 1 : 0);
    return 1;
}

// wifi_disconnect()
static int lua_wifi_disconnect(lua_State* L) {
    WiFi.disconnect(true); return 0;
}

// wifi_status() -> "connected" | "connecting" | "failed" | "disconnected"
static int lua_wifi_status(lua_State* L) {
    switch(WiFi.status()){
        case WL_CONNECTED:      lua_pushstring(L,"connected");    break;
        case WL_IDLE_STATUS:    lua_pushstring(L,"connecting");   break;
        case WL_CONNECT_FAILED: lua_pushstring(L,"failed");       break;
        default:                lua_pushstring(L,"disconnected"); break;
    }
    return 1;
}

// wifi_ip() -> IP string or ""
static int lua_wifi_ip(lua_State* L) {
    if(WiFi.status()==WL_CONNECTED)
        lua_pushstring(L, WiFi.localIP().toString().c_str());
    else
        lua_pushstring(L,"");
    return 1;
}

// wifi_rssi() -> signal strength in dBm
static int lua_wifi_rssi(lua_State* L) {
    lua_pushnumber(L,(lua_Number)WiFi.RSSI()); return 1;
}

// http_get(url, [headers_table]) -> body, status_code
static int lua_http_get(lua_State* L) {
    const char* url=luaL_checkstring(L,1);
    HTTPClient http;
    http.begin(url);
    if(lua_istable(L,2)){
        lua_pushnil(L);
        while(lua_next(L,2)!=0){
            const char* k=lua_tostring(L,-2);
            const char* v=lua_tostring(L,-1);
            if(k&&v) http.addHeader(k,v);
            lua_pop(L,1);
        }
    }
    int code=http.GET();
    if(code>0){
        String body=http.getString();
        lua_pushstring(L,body.c_str());
    } else {
        lua_pushstring(L,"");
    }
    lua_pushnumber(L,(lua_Number)code);
    http.end();
    return 2;
}

// http_post(url, body, [content_type], [headers]) -> response, status_code
static int lua_http_post(lua_State* L) {
    const char* url=luaL_checkstring(L,1);
    const char* body=luaL_checkstring(L,2);
    const char* ct=luaL_optstring(L,3,"application/json");
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", ct);
    if(lua_istable(L,4)){
        lua_pushnil(L);
        while(lua_next(L,4)!=0){
            const char* k=lua_tostring(L,-2);
            const char* v=lua_tostring(L,-1);
            if(k&&v) http.addHeader(k,v);
            lua_pop(L,1);
        }
    }
    int code=http.POST((uint8_t*)body, strlen(body));
    if(code>0){
        String resp=http.getString();
        lua_pushstring(L,resp.c_str());
    } else {
        lua_pushstring(L,"");
    }
    lua_pushnumber(L,(lua_Number)code);
    http.end();
    return 2;
}

// json_decode(str) -> Lua table or nil, error
static int lua_json_decode(lua_State* L) {
    const char* str=luaL_checkstring(L,1);
    DynamicJsonDocument doc(4096);
    DeserializationError err=deserializeJson(doc,str);
    if(err){ lua_pushnil(L); lua_pushstring(L,err.c_str()); return 2; }

    std::function<void(JsonVariant)> push = [&](JsonVariant v){
        if(v.is<JsonObject>()){
            lua_newtable(L);
            for(JsonPair kv : v.as<JsonObject>()){
                lua_pushstring(L, kv.key().c_str());
                push(kv.value());
                lua_settable(L,-3);
            }
        } else if(v.is<JsonArray>()){
            lua_newtable(L);
            int i=1;
            for(JsonVariant el : v.as<JsonArray>()){
                push(el);
                lua_rawseti(L,-2,i++);
            }
        } else if(v.is<bool>()){
            lua_pushboolean(L, v.as<bool>() ? 1 : 0);
        } else if(v.is<float>()){
            lua_pushnumber(L,(lua_Number)v.as<float>());
        } else {
            lua_pushstring(L, v.as<const char*>() ? v.as<const char*>() : "");
        }
    };
    push(doc.as<JsonVariant>());
    return 1;
}

// json_encode(table) -> JSON string
static int lua_json_encode(lua_State* L) {
    luaL_checktype(L,1,LUA_TTABLE);
    DynamicJsonDocument doc(4096);

    // Check if the table looks like an array
    lua_pushnil(L);
    bool isArray=true;
    while(lua_next(L,1)){
        if(lua_type(L,-2)!=LUA_TNUMBER){ isArray=false; lua_pop(L,2); break; }
        lua_pop(L,1);
    }

    std::function<void(JsonVariant, int)> fill = [&](JsonVariant node, int idx){
        lua_pushnil(L);
        while(lua_next(L, idx)){
            int keyType  = lua_type(L,-2);
            int valType  = lua_type(L,-1);
            auto setVal = [&](JsonVariant dst){
                if(valType==LUA_TNUMBER)       dst.set(lua_tonumber(L,-1));
                else if(valType==LUA_TBOOLEAN)  dst.set((bool)lua_toboolean(L,-1));
                else if(valType==LUA_TSTRING)   dst.set(lua_tostring(L,-1));
                else if(valType==LUA_TNIL)      dst.set((const char*)nullptr);
            };
            if(node.is<JsonArray>()){
                JsonVariant el = node.as<JsonArray>().add<JsonVariant>();
                setVal(el);
            } else {
                const char* k = (keyType==LUA_TSTRING) ? lua_tostring(L,-2) : nullptr;
                if(k) setVal(node[k]);
            }
            lua_pop(L,1);
        }
    };

    if(isArray) doc.to<JsonArray>();
    else        doc.to<JsonObject>();
    fill(doc.as<JsonVariant>(), 1);

    String out;
    serializeJson(doc,out);
    lua_pushstring(L,out.c_str()); return 1;
}

// wifi_scan() -> table of {ssid, rssi, secure}
static int lua_wifi_scan(lua_State* L) {
    int n = WiFi.scanNetworks();
    lua_newtable(L);
    for(int i=0;i<n;i++){
        lua_newtable(L);
        lua_pushstring(L, WiFi.SSID(i).c_str()); lua_setfield(L,-2,"ssid");
        lua_pushnumber(L, WiFi.RSSI(i));          lua_setfield(L,-2,"rssi");
        lua_pushboolean(L, WiFi.encryptionType(i)!=WIFI_AUTH_OPEN ? 1:0);
                                                   lua_setfield(L,-2,"secure");
        lua_rawseti(L,-2,i+1);
    }
    WiFi.scanDelete();
    return 1;
}

// --- PSRAM buffers ---
// Up to 8 large buffers managed from Lua, stored in PSRAM when available

#define PSRAM_MAX_BUFFERS 8

struct PsramBuffer {
    uint8_t* data;
    size_t   size;
};
static PsramBuffer _psramBufs[PSRAM_MAX_BUFFERS] = {};

// Returns the index of the first free slot, or -1
static int _psram_free_slot() {
    for(int i = 0; i < PSRAM_MAX_BUFFERS; i++)
        if(_psramBufs[i].data == nullptr) return i;
    return -1;
}

// psram_load(path) -> handle or nil, errmsg
static int lua_psram_load(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int slot = _psram_free_slot();
    if(slot < 0) { lua_pushnil(L); lua_pushstring(L, "no free psram slots"); return 2; }

    File f = SD.open(path);
    if(!f) { lua_pushnil(L); lua_pushstring(L, "file not found"); return 2; }

    size_t sz = f.size();
    uint8_t* buf = nullptr;
    if(ESP.getPsramSize() > 0)
        buf = (uint8_t*)ps_malloc(sz);
    if(!buf)
        buf = (uint8_t*)malloc(sz);
    if(!buf) { f.close(); lua_pushnil(L); lua_pushstring(L, "out of memory"); return 2; }

    f.read(buf, sz);
    f.close();

    _psramBufs[slot].data = buf;
    _psramBufs[slot].size = sz;
    lua_pushnumber(L, (lua_Number)slot);
    return 1;
}

// psram_alloc(size) -> handle or nil, errmsg
static int lua_psram_alloc(lua_State* L) {
    size_t sz = (size_t)luaL_checknumber(L, 1);
    int slot = _psram_free_slot();
    if(slot < 0) { lua_pushnil(L); lua_pushstring(L, "no free psram slots"); return 2; }

    uint8_t* buf = nullptr;
    if(ESP.getPsramSize() > 0)
        buf = (uint8_t*)ps_calloc(sz, 1);
    if(!buf)
        buf = (uint8_t*)calloc(sz, 1);
    if(!buf) { lua_pushnil(L); lua_pushstring(L, "out of memory"); return 2; }

    _psramBufs[slot].data = buf;
    _psramBufs[slot].size = sz;
    lua_pushnumber(L, (lua_Number)slot);
    return 1;
}

// psram_free(handle) — release buffer
static int lua_psram_free(lua_State* L) {
    int slot = (int)luaL_checknumber(L, 1);
    if(slot < 0 || slot >= PSRAM_MAX_BUFFERS) return 0;
    if(_psramBufs[slot].data) {
        free(_psramBufs[slot].data);
        _psramBufs[slot].data = nullptr;
        _psramBufs[slot].size = 0;
    }
    return 0;
}

// psram_size(handle) -> bytes
static int lua_psram_size(lua_State* L) {
    int slot = (int)luaL_checknumber(L, 1);
    if(slot < 0 || slot >= PSRAM_MAX_BUFFERS || !_psramBufs[slot].data) {
        lua_pushnumber(L, 0); return 1;
    }
    lua_pushnumber(L, (lua_Number)_psramBufs[slot].size);
    return 1;
}

// psram_get(handle, offset) -> byte
static int lua_psram_get(lua_State* L) {
    int slot    = (int)luaL_checknumber(L, 1);
    size_t off  = (size_t)luaL_checknumber(L, 2);
    if(slot < 0 || slot >= PSRAM_MAX_BUFFERS || !_psramBufs[slot].data
       || off >= _psramBufs[slot].size) {
        lua_pushnumber(L, 0); return 1;
    }
    lua_pushnumber(L, (lua_Number)_psramBufs[slot].data[off]);
    return 1;
}

// psram_set(handle, offset, byte)
static int lua_psram_set(lua_State* L) {
    int slot    = (int)luaL_checknumber(L, 1);
    size_t off  = (size_t)luaL_checknumber(L, 2);
    uint8_t val = (uint8_t)luaL_checknumber(L, 3);
    if(slot < 0 || slot >= PSRAM_MAX_BUFFERS || !_psramBufs[slot].data
       || off >= _psramBufs[slot].size) return 0;
    _psramBufs[slot].data[off] = val;
    return 0;
}

// psram_save(handle, path) -> true or false
static int lua_psram_save(lua_State* L) {
    int slot         = (int)luaL_checknumber(L, 1);
    const char* path = luaL_checkstring(L, 2);
    if(slot < 0 || slot >= PSRAM_MAX_BUFFERS || !_psramBufs[slot].data) {
        lua_pushboolean(L, 0); return 1;
    }
    File f = SD.open(path, FILE_WRITE);
    if(!f) { lua_pushboolean(L, 0); return 1; }
    f.write(_psramBufs[slot].data, _psramBufs[slot].size);
    f.close();
    lua_pushboolean(L, 1);
    return 1;
}

// free_psram() -> free PSRAM bytes
static int lua_free_psram(lua_State* L) {
    lua_pushnumber(L, (lua_Number)ESP.getFreePsram());
    return 1;
}

// Registers all Lua API functions with the VM
static void registerLuaAPI() {
    lua.Lua_register("cls",          lua_cls);
    lua.Lua_register("flip",         lua_flip);
    lua.Lua_register("pset",         lua_pset);
    lua.Lua_register("pget",         lua_pget);
    lua.Lua_register("line",         lua_line);
    lua.Lua_register("rect",         lua_rect);
    lua.Lua_register("frect",        lua_frect);
    lua.Lua_register("circ",         lua_circ);
    lua.Lua_register("fcirc",        lua_fcirc);
    lua.Lua_register("tri",          lua_tri);
    lua.Lua_register("ftri",         lua_ftri);
    lua.Lua_register("print",        lua_print);
    lua.Lua_register("measure_text", lua_measure_text);
    lua.Lua_register("rgb",          lua_rgb);
    lua.Lua_register("rgb_split",    lua_rgb_split);
    lua.Lua_register("spr_define",   lua_spr_define);
    lua.Lua_register("spr_load",     lua_spr_load);
    lua.Lua_register("spr",          lua_spr);
    lua.Lua_register("spr_scale",    lua_spr_scale);
    lua.Lua_register("camera",       lua_camera);
    lua.Lua_register("camera_get",   lua_camera_get);
    lua.Lua_register("btn",          lua_btn);
    lua.Lua_register("btnp",         lua_btnp);
    lua.Lua_register("btnr",         lua_btnr);
    lua.Lua_register("millis",       lua_millis);
    lua.Lua_register("delay",        lua_delay);
    lua.Lua_register("dt",           lua_dt);
    lua.Lua_register("tick",         lua_tick);
    lua.Lua_register("shouldExit",   lua_shouldExit);
    lua.Lua_register("rnd",          lua_rnd);
    lua.Lua_register("rnd_int",      lua_rnd_int);
    lua.Lua_register("clamp",        lua_clamp);
    lua.Lua_register("lerp",         lua_lerp);
    lua.Lua_register("sin_deg",      lua_sin_deg);
    lua.Lua_register("cos_deg",      lua_cos_deg);
    lua.Lua_register("atan2_deg",    lua_atan2_deg);
    lua.Lua_register("dist",         lua_dist);
    lua.Lua_register("aabb",         lua_aabb);
    lua.Lua_register("file_exists",  lua_file_exists);
    lua.Lua_register("file_read",    lua_file_read);
    lua.Lua_register("file_write",   lua_file_write);
    lua.Lua_register("file_delete",  lua_file_delete);
    lua.Lua_register("dir",          lua_dir);
    lua.Lua_register("log",          lua_log);
    lua.Lua_register("free_heap",    lua_free_heap);
    lua.Lua_register("wifi_connect",    lua_wifi_connect);
    lua.Lua_register("wifi_disconnect", lua_wifi_disconnect);
    lua.Lua_register("wifi_status",     lua_wifi_status);
    lua.Lua_register("wifi_ip",         lua_wifi_ip);
    lua.Lua_register("wifi_rssi",       lua_wifi_rssi);
    lua.Lua_register("wifi_scan",       lua_wifi_scan);
    lua.Lua_register("http_get",        lua_http_get);
    lua.Lua_register("http_post",       lua_http_post);
    lua.Lua_register("json_decode",     lua_json_decode);
    lua.Lua_register("json_encode",     lua_json_encode);
    lua.Lua_register("psram_load",   lua_psram_load);
    lua.Lua_register("psram_alloc",  lua_psram_alloc);
    lua.Lua_register("psram_free",   lua_psram_free);
    lua.Lua_register("psram_size",   lua_psram_size);
    lua.Lua_register("psram_get",    lua_psram_get);
    lua.Lua_register("psram_set",    lua_psram_set);
    lua.Lua_register("psram_save",   lua_psram_save);
    lua.Lua_register("free_psram",   lua_free_psram);
}

// Loads and runs a Lua script from SD card
static void runLuaScript(const char* path) {

    // 1. Allocate sprite bank once, fall back to SRAM if no PSRAM
    size_t bankSize = MAX_SPRITES * SPR_PIXELS * sizeof(uint16_t);

    if (spriteBank == nullptr) {
        if (ESP.getPsramSize() > 0)
            spriteBank = (uint16_t (*)[SPR_PIXELS]) ps_malloc(bankSize);
        if (spriteBank == nullptr) {
            Serial.println("No PSRAM, using SRAM for sprites...");
            spriteBank = (uint16_t (*)[SPR_PIXELS]) malloc(bankSize);
        }
        if (spriteBank == nullptr) {
            Serial.println("FATAL: not enough memory for spriteBank");
            return;
        }
    }
    // Clear sprites from the previous script
    memset(spriteBank, 0, bankSize);

    // 2. Reset all state before running
    for (int i = 0; i < MAX_SPRITES; i++) spriteDefined[i] = false;
    luaExitFlag  = false;
    _lastFrameMs = 0;
    _camX = _camY = 0;
    for (int i = 0; i < 7; i++) _btnPrev[i] = _btnCurrent[i] = false;

    // 3. Free any PSRAM buffers left by the previous script
    for (int i = 0; i < PSRAM_MAX_BUFFERS; i++) {
        if (_psramBufs[i].data) {
            free(_psramBufs[i].data);
            _psramBufs[i].data = nullptr;
            _psramBufs[i].size = 0;
        }
    }

    // 4. Show loading screen
    canvas.fillSprite(TFT_BLACK);
    drawText(8, 55, "LOADING LUA...", 0x07FF, 1);
    canvas.pushSprite(0, 0);

    // 5. Open the file
    File f = SD.open(path);
    if (!f) {
        canvas.fillSprite(TFT_BLACK);
        drawText(8, 55, "ERROR: CANT OPEN", 0xF800, 1);
        canvas.pushSprite(0, 0);
        delay(1500);
        return;
    }

    size_t scriptSize = f.size();

    // 6. Allocate script buffer (PSRAM preferred)
    char* scriptBuf = nullptr;
    if (ESP.getPsramSize() > 0)
        scriptBuf = (char*) ps_malloc(scriptSize + 1);
    if (!scriptBuf)
        scriptBuf = (char*) malloc(scriptSize + 1);
    if (!scriptBuf) {
        f.close();
        canvas.fillSprite(TFT_BLACK);
        drawText(8, 55, "ERROR: NO RAM", 0xF800, 1);
        canvas.pushSprite(0, 0);
        delay(1500);
        return;
    }

    // 7. Read the script into memory
    f.read((uint8_t*) scriptBuf, scriptSize);
    scriptBuf[scriptSize] = '\0';
    f.close();

    Serial.printf("Script: %s  |  %u bytes  |  free heap: %u\n",
                  path, scriptSize, ESP.getFreeHeap());

    // 8. Run the Lua script
    luaRunning = true;
    String scriptStr(scriptBuf);
    free(scriptBuf);
    scriptBuf = nullptr;

    String result = lua.Lua_dostring(&scriptStr);
    luaRunning = false;

    // 9. Show error message if something went wrong
    if (result.length() > 0) {
        canvas.fillSprite(TFT_BLACK);
        drawText(2, 30, "LUA ERROR:", 0xF800, 1);
        char errBuf[80];
        strncpy(errBuf, result.c_str(), 79);
        errBuf[79] = '\0';
        int len = strlen(errBuf);
        for (int line = 0; line < 4 && line * 20 < len; line++) {
            char chunk[21];
            strncpy(chunk, errBuf + line * 20, 20);
            chunk[20] = '\0';
            drawText(2, 42 + line * 10, chunk, 0xFB60, 1);
        }
        canvas.pushSprite(0, 0);
        delay(3000);
    }
}
