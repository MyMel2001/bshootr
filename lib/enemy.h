#pragma once
// square enemies: position, health, alive flag.
// AI: chase player, attack when adjacent.
#include "player.h"
#include <chrono>
#include <cmath>
#include <vector>

#define ENEMY_MOVE_EVERY_N_FRAMES 6
#define ENEMY_SPEED 0.08f
#define ENEMY_ATTACK_RANGE 1.2f
#define ENEMY_ATTACK_DAMAGE 8
#define ENEMY_ATTACK_SPEED 3

#include <chrono>

auto start = std::chrono::steady_clock::now();

struct Enemy {
  float x, y;
  int health;
  bool alive;

  Enemy(float x, float y) : x(x), y(y), health(50), alive(true) {}
};

void updateEnemies(std::vector<Enemy> &enemies, Player *player, Level *level,
                   int frame) {
  if (frame % ENEMY_MOVE_EVERY_N_FRAMES != 0)
    return;

  int mapH = (int)level->map.size();
  int mapW = mapH > 0 ? (int)level->map[0].size() : 0;

  for (auto &e : enemies) {
    if (!e.alive)
      continue;

    float dx = player->x - e.x;
    float dy = player->y - e.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.001f)
      continue;

    // attack instead of move when adjacent
    if (dist < ENEMY_ATTACK_RANGE) {

      bool waiting = true;
      auto elapsed = std::chrono::steady_clock::now() - start;
      if (waiting) {

        if (elapsed >= std::chrono::seconds(ENEMY_ATTACK_SPEED)) {
          waiting = false;
          player->health -= ENEMY_ATTACK_DAMAGE;
          start = std::chrono::steady_clock::now();
          waiting = true;
        }
      }

      continue;
    }

    // move toward player (split-axis wall collision so they slide around
    // corners)
    float nx = (dx / dist) * ENEMY_SPEED;
    float ny = (dy / dist) * ENEMY_SPEED;

    int tx = (int)(e.x + nx);
    int ty = (int)e.y;
    if (tx >= 0 && ty >= 0 && tx < mapW && ty < mapH &&
        level->get(tx, ty) == 0) {
      e.x += nx;
    }

    tx = (int)e.x;
    ty = (int)(e.y + ny);
    if (tx >= 0 && ty >= 0 && tx < mapW && ty < mapH &&
        level->get(tx, ty) == 0) {
      e.y += ny;
    }
  }
}
