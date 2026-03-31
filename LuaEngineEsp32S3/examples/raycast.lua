-- ============================================================
--  RAYCAST3D — raycaster Wolfenstein-style  (versión optimizada)
--  Pantalla 128×128 · Mapa 8×8
--  Controles: DPAD (arriba/abajo = mover, izq/der = rotar)
--  HOME → salir
--
--  CAMBIOS vs versión original:
--    · DDA por celdas (30-50x más rápido que paso fijo)
--    · Offsets de ángulo precalculados
--    · Cosenos de corrección precalculados
--    · Fondo dibujado con 2 frect en lugar de 128 líneas
-- ============================================================

-- ── Mapa (1=pared, 0=libre) ──────────────────────────────────
local MAP = {
  {1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,1},
  {1,0,1,0,0,1,0,1},
  {1,0,0,0,0,0,0,1},
  {1,0,1,0,0,1,0,1},
  {1,0,0,0,1,0,0,1},
  {1,0,0,0,0,0,0,1},
  {1,1,1,1,0,1,1,1},
  {1,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1},
}
local MCOLS = 8
local MROWS = 10

-- ── Constantes ───────────────────────────────────────────────
local W        = 128
local H        = 128
local FOV      = math.pi / 3        -- 60°
local HALF_FOV = FOV / 2
local MAX_DIST = 16
local MOVE_SPD = 0.20
local ROT_SPD  = 0.10

-- ── Estado jugador ───────────────────────────────────────────
local px = 1.5
local py = 3.5
local pa = 0.3

-- ── Precalcular offsets de ángulo por columna ────────────────
local RAY_OFFSET = {}
for col = 0, W - 1 do
  RAY_OFFSET[col] = -HALF_FOV + (col / W) * FOV
end

-- Corrección de ojo de pez por columna (cos del offset)
local FISH_COS = {}
for col = 0, W - 1 do
  FISH_COS[col] = math.cos(RAY_OFFSET[col])
end

-- ── Colores de pared (brillo/sombra, 16 niveles) ─────────────
local WALL_BRIGHT = {}
local WALL_DARK   = {}
for i = 0, 15 do
  local v = math.floor(255 * (1 - i / 15))
  WALL_BRIGHT[i] = rgb(v,             math.floor(v*0.8), math.floor(v*0.5))
  WALL_DARK[i]   = rgb(math.floor(v*0.5), math.floor(v*0.6), v)
end

-- Colores planos de techo y suelo
local C_CEIL  = rgb(10, 10, 30)
local C_FLOOR = rgb(80, 65, 55)

-- ── DDA por celdas ───────────────────────────────────────────
local function cast_ray(cosA, sinA)
  local mx = math.floor(px)
  local my = math.floor(py)

  local ddx = math.abs(cosA) < 1e-9 and 1e30 or math.abs(1 / cosA)
  local ddy = math.abs(sinA) < 1e-9 and 1e30 or math.abs(1 / sinA)

  local stepX, stepY, sideDistX, sideDistY

  if cosA < 0 then
    stepX     = -1
    sideDistX = (px - mx) * ddx
  else
    stepX     = 1
    sideDistX = (mx + 1 - px) * ddx
  end
  if sinA < 0 then
    stepY     = -1
    sideDistY = (py - my) * ddy
  else
    stepY     = 1
    sideDistY = (my + 1 - py) * ddy
  end

  local side = false

  for _ = 1, 32 do
    if sideDistX < sideDistY then
      sideDistX = sideDistX + ddx
      mx        = mx + stepX
      side      = false
    else
      sideDistY = sideDistY + ddy
      my        = my + stepY
      side      = true
    end

    if mx < 0 or mx >= MCOLS or my < 0 or my >= MROWS then
      return false, MAX_DIST, false
    end

    if MAP[my + 1][mx + 1] == 1 then
      local dist
      if not side then
        dist = sideDistX - ddx
      else
        dist = sideDistY - ddy
      end
      return true, dist, side
    end
  end

  return false, MAX_DIST, false
end

-- ── Dibujar fondo ────────────────────────────────────────────
local function draw_bg()
  frect(0,     0,   W, H/2, C_CEIL)
  frect(0, H/2,   W, H/2, C_FLOOR)
end

-- ── Dibujar paredes ──────────────────────────────────────────
local function draw_walls()
  for col = 0, W - 1 do
    local angle = pa + RAY_OFFSET[col]
    local cosA  = math.cos(angle)
    local sinA  = math.sin(angle)

    local hit, dist, side = cast_ray(cosA, sinA)

    if hit then
      local corr = dist * FISH_COS[col]
      if corr < 0.01 then corr = 0.01 end

      local h   = math.min(H, math.floor(H / corr))
      local top = math.floor((H - h) / 2)

      local bi = math.min(15, math.floor(dist * 15 / MAX_DIST))
      local c  = side and WALL_DARK[bi] or WALL_BRIGHT[bi]

      frect(col, top, 1, h, c)
    end
  end
end

-- ── INIT ─────────────────────────────────────────────────────
cls()
flip()

-- ── LOOP PRINCIPAL ───────────────────────────────────────────
while not tick() do   -- tick() lee botones Y devuelve shouldExit

  -- Entrada
  local cur_up    = btn(0)
  local cur_down  = btn(1)
  local cur_left  = btn(2)
  local cur_right = btn(3)

  -- Movimiento
  if cur_up then
    local nx = px + math.cos(pa) * MOVE_SPD
    local ny = py + math.sin(pa) * MOVE_SPD
    if MAP[math.floor(ny)+1][math.floor(nx)+1] == 0 then
      px = nx; py = ny
    end
  end
  if cur_down then
    local nx = px - math.cos(pa) * MOVE_SPD
    local ny = py - math.sin(pa) * MOVE_SPD
    if MAP[math.floor(ny)+1][math.floor(nx)+1] == 0 then
      px = nx; py = ny
    end
  end
  if cur_left  then pa = pa - ROT_SPD end
  if cur_right then pa = pa + ROT_SPD end

  -- Render
  cls()
  draw_bg()
  draw_walls()
  flip()

  delay(16)
end

cls()
flip()