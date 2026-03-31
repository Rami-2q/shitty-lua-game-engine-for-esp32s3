-- ============================================================
--  SNAKE  — estilo Google Snake
--  Grilla 8×7 celdas de 16px · Pantalla 128×128
--  HUD 16px arriba · Tablero 128×112
-- ============================================================

-- ── Colores ─────────────────────────────────────────────────
local BG1      = rgb(76,  175, 80)
local BG2      = rgb(66,  160, 71)
local HUD_BG   = rgb(27,  94,  32)
local BORDER   = rgb(50,  120, 60)
local WHITE    = rgb(255, 255, 255)
local BLACK    = rgb(0,   0,   0)
local YELLOW   = rgb(255, 214, 0)
local APPLE    = rgb(229, 57,  53)
local APPLE_D  = rgb(183, 28,  28)
local APPLE_H  = rgb(255, 160, 130)
local LEAF     = rgb(30,  100, 20)
local BODY     = rgb(66,  150, 210)
local BODY_D   = rgb(30,  100, 160)
local RED_GO   = rgb(229, 130, 130)
local GOLD     = rgb(255, 200, 0)
local TRANSP   = rgb(0, 0, 0)


-- ── Constantes grilla ────────────────────────────────────────
local CELL     = 16
local COLS     = 8
local ROWS     = 7
local OX       = 0
local OY       = 16
local R        = 5
local SPD      = 2
local MAX_SCORE = COLS * ROWS  -- 56

local DIR_UP    = 0
local DIR_DOWN  = 1
local DIR_LEFT  = 2
local DIR_RIGHT = 3

-- ── IDs de sprites ───────────────────────────────────────────
local SPR_BODY = 0

-- ── Generar sprite del cuerpo ────────────────────────────────
local function make_body_sprite()

  local pixels = {}
  local cx = 8
  local cy = 8
  for i = 1, 256 do pixels[i] = TRANSP end
  for py = 0, 15 do
    for px2 = 0, 15 do
      local dx = px2 - cx + 0.5
      local dy = py  - cy + 0.5
      local d  = math.sqrt(dx*dx + dy*dy)
      local col
      if d <= R then
        col = BODY
      elseif d <= R + 1.5 then
        col = BODY_D
      else
        col = TRANSP
      end
      pixels[py * 16 + px2 + 1] = col
    end
  end
  spr_define(SPR_BODY, pixels)
-- test: dibuja el sprite en la esquina
cls()
spr(SPR_BODY, 0, 0, TRANSP)
flip()
delay(2000)
end

-- ── Estado del juego ─────────────────────────────────────────
local HIST = (COLS * ROWS + 4) * CELL

local hx = {}
local hy = {}
local hidx  = 0
local blen  = 0

local dir   = DIR_RIGHT
local grow  = 0

local q     = {}
local qlen  = 0

local food_col = 0
local food_row = 0

local score = 0
local best  = 0
local dead  = false
local won   = false

-- ── FPS ──────────────────────────────────────────────────────
local fps        = 0
local fps_frames = 0
local fps_timer  = 0

-- ── Helpers dirección ────────────────────────────────────────
local function ddx(d)
  if d == DIR_LEFT  then return -1 end
  if d == DIR_RIGHT then return  1 end
  return 0
end
local function ddy(d)
  if d == DIR_UP   then return -1 end
  if d == DIR_DOWN then return  1 end
  return 0
end

-- ── Acceso circular al historial ─────────────────────────────
local function buf(offset)
  local i = hidx - offset
  while i < 0 do i = i + HIST end
  return i
end
local function px(offset) return hx[buf(offset)] end
local function py(offset) return hy[buf(offset)] end

local function ax(bx) return OX + bx + CELL/2 end
local function ay(by) return OY + by + CELL/2 end

-- ── Cola de inputs ───────────────────────────────────────────
local function q_last()
  if qlen == 0 then return dir end
  return q[qlen]
end

local function q_push(d)
  if qlen >= 3 then return end
  local last = q_last()
  if d == DIR_UP    and last == DIR_DOWN  then return end
  if d == DIR_DOWN  and last == DIR_UP    then return end
  if d == DIR_LEFT  and last == DIR_RIGHT then return end
  if d == DIR_RIGHT and last == DIR_LEFT  then return end
  if d == last then return end
  qlen = qlen + 1
  q[qlen] = d
end

local function q_apply()
  if qlen == 0 then return end
  local d   = q[1]
  local toH = (d == DIR_LEFT or d == DIR_RIGHT)
  local toV = (d == DIR_UP   or d == DIR_DOWN)
  local cx  = px(0)
  local cy  = py(0)
  local hAlign = (cx % CELL) == 0
  local vAlign = (cy % CELL) == 0
  if (toH and vAlign) or (toV and hAlign) then
    dir = d
    for i = 1, qlen-1 do q[i] = q[i+1] end
    qlen = qlen - 1
  end
end

-- ── Colocar comida ───────────────────────────────────────────
local function place_food()
  local occ = {}
  for c = 0, COLS-1 do
    occ[c] = {}
    for r = 0, ROWS-1 do occ[c][r] = false end
  end
  local i = 0
  while i < blen do
    local c = math.floor(px(i) / CELL)
    local r = math.floor(py(i) / CELL)
    if c >= 0 and c < COLS and r >= 0 and r < ROWS then
      occ[c][r] = true
    end
    i = i + CELL
  end
  local tries = 0
  repeat
    food_col = math.random(0, COLS-1)
    food_row = math.random(0, ROWS-1)
    tries = tries + 1
  until (not occ[food_col][food_row]) or tries > 200
end

-- ── Reset ────────────────────────────────────────────────────
local function reset()
  score = 0
  dead  = false
  won   = false
  grow  = 0
  dir   = DIR_RIGHT
  q     = {}
  qlen  = 0

  local startX = 3 * CELL
  local startY = math.floor(ROWS / 2) * CELL
  blen  = 3 * CELL
  hidx  = HIST - 1

  for offset = 0, HIST-1 do
    local bx = startX - offset
    if bx < 0 then bx = 0 end
    local idx2 = hidx - offset
    while idx2 < 0 do idx2 = idx2 + HIST end
    hx[idx2] = bx
    hy[idx2] = startY
  end

  place_food()
end

-- ── Dibujo fondo tablero ─────────────────────────────────────
local function draw_bg()
  for row = 0, ROWS-1 do
    for col = 0, COLS-1 do
      local c = ((col + row) % 2 == 1) and BG2 or BG1
      frect(OX + col*CELL, OY + row*CELL, CELL, CELL, c)
    end
  end
end

-- ── Dibujo manzana ───────────────────────────────────────────
local function draw_apple()
  local apx = OX + food_col * CELL
  local apy = OY + food_row * CELL
  frect(apx+3, apy+4, 10, 10, APPLE)
  frect(apx+2, apy+5,  1,  8, APPLE)
  frect(apx+13,apy+5,  1,  8, APPLE)
  frect(apx+4, apy+3,  8,  1, APPLE)
  frect(apx+4, apy+14, 8,  1, APPLE)
  frect(apx+8, apy+11, 4,  3, APPLE_D)
  frect(apx+4, apy+5,  2,  2, APPLE_H)
  pset(apx+5,  apy+4,      APPLE_H)
  pset(apx+8,  apy+3,      LEAF)
  pset(apx+8,  apy+2,      LEAF)
  frect(apx+9, apy+2,  3,  2, LEAF)
  pset(apx+11, apy+1,      LEAF)
end

-- ── Dibujo serpiente ─────────────────────────────────────────
local function draw_snake()
  if blen <= 0 then return end
  for i = 0, blen - 1, CELL do
    local sx = ax(px(i)) - 8
    local sy = ay(py(i)) - 8
    spr(SPR_BODY, sx, sy, TRANSP)
  end
  -- Ojos
  local hcx = ax(px(0))
  local hcy = ay(py(0))
  local ex  = ddx(dir)
  local ey  = ddy(dir)
  local px2 =  ey
  local py2 = -ex
  local rr  = R - 2
  local ox1 = hcx + ex*rr + px2*rr
  local oy1 = hcy + ey*rr + py2*rr
  local ox2 = hcx + ex*rr - px2*rr
  local oy2 = hcy + ey*rr - py2*rr
  frect(ox1-1, oy1-1, 2, 2, WHITE)
  frect(ox2-1, oy2-1, 2, 2, WHITE)
  pset(ox1, oy1, BLACK)
  pset(ox2, oy2, BLACK)
end

-- ── HUD ──────────────────────────────────────────────────────
local function draw_hud()
  frect(0, 0, 128, OY, HUD_BG)
  -- ícono manzana
  frect(2,3, 8,8, APPLE)
  frect(5,2, 2,2, LEAF)
  frect(5,9, 4,2, APPLE_D)
  -- score
  print(13, 5, tostring(score), WHITE)
  -- separador
  line(63, 2, 63, OY-2, BORDER)
  -- trofeo
  frect(68, 3, 7, 6, YELLOW)
  frect(70, 9, 3, 2, YELLOW)
  frect(69,11, 5, 1, YELLOW)
  -- best
  print(78, 5, tostring(best), WHITE)
  -- FPS (derecha del todo)
  print(100, 5, tostring(fps), GOLD)
  -- línea inferior HUD
  line(0, OY-1, 127, OY-1, BORDER)
end

-- ── Pantalla Game Over ───────────────────────────────────────
local function draw_game_over()
  frect(18, 38, 94, 56, rgb(20,20,20))
  frect(15, 35, 94, 56, rgb(48,48,72))
  rect(15, 35, 94, 56, RED_GO)
  rect(16, 36, 92, 54, rgb(250,200,200))
  print(28, 44, "GAME OVER", RED_GO)
  line(21, 55, 102, 55, rgb(130,130,130))
  print(20, 59, "SCORE: "..tostring(score), WHITE)
  print(20, 69, "BEST:  "..tostring(best),  YELLOW)
  line(21, 80, 102, 80, rgb(130,130,130))
  print(24, 84, "A -> RETRY", LEAF)
end

-- ── Pantalla You Win ─────────────────────────────────────────
local function draw_you_win()
  frect(18, 38, 94, 56, rgb(20,20,20))
  frect(15, 35, 94, 56, rgb(30,60,30))
  rect(15, 35, 94, 56, GOLD)
  rect(16, 36, 92, 54, rgb(255,240,150))
  print(34, 44, "YOU WIN!", GOLD)
  line(21, 55, 102, 55, rgb(130,130,130))
  print(20, 59, "SCORE: "..tostring(score), WHITE)
  print(20, 69, "PERFECT!", YELLOW)
  line(21, 80, 102, 80, rgb(130,130,130))
  print(24, 84, "A -> RETRY", LEAF)
end

-- ── Lógica un paso ───────────────────────────────────────────
local function step()
  for _ = 1, SPD do
    q_apply()

    local nx = px(0) + ddx(dir)
    local ny = py(0) + ddy(dir)

    if nx < 0 or nx >= COLS*CELL or ny < 0 or ny >= ROWS*CELL then
      dead = true
      if score > best then best = score end
      return
    end

    hidx = hidx + 1
    if hidx >= HIST then hidx = 0 end
    hx[hidx] = nx
    hy[hidx] = ny

    if grow > 0 then
      blen = blen + 1
      if blen > HIST - 1 then blen = HIST - 1 end
      grow = grow - 1
    end

    if (nx % CELL) == 0 and (ny % CELL) == 0 then
      local fc = math.floor(nx / CELL)
      local fr = math.floor(ny / CELL)

      if fc == food_col and fr == food_row then
        score = score + 1
        if score >= MAX_SCORE then
          won = true
          if score > best then best = score end
          return
        end
        grow = grow + CELL
        place_food()
      end

      local skip = CELL * 3
      local i = skip
      while i < blen do
        if px(i) == nx and py(i) == ny then
          dead = true
          if score > best then best = score end
          return
        end
        i = i + CELL
      end
    end
  end
end

-- ── Variables para detección de flancos ──────────────────────
local prev_up    = false
local prev_down  = false
local prev_left  = false
local prev_right = false
local prev_a     = false

-- ── INIT ─────────────────────────────────────────────────────
math.randomseed(millis())
make_body_sprite()
reset()

-- ── LOOP PRINCIPAL ───────────────────────────────────────────
while not tick() do

  -- FPS
  fps_frames = fps_frames + 1
  local now = millis()
  if now - fps_timer >= 1000 then
    fps       = fps_frames
    fps_frames = 0
    fps_timer  = now
  end

  local cur_up    = btn(0)
  local cur_down  = btn(1)
  local cur_left  = btn(2)
  local cur_right = btn(3)
  local cur_a     = btn(4)

  local press_up    = cur_up    and not prev_up
  local press_down  = cur_down  and not prev_down
  local press_left  = cur_left  and not prev_left
  local press_right = cur_right and not prev_right
  local press_a     = cur_a     and not prev_a

  if not dead and not won then
    if press_up    then q_push(DIR_UP)    end
    if press_down  then q_push(DIR_DOWN)  end
    if press_left  then q_push(DIR_LEFT)  end
    if press_right then q_push(DIR_RIGHT) end
    step()
  else
    if press_a then reset() end
  end

  cls()
  draw_bg()
  if not won then draw_apple() end
  draw_snake()
  draw_hud()
  if dead then draw_game_over() end
  if won  then draw_you_win()  end
  flip()

  prev_up    = cur_up
  prev_down  = cur_down
  prev_left  = cur_left
  prev_right = cur_right
  prev_a     = cur_a

  delay(16)
end

cls()
flip()