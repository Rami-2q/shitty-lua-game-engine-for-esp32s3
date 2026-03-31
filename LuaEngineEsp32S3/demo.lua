-- =============================================================
--  DEMO TÉCNICA — LuaAPI completa
--  Navegar escenas: LEFT / RIGHT
--  Salir:           HOME
--  Cada escena testea un grupo de funciones
-- =============================================================

-- ── Constantes de pantalla ───────────────────────────────────
local SW = 128   -- screen width  (ajustar a tu resolución)
local SH = 128   -- screen height

-- ── Paleta básica ────────────────────────────────────────────
local BLACK   = rgb(  0,   0,   0)
local WHITE   = rgb(255, 255, 255)
local RED     = rgb(255,   0,   0)
local GREEN   = rgb(  0, 255,   0)
local BLUE    = rgb(  0,   0, 255)
local YELLOW  = rgb(255, 255,   0)
local CYAN    = rgb(  0, 255, 255)
local MAGENTA = rgb(255,   0, 255)
local ORANGE  = rgb(255, 128,   0)
local GRAY    = rgb(128, 128, 128)
local DKGRAY  = rgb( 40,  40,  40)
local LTBLUE  = rgb( 80, 160, 255)

-- ── Estado global ────────────────────────────────────────────
local scene       = 1
local TOTAL_SCENES = 9
local frame       = 0
local angle       = 0.0

-- ── Sprite 0: carita 16x16 ───────────────────────────────────
local function make_face_sprite()
    local px = {}
    local Y = YELLOW
    local B = BLACK
    local R = RED
    local _ = 0  -- transparente
    -- fila a fila, 16 cols
    local rows = {
        {_,_,_,_,_,Y,Y,Y,Y,Y,Y,_,_,_,_,_},
        {_,_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_,_},
        {_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_},
        {_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_},
        {_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_},
        {Y,Y,Y,B,B,Y,Y,Y,Y,Y,Y,B,B,Y,Y,Y},
        {Y,Y,Y,B,B,Y,Y,Y,Y,Y,Y,B,B,Y,Y,Y},
        {Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
        {Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
        {Y,Y,B,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,B,Y,Y},
        {Y,Y,Y,B,Y,Y,Y,Y,Y,Y,Y,Y,B,Y,Y,Y},
        {Y,Y,Y,Y,R,R,R,R,R,R,R,R,Y,Y,Y,Y},
        {_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_},
        {_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_},
        {_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_},
        {_,_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_,_},
    }
    for r=1,16 do
        for c=1,16 do
            px[#px+1] = rows[r][c]
        end
    end
    spr_define(0, px)
end
make_face_sprite()

-- ── Helper: dibuja el HUD con nombre de escena ───────────────
local SCENE_NAMES = {
    "1: GRAFICOS",
    "2: TEXTO/COLOR",
    "3: SPRITES",
    "4: INPUT",
    "5: TIEMPO/DT",
    "6: MATEMATICAS",
    "7: SD/ARCHIVO",
    "8: SISTEMA",
    "9: WIFI/HTTP",
}

local function draw_hud()
    -- barra superior
    frect(0, 0, SW, 9, DKGRAY)
    local name = SCENE_NAMES[scene]
    local tw   = measure_text(name, 1)
    print(math.floor((SW - tw) / 2), 1, name, CYAN, 1)
    -- indicadores left / right
    print(1,  1, "<", GRAY, 1)
    print(SW-7, 1, ">", GRAY, 1)
end

-- =============================================================
--  ESCENA 1 — GRÁFICOS PRIMITIVOS
--  Prueba: cls, pset, pget, line, rect, frect, circ, fcirc,
--           tri, ftri, flip
-- =============================================================
local function scene_graphics()
    cls(BLACK)

    -- frect y rect
    frect(2, 12, 30, 20, BLUE)
    rect( 2, 12, 30, 20, CYAN)

    -- fcirc y circ
    fcirc(50, 22, 10, MAGENTA)
    circ( 50, 22, 10, WHITE)

    -- ftri y tri
    ftri(75, 32, 95, 12, 115, 32, RED)
    tri( 75, 32, 95, 12, 115, 32, WHITE)

    -- líneas diagonales
    local cols = {RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, MAGENTA}
    for i = 1, 7 do
        line(0, 40 + (i-1)*4, SW-1, 40 + i*4, cols[i])
    end

    -- pset: dibuja una pequeña X con pixels individuales
    local px = 10
    local py = 78
    for i = -3, 3 do
        pset(px + i, py + i, WHITE)
        pset(px + i, py - i, WHITE)
    end

    -- pget: lee el pixel del centro del circulo y muestra su color
    local got = pget(50, 22)
    local gr, gg, gb = rgb_split(got)
    local info = "pget R:" .. gr .. " G:" .. gg
    print(2, 82, info, YELLOW, 1)

    -- camera test: dibuja un rect con offset de camara
    camera(4, 0)
    frect(60, 90, 20, 12, GREEN)
    print(60, 91, "CAM", BLACK, 1)
    camera(0, 0)

    print(2, 106, "LEFT/RIGHT:escena", GRAY, 1)
    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 2 — TEXTO Y COLOR
--  Prueba: print, measure_text, rgb, rgb_split
-- =============================================================
local function scene_text_color()
    cls(BLACK)

    -- Distintos tamaños de texto
    print(2, 12, "SIZE 1", WHITE,  1)
    print(2, 22, "SIZE 2", YELLOW, 2)

    -- Texto centrado usando measure_text
    local msg = "CENTRADO"
    local tw  = measure_text(msg, 1)
    print(math.floor((SW - tw) / 2), 40, msg, GREEN, 1)

    -- Muestra rgb y rgb_split
    local c1 = rgb(255, 100, 50)
    frect(2, 50, 20, 10, c1)
    local r1,g1,b1 = rgb_split(c1)
    print(25, 51, "R:"..r1.." G:"..g1.." B:"..b1, WHITE, 1)

    -- Arcoiris de colores
    local rainbow = {
        rgb(255,0,0), rgb(255,128,0), rgb(255,255,0),
        rgb(0,255,0), rgb(0,0,255),   rgb(128,0,255),
    }
    for i,c in ipairs(rainbow) do
        frect((i-1)*21, 65, 20, 10, c)
    end
    print(2, 78, "rgb() rainbow ^", GRAY, 1)

    -- Degradado manual por lerp de componentes
    print(2, 90, "DEGRADADO:", WHITE, 1)
    for i = 0, SW-1 do
        local t  = i / (SW-1)
        local r2 = math.floor(lerp(255, 0, t))
        local b2 = math.floor(lerp(0, 255, t))
        local gc = rgb(r2, 0, b2)
        pset(i, 100, gc)
        pset(i, 101, gc)
        pset(i, 102, gc)
    end

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 3 — SPRITES
--  Prueba: spr_define, spr (normal/flipH/flipV), spr_scale
-- =============================================================
local sprBounceX = 0
local sprBounceVX = 1.5

local function scene_sprites()
    cls(DKGRAY)

    -- normal
    spr(0, 10, 14, 0)
    print(10, 32, "normal", WHITE, 1)

    -- flip horizontal
    spr(0, 40, 14, 0, 1, 0)
    print(38, 32, "flipH", WHITE, 1)

    -- flip vertical
    spr(0, 72, 14, 0, 0, 1)
    print(70, 32, "flipV", WHITE, 1)

    -- escala x2
    spr_scale(0, 4, 44, 2, 0)
    print(4, 78, "x2", YELLOW, 1)

    -- escala x3 (puede salir del borde, recortado por canvas)
    spr_scale(0, 52, 44, 3, 0)
    print(52, 94, "x3", YELLOW, 1)

    -- sprite con transparencia distinta (fondo verde para ver que no pinta negro)
    frect(2, 100, 20, 20, GREEN)
    spr(0, 2, 100, 0)   -- negro como transparente → carita sobre verde
    print(24, 106, "transp OK", WHITE, 1)

    -- sprite rebotando (usa frame para animar)
    sprBounceX = sprBounceX + sprBounceVX
    if sprBounceX > SW - 20 then sprBounceVX = -math.abs(sprBounceVX) end
    if sprBounceX < 2       then sprBounceVX =  math.abs(sprBounceVX) end

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 4 — INPUT
--  Prueba: btn, btnp, btnr
--  Muestra estado en tiempo real + contador de pulsaciones
-- =============================================================
local pressCount = 0
local releaseCount = 0
local btnNames = {"UP","DN","LT","RT"," A"," B","HM"}

local function scene_input()
    cls(BLACK)

    -- Estado held de cada botón
    print(2, 12, "btn() HELD:", WHITE, 1)
    for i = 0, 6 do
        local held = btn(i)
        local col  = held and GREEN or RED
        local x    = 2 + (i * 18)
        frect(x, 22, 16, 10, col)
        print(x+1, 23, btnNames[i+1], BLACK, 1)
    end

    -- btnp: solo primer frame
    if btnp(4) then   -- botón A
        pressCount = pressCount + 1
    end
    -- btnr: solo frame de soltar
    if btnr(4) then   -- botón A
        releaseCount = releaseCount + 1
    end

    print(2, 38, "A presiones (btnp):", WHITE, 1)
    print(2, 48, tostring(pressCount), YELLOW, 2)

    print(2, 68, "A sueltas  (btnr):", WHITE, 1)
    print(2, 78, tostring(releaseCount), CYAN, 2)

    -- Indicador visual grande del botón A
    if btn(4) then
        fcirc(SW-20, 90, 12, GREEN)
        print(SW-28, 87, "A", BLACK, 2)
    else
        circ(SW-20, 90, 12, GRAY)
        print(SW-28, 87, "A", GRAY, 2)
    end

    print(2, 106, "Pulsá A para contar", GRAY, 1)

    -- Resetear con B
    if btnp(5) then
        pressCount   = 0
        releaseCount = 0
    end
    print(2, 116, "B=reset", GRAY, 1)

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 5 — TIEMPO Y DELTA TIME
--  Prueba: millis, dt, tick (llamado en el loop principal)
-- =============================================================
local dtAcc    = 0.0
local dtFrames = 0
local dtAvg    = 0.0
local rectX    = 0.0   -- objeto que se mueve con dt (velocidad constante px/s)
local RECT_SPD = 30.0  -- px por segundo

local function scene_time()
    cls(BLACK)

    local ms = millis()
    local secs = math.floor(ms / 1000)
    local ms10 = math.floor(ms / 100) % 10

    print(2, 12, "millis():", WHITE, 1)
    print(2, 22, tostring(ms), YELLOW, 2)

    -- dt acumulado para promedio
    local d = dt()
    dtAcc    = dtAcc + d
    dtFrames = dtFrames + 1
    if dtFrames >= 30 then
        dtAvg    = dtAcc / dtFrames
        dtAcc    = 0
        dtFrames = 0
    end

    print(2, 42, "dt() actual:", WHITE, 1)
    local dtStr = string.format("%.4f s", d)
    print(2, 52, dtStr, CYAN, 1)

    print(2, 62, "FPS aprox:", WHITE, 1)
    local fps = (d > 0) and math.floor(1.0 / d) or 0
    print(2, 72, tostring(fps), GREEN, 2)

    -- Rect que se mueve a velocidad constante usando dt
    rectX = rectX + RECT_SPD * d
    if rectX > SW then rectX = -16 end

    frect(math.floor(rectX), 96, 16, 8, ORANGE)
    print(2, 106, "->30px/s con dt", GRAY, 1)

    -- Segundero visual
    local barW = math.floor((ms % 1000) / 1000 * (SW - 4))
    frect(2, 116, barW, 5, LTBLUE)
    rect( 2, 116, SW-4, 5, GRAY)

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 6 — MATEMÁTICAS
--  Prueba: rnd, rnd_int, clamp, lerp, sin_deg, cos_deg,
--          atan2_deg, dist, aabb
-- =============================================================
local mathStars = {}
local function init_stars()
    for i = 1, 20 do
        mathStars[i] = {
            x = rnd(0, SW),
            y = rnd(12, SH),
            r = math.floor(rnd(1, 3)),
            c = rgb(rnd_int(100,255), rnd_int(100,255), rnd_int(100,255))
        }
    end
end
init_stars()

local lerpT  = 0.0
local lerpDir = 1

local function scene_math()
    cls(BLACK)

    -- Estrellas aleatorias (rnd)
    for _, s in ipairs(mathStars) do
        fcirc(math.floor(s.x), math.floor(s.y), s.r, s.c)
    end

    -- Órbita con sin_deg / cos_deg
    local cx, cy = 90, 60
    local orb_r  = 18
    angle = (angle + 2) % 360
    local ox = cx + math.floor(cos_deg(angle) * orb_r)
    local oy = cy + math.floor(sin_deg(angle) * orb_r)
    fcirc(cx, cy, 4, YELLOW)
    fcirc(ox, oy, 3, CYAN)
    -- atan2_deg para mostrar ángulo real
    local angGot = math.floor(atan2_deg(oy - cy, ox - cx))
    print(2, 12, "sin/cos orb "..angGot.."deg", WHITE, 1)

    -- dist entre centro y planeta
    local d = math.floor(dist(cx, cy, ox, oy))
    print(2, 22, "dist="..d.." px", GRAY, 1)

    -- lerp animado (barra que va y vuelve)
    lerpT = lerpT + 0.02 * lerpDir
    if lerpT >= 1.0 then lerpDir = -1 end
    if lerpT <= 0.0 then lerpDir =  1 end
    local lx = math.floor(lerp(4, SW-20, lerpT))
    frect(4, 80, SW-8, 6, DKGRAY)
    fcirc(lx, 83, 4, MAGENTA)
    print(2, 88, "lerp t="..string.format("%.2f",lerpT), WHITE, 1)

    -- clamp demo
    local raw = math.floor(sin_deg(angle) * 50)
    local clamped = math.floor(clamp(raw, -20, 20))
    print(2, 96, "clamp("..raw..",±20)="..clamped, CYAN, 1)

    -- aabb: dos rectángulos, uno fijo, uno en órbita
    local ax, ay, aw, ah = 2, 106, 20, 10
    local bx, by, bw, bh = ox - 56, oy + 50, 14, 10
    local hit = aabb(ax,ay,aw,ah, bx,by,bw,bh)
    frect(ax,ay,aw,ah, hit and RED or GREEN)
    print(ax+1,ay+1, "AABB", BLACK, 1)
    print(2, 118, "aabb="..(hit and "HIT" or "miss"), hit and RED or GRAY, 1)

    -- rnd_int muestra
    print(55, 106, "rnd_int:", WHITE, 1)
    print(55, 116, tostring(rnd_int(0, 99)), YELLOW, 2)

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 7 — SD / ARCHIVO
--  Prueba: file_write, file_exists, file_read, file_delete, dir
-- =============================================================
local sdLog    = {}
local sdDone   = false
local sdLines  = {}

local function scene_sd()
    if not sdDone then
        sdDone = true
        local path = "/demo_test.txt"

        -- file_write (crear)
        local ok = file_write(path, "linea 1\nlinea 2\nlinea 3\n")
        sdLog[#sdLog+1] = "write: "..(ok and "OK" or "FAIL")

        -- file_exists
        local ex = file_exists(path)
        sdLog[#sdLog+1] = "exists: "..(ex and "SI" or "NO")

        -- file_write append
        file_write(path, "linea 4\n", 1)
        sdLog[#sdLog+1] = "append: OK"

        -- file_read
        local content = file_read(path)
        if content then
            sdLog[#sdLog+1] = "read: OK ("..#content.." bytes)"
            -- parsear líneas para mostrar
            for ln in content:gmatch("[^\n]+") do
                sdLines[#sdLines+1] = ln
            end
        else
            sdLog[#sdLog+1] = "read: FAIL"
        end

        -- dir raiz
        local entries = dir("/")
        sdLog[#sdLog+1] = "dir /: "..#entries.." entradas"

        -- file_delete
        local del = file_delete(path)
        sdLog[#sdLog+1] = "delete: "..(del and "OK" or "FAIL")
    end

    cls(BLACK)
    print(2, 12, "SD / ARCHIVO", WHITE, 1)

    -- log de operaciones
    for i, ln in ipairs(sdLog) do
        local col = ln:find("FAIL") and RED or GREEN
        print(2, 10 + i*10, ln, col, 1)
    end

    -- contenido leído
    if #sdLines > 0 then
        print(2, 82, "Contenido leido:", CYAN, 1)
        for i = 1, math.min(4, #sdLines) do
            print(2, 90 + (i-1)*9, sdLines[i], YELLOW, 1)
        end
    end

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 8 — SISTEMA
--  Prueba: log, free_heap, millis
-- =============================================================
local heapHistory = {}
local heapTimer   = 0

local function scene_system()
    cls(BLACK)

    -- free_heap
    local heap = free_heap()
    heapTimer = heapTimer + dt()
    if heapTimer > 0.5 then
        heapTimer = 0
        heapHistory[#heapHistory+1] = heap
        if #heapHistory > 50 then table.remove(heapHistory, 1) end
        log("DEMO heap=" .. heap)   -- aparece en Serial Monitor
    end

    print(2, 12, "SISTEMA", WHITE, 1)
    print(2, 22, "free_heap():", GRAY, 1)
    print(2, 32, tostring(heap) .. " bytes", GREEN, 2)

    print(2, 52, "log() -> Serial", GRAY, 1)
    print(2, 62, "Abre Serial Mon", CYAN, 1)
    print(2, 72, "para ver output", CYAN, 1)

    -- Gráfico de heap en el tiempo
    print(2, 84, "Heap hist (50 frames):", WHITE, 1)
    if #heapHistory > 1 then
        local minH, maxH = math.huge, 0
        for _, v in ipairs(heapHistory) do
            if v < minH then minH = v end
            if v > maxH then maxH = v end
        end
        local rangeH = maxH - minH
        if rangeH == 0 then rangeH = 1 end
        local gx, gy, gw, gh = 2, 92, SW-4, 28
        frect(gx, gy, gw, gh, DKGRAY)
        for i, v in ipairs(heapHistory) do
            local px2 = gx + math.floor((i-1) / 49 * (gw-1))
            local py2 = gy + gh - 1 - math.floor((v - minH) / rangeH * (gh-1))
            pset(px2, py2, GREEN)
        end
        rect(gx, gy, gw, gh, GRAY)
    end

    -- millis formateado
    local ms = millis()
    local ss = math.floor(ms / 1000) % 60
    local mm = math.floor(ms / 60000) % 60
    print(2, 122, string.format("uptime %02d:%02d", mm, ss), GRAY, 1)

    draw_hud()
    flip()
end

-- =============================================================
--  ESCENA 9 — WIFI SCAN
--  Prueba: wifi_scan, wifi_status
--  Solo escanea redes, sin conectar.
--  Pulsá A para re-escanear.
-- =============================================================
local scanDone   = false
local scanList   = {}
local scanMsg    = "Pulsá A para escanear"
local scanning   = false
local scanFrames = 0   -- delay de un frame para mostrar "Escaneando..."

local function scene_wifi()
    -- A: lanzar escaneo
    if btnp(4) and not scanning then
        scanning   = true
        scanFrames = 0
        scanMsg    = "Escaneando..."
        scanList   = {}
        scanDone   = false
    end

    -- El escaneo ocupa un frame: mostramos primero el mensaje
    -- y ejecutamos wifi_scan() el frame siguiente
    if scanning then
        scanFrames = scanFrames + 1
        if scanFrames >= 2 then
            scanList  = wifi_scan()
            scanDone  = true
            scanning  = false
            scanMsg   = "Redes encontradas: " .. #scanList
        end
    end

    cls(BLACK)
    print(2, 12, "WIFI SCAN", WHITE, 1)

    -- Estado del chip WiFi
    local status = wifi_status()
    local scol   = (status == "connected") and GREEN or ORANGE
    print(2, 22, "Estado: " .. status, scol, 1)

    -- Mensaje de estado del scan
    print(2, 32, scanMsg, CYAN, 1)

    -- Lista de redes
    if scanDone and #scanList > 0 then
        -- Cabecera
        frect(0, 42, SW, 8, DKGRAY)
        print(1, 43, "SEC SSID            RSSI", GRAY, 1)

        local maxShow = 8   -- máximo de redes que caben en pantalla
        for i = 1, math.min(maxShow, #scanList) do
            local s    = scanList[i]
            local y    = 42 + i * 9

            -- Fondo alternado
            if i % 2 == 0 then frect(0, y, SW, 8, DKGRAY) end

            -- Icono de seguridad
            local sec = s.secure and "[*]" or "[ ]"
            local secCol = s.secure and RED or GREEN
            print(1, y+1, sec, secCol, 1)

            -- SSID (truncado a 13 chars para que quepa)
            local ssid = s.ssid
            if #ssid > 13 then ssid = ssid:sub(1,12) .. "~" end
            print(22, y+1, ssid, WHITE, 1)

            -- RSSI con color por señal
            local rssi   = s.rssi
            local rcol   = rssi > -60 and GREEN or
                           rssi > -75 and YELLOW or RED
            print(SW - 28, y+1, tostring(rssi), rcol, 1)
        end

        if #scanList > maxShow then
            print(2, 42 + (maxShow+1)*9, "+" .. (#scanList-maxShow) .. " mas...", GRAY, 1)
        end
    elseif scanDone and #scanList == 0 then
        print(2, 52, "Sin redes visibles", RED, 1)
    end

    -- Instrucción
    print(1, SH - 10, "A=escanear  HOME=salir", GRAY, 1)

    draw_hud()
    flip()
end

-- =============================================================
--  LOOP PRINCIPAL
-- =============================================================
local scenes = {
    scene_graphics,
    scene_text_color,
    scene_sprites,
    scene_input,
    scene_time,
    scene_math,
    scene_sd,
    scene_system,
    scene_wifi,
}

local prevScene = 0

while not tick() do
    frame = frame + 1

    -- Cambio de escena con LEFT / RIGHT
    if btnp(2) then   -- LEFT
        scene = scene - 1
        if scene < 1 then scene = TOTAL_SCENES end
    end
    if btnp(3) then   -- RIGHT
        scene = scene + 1
        if scene > TOTAL_SCENES then scene = 1 end
    end

    -- Reiniciar estado de escenas que lo necesitan
    if scene ~= prevScene then
        prevScene = scene
        if scene == 7 then
            sdDone  = false
            sdLog   = {}
            sdLines = {}
        end
        if scene == 9 then
            scanDone  = false
            scanList  = {}
            scanMsg   = "Pulsá A para escanear"
            scanning  = false
        end
        if scene == 6 then
            init_stars()
        end
    end

    -- Ejecutar escena activa
    scenes[scene]()
end