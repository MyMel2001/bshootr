#pragma once
// weapon system: pistol (ranged) and fist (melee).
#include "boss.h"
#include "enemy.h"
#include <cmath>
#include <vector>

// ── Constants ────────────────────────────────────────────────────────────────
#define PISTOL_DAMAGE 34
#define PISTOL_COOLDOWN 24     // frames between shots
#define PISTOL_HIT_HALF_W 0.4f // half-width of the hit cylinder
#define PISTOL_MAX_RANGE 28.0f

#define PUNCH_DAMAGE 25
#define PUNCH_COOLDOWN 18 // frames between punches
#define PUNCH_RANGE 1.5f
#define PUNCH_HALF_ANGLE 0.52f // ~30 degrees in radians

#define ANIM_SHOOT 1
#define ANIM_PUNCH 2

// ── State ────────────────────────────────────────────────────────────────────
struct WeaponState {
  int ammo;
  int pistolCooldown;
  int punchCooldown;
  int animFrame; // counts down from N → 0 for animation
  int animType;  // ANIM_SHOOT or ANIM_PUNCH
  bool lastWasPistol;
  bool hitFlash; // true for one frame on a successful hit

  WeaponState()
      : ammo(32), pistolCooldown(0), punchCooldown(0), animFrame(0),
        animType(0), lastWasPistol(true), hitFlash(false) {}
};

// ── Internal helpers ─────────────────────────────────────────────────────────
static float _wallDist(Player *player, Level *level, float angle,
                       float maxDist) {
  int mapH = (int)level->map.size();
  int mapW = mapH > 0 ? (int)level->map[0].size() : 0;
  float dx = cosf(angle);
  float dy = sinf(angle);
  for (float d = 0.1f; d < maxDist; d += 0.05f) {
    int mx = (int)(player->x + dx * d);
    int my = (int)(player->y + dy * d);
    if (mx < 0 || my < 0 || mx >= mapW || my >= mapH)
      return d;
    if (level->get(mx, my) == 1)
      return d;
  }
  return maxDist;
}

static float _normalizeAngle(float a) {
  while (a > M_PI)
    a -= 2.0f * M_PI;
  while (a < -M_PI)
    a += 2.0f * M_PI;
  return a;
}

// ── Public API ───────────────────────────────────────────────────────────────

// Shoot the pistol toward player.heading. Returns true on hit.
bool shootPistol(Player *player, Level *level, std::vector<Enemy> &enemies,
                 std::vector<Boss> &bosses, WeaponState &weapon) {
  if (weapon.ammo <= 0 || weapon.pistolCooldown > 0)
    return false;

  weapon.ammo--;
  weapon.pistolCooldown = PISTOL_COOLDOWN;
  weapon.animFrame = 10;
  weapon.animType = ANIM_SHOOT;
  weapon.lastWasPistol = true;

  float wallD = _wallDist(player, level, player->heading, PISTOL_MAX_RANGE);
  float cdx = cosf(player->heading);
  float cdy = sinf(player->heading);

  // ── Check enemies ──────────────────────────────────────────────────────
  Enemy *targetE = nullptr;
  float targetDist = wallD; // only hit entities closer than the wall

  for (auto &e : enemies) {
    if (!e.alive)
      continue;
    float ex = e.x - player->x;
    float ey = e.y - player->y;
    float dot = ex * cdx + ey * cdy; // distance along ray
    if (dot <= 0.0f || dot >= wallD)
      continue;
    float perp = fabsf(ex * cdy - ey * cdx); // lateral distance from ray
    if (perp < PISTOL_HIT_HALF_W && dot < targetDist) {
      targetDist = dot;
      targetE = &e;
    }
  }

  // ── Check bosses ───────────────────────────────────────────────────────
  Boss *targetB = nullptr;
  for (auto &b : bosses) {
    if (!b.alive)
      continue;
    float bx = b.x - player->x;
    float by = b.y - player->y;
    float dot = bx * cdx + by * cdy;
    if (dot <= 0.0f || dot >= wallD)
      continue;
    float perp = fabsf(bx * cdy - by * cdx);
    if (perp < PISTOL_HIT_HALF_W && dot < targetDist) {
      targetDist = dot;
      targetE = nullptr; // boss is closer, override enemy
      targetB = &b;
    }
  }

  if (targetB) {
    targetB->health -= PISTOL_DAMAGE;
    if (targetB->health <= 0)
      targetB->alive = false;
    weapon.hitFlash = true;
    return true;
  }
  if (targetE) {
    targetE->health -= PISTOL_DAMAGE;
    if (targetE->health <= 0)
      targetE->alive = false;
    weapon.hitFlash = true;
    return true;
  }
  return false;
}

// Punch: short-range cone in front of player. Returns true on hit.
bool punch(Player *player, std::vector<Enemy> &enemies,
           std::vector<Boss> &bosses, WeaponState &weapon) {
  if (weapon.punchCooldown > 0)
    return false;

  weapon.punchCooldown = PUNCH_COOLDOWN;
  weapon.animFrame = 12;
  weapon.animType = ANIM_PUNCH;
  weapon.lastWasPistol = false;

  bool hit = false;

  // ── Check enemies ──────────────────────────────────────────────────────
  for (auto &e : enemies) {
    if (!e.alive)
      continue;
    float dx = e.x - player->x;
    float dy = e.y - player->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > PUNCH_RANGE)
      continue;
    float angle = _normalizeAngle(atan2f(dy, dx) - player->heading);
    if (fabsf(angle) < PUNCH_HALF_ANGLE) {
      e.health -= PUNCH_DAMAGE;
      if (e.health <= 0)
        e.alive = false;
      weapon.hitFlash = true;
      hit = true;
    }
  }

  // ── Check bosses ───────────────────────────────────────────────────────
  for (auto &b : bosses) {
    if (!b.alive)
      continue;
    float dx = b.x - player->x;
    float dy = b.y - player->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > PUNCH_RANGE)
      continue;
    float angle = _normalizeAngle(atan2f(dy, dx) - player->heading);
    if (fabsf(angle) < PUNCH_HALF_ANGLE) {
      b.health -= PUNCH_DAMAGE;
      if (b.health <= 0)
        b.alive = false;
      weapon.hitFlash = true;
      hit = true;
    }
  }

  return hit;
}

// Tick all cooldowns and the animation counter down one frame.
void updateWeapon(WeaponState &weapon) {
  if (weapon.pistolCooldown > 0)
    weapon.pistolCooldown--;
  if (weapon.punchCooldown > 0)
    weapon.punchCooldown--;
  if (weapon.animFrame > 0)
    weapon.animFrame--;
  else
    weapon.animType = 0;
  weapon.hitFlash = false; // flash lasts exactly one frame
}
