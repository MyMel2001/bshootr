#pragma once
// simple player controller.
// position and orientation.
#include "getCurrentLevel.h"
#include <cmath>

struct Player {
  float x;
  float y;
  float heading;
  int health;
};