#pragma once
// Pickups: health and ammo items placed in the level.
// Player walks over them to collect.
#include "player.h"
#include "weapon.h"
#include <cmath>
#include <vector>

#define HEALTH_PICKUP_AMOUNT 25
#define AMMO_PICKUP_AMOUNT 12
#define PICKUP_COLLECT_DIST 0.8f

enum PickupType { PICKUP_HEALTH, PICKUP_AMMO };

struct Pickup {
  float x, y;
  PickupType type;
  bool active; // false after collected

  Pickup(float x, float y, PickupType type)
      : x(x), y(y), type(type), active(true) {}
};

// Check all active pickups and collect any the player is close enough to.
void updatePickups(std::vector<Pickup> &pickups, Player *player,
                   WeaponState &weapon) {
  for (auto &p : pickups) {
    if (!p.active)
      continue;

    float dx = player->x - p.x;
    float dy = player->y - p.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < PICKUP_COLLECT_DIST) {
      p.active = false;
      if (p.type == PICKUP_HEALTH) {
        player->health += HEALTH_PICKUP_AMOUNT;
        if (player->health > 100)
          player->health = 100;
      } else if (p.type == PICKUP_AMMO) {
        weapon.ammo += AMMO_PICKUP_AMOUNT;
      }
    }
  }
}
