#pragma once
// basic renderer.
// uses raycasting for walls + sprite rendering for enemies/bosses/pickups.
// uses ncurses for output.
#include "boss.h"
#include "mincurses.h"
#include "pickup.h"
#include "weapon.h"
#include <algorithm>
#include <cmath>
#include <vector>

#define FOV (M_PI / 3.0f) // 60-degree field of view
#define MAX_DEPTH 32.0f
#define STEP_SIZE 0.05f
#define HUD_ROWS 5 // rows reserved at bottom for HUD

// ── HUD helpers
// ───────────────────────────────────────────────────────────────

static void drawHPBar(int row, int col, int hp, int maxHp, int barW) {
  int filled = (int)((float)hp / maxHp * barW);
  filled = filled < 0 ? 0 : (filled > barW ? barW : filled);
  mvprintw(row, col, "HP [");
  for (int i = 0; i < barW; i++)
    mvaddch(row, col + 4 + i, i < filled ? '#' : '.');
  mvprintw(row, col + 4 + barW, "] %3d", hp);
}

static void drawWeaponArt(int screenH, int screenW, const WeaponState &weapon) {
  int cx = screenW / 2;
  bool anim = weapon.animFrame > 0;

  if (weapon.lastWasPistol) {
    if (anim && weapon.animType == ANIM_SHOOT) {
      mvprintw(screenH - 4, cx - 3, " * | ");
      mvprintw(screenH - 3, cx - 3, " [=] ");
      mvprintw(screenH - 2, cx - 3, " /___\\");
    } else {
      mvprintw(screenH - 4, cx - 3, " | ");
      mvprintw(screenH - 3, cx - 3, " [=] ");
      mvprintw(screenH - 2, cx - 3, " /___\\");
    }
  } else {
    if (anim && weapon.animType == ANIM_PUNCH) {
      mvprintw(screenH - 4, cx - 4, " >> ___ ");
      mvprintw(screenH - 3, cx - 4, " >>ooo|");
      mvprintw(screenH - 2, cx - 4, " |___|");
    } else {
      mvprintw(screenH - 4, cx - 4, " ___ ");
      mvprintw(screenH - 3, cx - 4, " |ooo|");
      mvprintw(screenH - 2, cx - 4, " |___|");
    }
  }
}

// ── Sprite rendering
// ──────────────────────────────────────────────────────────

// Draw a single enemy sprite, respecting the z-buffer for occlusion.
static void drawSprite(int screenW, int renderH, float *zBuf, float ex,
                       float ey, Player *player) {
  float dx = ex - player->x;
  float dy = ey - player->y;
  float spriteDist = sqrtf(dx * dx + dy * dy);
  if (spriteDist < 0.2f)
    return;

  float spriteAngle = atan2f(dy, dx) - player->heading;
  // normalize to [-PI, PI]
  while (spriteAngle > M_PI)
    spriteAngle -= 2.0f * M_PI;
  while (spriteAngle < -M_PI)
    spriteAngle += 2.0f * M_PI;

  // cull sprites outside ~FOV
  if (fabsf(spriteAngle) > FOV * 0.65f)
    return;

  // project onto screen
  float proj = tanf(FOV / 2.0f);
  int screenX = (int)(screenW / 2.0f * (1.0f + tanf(spriteAngle) / proj));

  int spriteH = (int)(renderH / spriteDist);
  if (spriteH <= 0)
    return;
  int spriteW = spriteH / 2;
  if (spriteW < 1)
    spriteW = 1;

  int x0 = screenX - spriteW / 2;
  int x1 = screenX + spriteW / 2;
  int y0 = renderH / 2 - spriteH / 2;
  int y1 = renderH / 2 + spriteH / 2;

  for (int sx = x0; sx <= x1; sx++) {
    if (sx < 0 || sx >= screenW)
      continue;
    if (spriteDist >= zBuf[sx])
      continue; // wall is closer — occlude the sprite

    for (int sy = y0; sy <= y1; sy++) {
      if (sy < 0 || sy >= renderH)
        continue;

      // pick sprite character by vertical position — gives a blocky "face"
      float rel = (float)(sy - y0) / (float)(y1 - y0 + 1);
      char c;
      if (rel < 0.12f)
        c = '-'; // top of head
      else if (rel < 0.38f)
        c = 'O'; // eyes/face
      else if (rel < 0.82f)
        c = '|'; // body
      else
        c = '_'; // feet
      mvaddch(sy, sx, c);
    }
  }
}

// Draw a boss sprite — larger and with distinct appearance.
static void drawBossSprite(int screenW, int renderH, float *zBuf, float bx,
                           float by, Player *player) {
  float dx = bx - player->x;
  float dy = by - player->y;
  float spriteDist = sqrtf(dx * dx + dy * dy);
  if (spriteDist < 0.2f)
    return;

  float spriteAngle = atan2f(dy, dx) - player->heading;
  while (spriteAngle > M_PI)
    spriteAngle -= 2.0f * M_PI;
  while (spriteAngle < -M_PI)
    spriteAngle += 2.0f * M_PI;

  if (fabsf(spriteAngle) > FOV * 0.65f)
    return;

  float proj = tanf(FOV / 2.0f);
  int screenX = (int)(screenW / 2.0f * (1.0f + tanf(spriteAngle) / proj));

  // bosses appear 1.5x taller than regular enemies
  int spriteH = (int)(renderH * 1.5f / spriteDist);
  if (spriteH <= 0)
    return;
  int spriteW = spriteH / 2;
  if (spriteW < 1)
    spriteW = 1;

  int x0 = screenX - spriteW / 2;
  int x1 = screenX + spriteW / 2;
  int y0 = renderH / 2 - spriteH / 2;
  int y1 = renderH / 2 + spriteH / 2;

  for (int sx = x0; sx <= x1; sx++) {
    if (sx < 0 || sx >= screenW)
      continue;
    if (spriteDist >= zBuf[sx])
      continue;

    for (int sy = y0; sy <= y1; sy++) {
      if (sy < 0 || sy >= renderH)
        continue;

      float rel = (float)(sy - y0) / (float)(y1 - y0 + 1);
      char c;
      if (rel < 0.08f)
        c = '='; // horns/crown
      else if (rel < 0.15f)
        c = 'W'; // wide head
      else if (rel < 0.40f)
        c = 'X'; // menacing eyes
      else if (rel < 0.85f)
        c = 'H'; // heavy body
      else
        c = '='; // feet
      mvaddch(sy, sx, c);
    }
  }
}

// Draw a pickup sprite — small floating item.
static void drawPickupSprite(int screenW, int renderH, float *zBuf, float px,
                             float py, Player *player, char label) {
  float dx = px - player->x;
  float dy = py - player->y;
  float spriteDist = sqrtf(dx * dx + dy * dy);
  if (spriteDist < 0.2f)
    return;

  float spriteAngle = atan2f(dy, dx) - player->heading;
  while (spriteAngle > M_PI)
    spriteAngle -= 2.0f * M_PI;
  while (spriteAngle < -M_PI)
    spriteAngle += 2.0f * M_PI;

  if (fabsf(spriteAngle) > FOV * 0.65f)
    return;

  float proj = tanf(FOV / 2.0f);
  int screenX = (int)(screenW / 2.0f * (1.0f + tanf(spriteAngle) / proj));

  // pickups are small — half the height of a regular enemy
  int spriteH = (int)(renderH * 0.5f / spriteDist);
  if (spriteH <= 0)
    return;
  int spriteW = spriteH / 2;
  if (spriteW < 1)
    spriteW = 1;

  int x0 = screenX - spriteW / 2;
  int x1 = screenX + spriteW / 2;
  // pickups float at mid-height (shifted down slightly from center)
  int y0 = renderH / 2 - spriteH / 2 + spriteH / 4;
  int y1 = y0 + spriteH;

  for (int sx = x0; sx <= x1; sx++) {
    if (sx < 0 || sx >= screenW)
      continue;
    if (spriteDist >= zBuf[sx])
      continue;

    for (int sy = y0; sy <= y1; sy++) {
      if (sy < 0 || sy >= renderH)
        continue;

      float rel = (float)(sy - y0) / (float)(y1 - y0 + 1);
      char c;
      if (rel < 0.25f)
        c = '-';
      else if (rel < 0.50f)
        c = label; // 'H' or 'A'
      else if (rel < 0.75f)
        c = label;
      else
        c = '_';
      mvaddch(sy, sx, c);
    }
  }
}

// ── Main draw call
// ────────────────────────────────────────────────────────────

void drawLevel(Level *level, Player *player, std::vector<Enemy> &enemies,
               std::vector<Boss> &bosses, std::vector<Pickup> &pickups,
               WeaponState &weapon) {
  int screenW = getmaxx(stdscr);
  int screenH = getmaxy(stdscr);
  int renderH = screenH - HUD_ROWS; // 3D view height (leave bottom for HUD)

  int mapH = (int)level->map.size();
  int mapW = mapH > 0 ? (int)level->map[0].size() : 0;

  // per-column wall distance (z-buffer for sprite occlusion)
  std::vector<float> zBuf(screenW, MAX_DEPTH);

  // ── Wall pass ─────────────────────────────────────────────────────────────
  for (int x = 0; x < screenW; x++) {
    float rayAngle =
        (player->heading - FOV / 2.0f) + ((float)x / screenW) * FOV;
    float rdx = cosf(rayAngle);
    float rdy = sinf(rayAngle);

    float dist = MAX_DEPTH;
    bool hit = false;

    for (float d = STEP_SIZE; d < MAX_DEPTH && !hit; d += STEP_SIZE) {
      int mx = (int)(player->x + rdx * d);
      int my = (int)(player->y + rdy * d);
      if (mx < 0 || my < 0 || mx >= mapW || my >= mapH) {
        dist = d;
        hit = true;
      } else if (level->get(mx, my) == 1) {
        dist = d;
        hit = true;
      }
    }

    // fisheye correction
    dist *= cosf(rayAngle - player->heading);
    if (dist < 0.1f)
      dist = 0.1f;
    zBuf[x] = dist;

    int lineH = (int)(renderH / dist);
    int startY = renderH / 2 - lineH / 2;
    int endY = renderH / 2 + lineH / 2;

    for (int y = 0; y < renderH; y++) {
      if (y < startY) {
        mvaddch(y, x, ' '); // ceiling
      } else if (y > endY) {
        mvaddch(y, x, '.'); // floor
      } else {
        // wall — shade darker with distance
        char wc = '#';
        if (dist > 5.0f)
          wc = '+';
        if (dist > 12.0f)
          wc = '-';
        mvaddch(y, x, wc);
      }
    }
  }

  // ── Sprite pass (enemies + bosses + pickups, farthest first) ──────────────
  // Collect all sprites with their distance for sorting.
  // We use an enum to tag the type and an index.
  enum SpriteKind { SK_ENEMY, SK_BOSS, SK_PICKUP };
  struct SpriteRef {
    float distSq;
    SpriteKind kind;
    int idx;
  };
  std::vector<SpriteRef> sprites;

  for (int i = 0; i < (int)enemies.size(); i++) {
    if (!enemies[i].alive)
      continue;
    float dx = enemies[i].x - player->x;
    float dy = enemies[i].y - player->y;
    sprites.push_back({dx * dx + dy * dy, SK_ENEMY, i});
  }
  for (int i = 0; i < (int)bosses.size(); i++) {
    if (!bosses[i].alive)
      continue;
    float dx = bosses[i].x - player->x;
    float dy = bosses[i].y - player->y;
    sprites.push_back({dx * dx + dy * dy, SK_BOSS, i});
  }
  for (int i = 0; i < (int)pickups.size(); i++) {
    if (!pickups[i].active)
      continue;
    float dx = pickups[i].x - player->x;
    float dy = pickups[i].y - player->y;
    sprites.push_back({dx * dx + dy * dy, SK_PICKUP, i});
  }

  // sort farthest first so closer sprites overwrite farther ones
  std::sort(sprites.begin(), sprites.end(),
            [](const SpriteRef &a, const SpriteRef &b) {
              return a.distSq > b.distSq;
            });

  for (const auto &sr : sprites) {
    if (sr.kind == SK_ENEMY) {
      drawSprite(screenW, renderH, zBuf.data(), enemies[sr.idx].x,
                 enemies[sr.idx].y, player);
    } else if (sr.kind == SK_BOSS) {
      drawBossSprite(screenW, renderH, zBuf.data(), bosses[sr.idx].x,
                     bosses[sr.idx].y, player);
    } else if (sr.kind == SK_PICKUP) {
      char label = (pickups[sr.idx].type == PICKUP_HEALTH) ? 'H' : 'A';
      drawPickupSprite(screenW, renderH, zBuf.data(), pickups[sr.idx].x,
                       pickups[sr.idx].y, player, label);
    }
  }

  // ── HUD ───────────────────────────────────────────────────────────────────
  // separator
  for (int x = 0; x < screenW; x++)
    mvaddch(renderH, x, '-');

  // HP bar (left)
  drawHPBar(renderH + 1, 1, player->health, 100, 20);

  // weapon / ammo (right)
  if (weapon.lastWasPistol)
    mvprintw(renderH + 1, screenW - 18, "PISTOL AMMO: %2d", weapon.ammo);
  else
    mvprintw(renderH + 1, screenW - 18, "FIST ");

  // hit flash
  if (weapon.hitFlash)
    mvprintw(renderH + 1, screenW / 2 - 3, " *HIT* ");

  // weapon art (bottom center)
  drawWeaponArt(screenH, screenW, weapon);

  // key hints
  mvprintw(screenH - 1, 1, "[WASD] Move [F/Space] Shoot [E] Punch [Q] Quit");

  // mini position readout
  mvprintw(0, 0, "(%.1f,%.1f)", player->x, player->y);
}
