#pragma once
#include "./levels.h"
#include <vector>

int currentLevel = 0;
std::vector<Level> levels = {makeLevel0(), makeLevel1(), makeLevel2(),
                             makeLevel3()};

Level *getLevel() { return &levels[currentLevel]; }

// Returns true if there is a next level, false if this was the last one.
bool hasNextLevel() { return currentLevel + 1 < (int)levels.size(); }

// Advance to the next level. Only call after hasNextLevel() returns true.
Level *nextLevel() {
  currentLevel++;
  return &levels[currentLevel];
}
