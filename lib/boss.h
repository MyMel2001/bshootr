#pragma once
// Boss enemies: position, health, alive flag.
// AI: chase player, attack when adjacent. Slower but tougher than regular
// enemies.
#include "player.h"
#include <chrono>
#include <cmath>
#include <vector>

#define BOSS_MOVE_EVERY_N_FRAMES 8
#define BOSS_SPEED 0.06f
#define BOSS_ATTACK_RANGE 1.7f
#define BOSS_ATTACK_DAMAGE 13
#define BOSS_ATTACK_COOLDOWN_MS 4000

struct Boss {
  float x, y;
  int health;
  bool alive;
  std::chrono::steady_clock::time_point lastAttack;

  Boss(float x, float y)
      : x(x), y(y), health(150), alive(true),
        lastAttack(std::chrono::steady_clock::now()) {}
};

void updateBosses(std::vector<Boss> &bosses, Player *player, Level *level,
                  int frame) {
  if (frame % BOSS_MOVE_EVERY_N_FRAMES != 0)
    return;

  int mapH = (int)level->map.size();
  int mapW = mapH > 0 ? (int)level->map[0].size() : 0;

  for (auto &b : bosses) {
    if (!b.alive)
      continue;

    float dx = player->x - b.x;
    float dy = player->y - b.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.001f)
      continue;

    // attack instead of move when adjacent
    if (dist < BOSS_ATTACK_RANGE) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - b.lastAttack)
                         .count();
      if (elapsed >= BOSS_ATTACK_COOLDOWN_MS) {
        player->health -= BOSS_ATTACK_DAMAGE;
        b.lastAttack = now;
      }
      continue;
    }

    // move toward player (split-axis wall collision so they slide around
    // corners)
    float nx = (dx / dist) * BOSS_SPEED;
    float ny = (dy / dist) * BOSS_SPEED;

    int tx = (int)(b.x + nx);
    int ty = (int)b.y;
    if (tx >= 0 && ty >= 0 && tx < mapW && ty < mapH &&
        level->get(tx, ty) == 0) {
      b.x += nx;
    }

    tx = (int)b.x;
    ty = (int)(b.y + ny);
    if (tx >= 0 && ty >= 0 && tx < mapW && ty < mapH &&
        level->get(tx, ty) == 0) {
      b.y += ny;
    }
  }
}
