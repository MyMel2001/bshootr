#pragma once
// Level data for the raycasted engine.
// '#' = wall, ' ' = open, '@' = enemy spawn, '!' = boss spawn,
// 'H' = health pickup, 'A' = ammo pickup (all treated as open after extraction)
#include <string>
#include <vector>

class Level {
public:
  Level() {} // default ctor for vector storage

  Level(const std::vector<std::string> &m) : map(m) {}

  // Returns the raw character at (x, y).
  char getChar(int x, int y) {
    if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size())
      return '#';
    return map[y][x];
  }

  // Returns 1 if wall, 0 if walkable (open, enemy spawn, boss spawn, pickup).
  int get(int x, int y) {
    char c = getChar(x, y);
    return (c == '#') ? 1 : 0;
  }

  // Extract enemy spawn positions from '@' markers in the map.
  // Each '@' at column x, row y spawns an enemy at (x+0.5, y+0.5).
  std::vector<std::pair<float, float>> getEnemySpawns() {
    std::vector<std::pair<float, float>> spawns;
    for (int y = 0; y < (int)map.size(); y++) {
      for (int x = 0; x < (int)map[y].size(); x++) {
        if (map[y][x] == '@') {
          spawns.push_back({x + 0.5f, y + 0.5f});
        }
      }
    }
    return spawns;
  }

  // Extract boss spawn positions from '!' markers in the map.
  // Each '!' at column x, row y spawns exactly one boss at (x+0.5, y+0.5).
  std::vector<std::pair<float, float>> getBossSpawns() {
    std::vector<std::pair<float, float>> spawns;
    for (int y = 0; y < (int)map.size(); y++) {
      for (int x = 0; x < (int)map[y].size(); x++) {
        if (map[y][x] == '!') {
          spawns.push_back({x + 0.5f, y + 0.5f});
        }
      }
    }
    return spawns;
  }

  // Extract health pickup positions from 'H' markers in the map.
  std::vector<std::pair<float, float>> getHealthPickupSpawns() {
    std::vector<std::pair<float, float>> spawns;
    for (int y = 0; y < (int)map.size(); y++) {
      for (int x = 0; x < (int)map[y].size(); x++) {
        if (map[y][x] == 'H') {
          spawns.push_back({x + 0.5f, y + 0.5f});
        }
      }
    }
    return spawns;
  }

  // Extract ammo pickup positions from 'A' markers in the map.
  std::vector<std::pair<float, float>> getAmmoPickupSpawns() {
    std::vector<std::pair<float, float>> spawns;
    for (int y = 0; y < (int)map.size(); y++) {
      for (int x = 0; x < (int)map[y].size(); x++) {
        if (map[y][x] == 'A') {
          spawns.push_back({x + 0.5f, y + 0.5f});
        }
      }
    }
    return spawns;
  }

  std::vector<std::string> map;
};

// ── Level definitions ────────────────────────────────────────────────────────

inline Level makeLevel0() {
  return Level({
      "#############",
      "#          ##",
      "#  ####    # #",
      "# @#HH#   # #",
      "#      ######",
      "#  #     @  #",
      "#############",
  });
}

inline Level makeLevel1() {
  return Level({
      "########################",
      "#          #           #",
      "#H####     #    ####A   #",
      "#A #   #   #   #   #   #",
      "#  #   #   #   # @     #",
      "#  #  ##       ### ######",
      "#      #   #   #     @ ##",
      "# ### @#   #   #@ ##### #",
      "#  #####  A#   ####     #",
      "#         H#            #",
      "########################",
  });
}

inline Level makeLevel2() {
  return Level({
      "########################",
      "#         #   @        #",
      "#####  #######  ########",
      "######   ######@   #####",
      "####### #######    #######",
      "#####AH     #####    # @#######",
      "#  #####       ###           #",
      "#      @       # @ #   ###   #",
      "#  # ##            #    @  HA#",
      "#  # @ #    #####   ##### ###",
      "#      ##   @       ##########",
      "#################################",
  });
}

inline Level makeLevel3() {
  return Level({
      "#################",
      "#         #   @#",
      "# A   ###### ####",
      "# H#  #####  ##@  #############################",
      "#####   @   ##  #               @            #",
      "#   ##         ## ##### ##A###########HA#@#####",
      "#@##   ####@   #    @ #    ####@  ######      #",
      "####@   #@    H  ######       ###    @    H####",
      "#   ##  ####  #########   ################   @#",
      "################################################",
  });
}

inline Level makeLevel4() {
  return Level({
      "####################################################################",
      "#    H    #   @        ############################                #",
      "##### A#######  ###############################HAA####         #AH#",
      "###### H ######@   ##############################            ######",
      "#######A#######    ##############################            ######",
      "##### AH    #####    # @###############################  #        #",
      "#  #####       ###           ###############  #          #   A   H#",
      "#   @          #   #  ####                               #       H#",
      "#  # ##            ##   AAA    ###################  ###############",
      "#  #   #    #####   ##### ####  AH####AA    !        H####",
      "#      ##           ######################################",
      "##########################################################",
  });
}
