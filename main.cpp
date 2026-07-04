#include "./lib/mincurses.h"
#include "./lib/renderer.h"
#include <cmath>
#include <unistd.h>
#include <vector>

// ── Helper: show a centred message for a given number of seconds ──────────
static void showSplash(const char *line1, const char *line2, int seconds) {
  int sw = getmaxx(stdscr), sh = getmaxy(stdscr);
  // block input during splash
  nodelay(stdscr, FALSE);

  for (int remaining = seconds; remaining > 0; remaining--) {
    clear();
    mvprintw(sh / 2 - 1, sw / 2 - (int)strlen(line1) / 2, "%s", line1);
    if (line2)
      mvprintw(sh / 2 + 1, sw / 2 - (int)strlen(line2) / 2, "%s", line2);
    mvprintw(sh / 2 + 3, sw / 2 - 6, "(%ds...)", remaining);
    refresh();
    sleep(1);
  }

  // restore non-blocking input
  nodelay(stdscr, TRUE);
}

// ── Helper: (re)initialise level-dependent state ─────────────────────────
static void loadLevel(Level *level, Player &player, std::vector<Enemy> &enemies,
                      std::vector<Boss> &bosses, std::vector<Pickup> &pickups,
                      int &mapW, int &mapH) {
  mapW = (int)level->map[0].size();
  mapH = (int)level->map.size();

  enemies.clear();
  for (auto &sp : level->getEnemySpawns())
    enemies.push_back(Enemy(sp.first, sp.second));

  bosses.clear();
  for (auto &sp : level->getBossSpawns())
    bosses.push_back(Boss(sp.first, sp.second));

  pickups.clear();
  for (auto &sp : level->getHealthPickupSpawns())
    pickups.push_back(Pickup(sp.first, sp.second, PICKUP_HEALTH));
  for (auto &sp : level->getAmmoPickupSpawns())
    pickups.push_back(Pickup(sp.first, sp.second, PICKUP_AMMO));

  // reset player to top-left open cell
  player.x = 1.5f;
  player.y = 1.5f;
  player.heading = 0.0f;
}

int main() {
  // ── ncurses init ──────────────────────────────────────────────────────────
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);

  // ── Player init ───────────────────────────────────────────────────────────
  Player player;
  player.health = 100;

  Level *level = getLevel();
  int mapW, mapH;
  std::vector<Enemy> enemies;
  std::vector<Boss> bosses;
  std::vector<Pickup> pickups;
  loadLevel(level, player, enemies, bosses, pickups, mapW, mapH);

  // ── Weapon init ───────────────────────────────────────────────────────────
  WeaponState weapon;

  int frame = 0;

  // initial render before first input
  clear();
  drawLevel(level, &player, enemies, bosses, pickups, weapon);
  refresh();

  // ── Game loop ─────────────────────────────────────────────────────────────
  while (1) {
    int ch = getch();

    if (ch == 'q' || ch == 'Q')
      break;

    const float MOVE_SPEED = 0.1f;
    const float ROT_SPEED = 0.08f;

    // Forward / backward — split-axis so player slides along walls
    if (ch == 'w' || ch == 'W') {
      float nx = player.x + cosf(player.heading) * MOVE_SPEED;
      float ny = player.y + sinf(player.heading) * MOVE_SPEED;
      if ((int)nx >= 0 && (int)nx < mapW && (int)player.y >= 0 &&
          (int)player.y < mapH && level->get((int)nx, (int)player.y) == 0)
        player.x = nx;
      if ((int)player.x >= 0 && (int)player.x < mapW && (int)ny >= 0 &&
          (int)ny < mapH && level->get((int)player.x, (int)ny) == 0)
        player.y = ny;
    }
    if (ch == 's' || ch == 'S') {
      float nx = player.x - cosf(player.heading) * MOVE_SPEED;
      float ny = player.y - sinf(player.heading) * MOVE_SPEED;
      if ((int)nx >= 0 && (int)nx < mapW && (int)player.y >= 0 &&
          (int)player.y < mapH && level->get((int)nx, (int)player.y) == 0)
        player.x = nx;
      if ((int)player.x >= 0 && (int)player.x < mapW && (int)ny >= 0 &&
          (int)ny < mapH && level->get((int)player.x, (int)ny) == 0)
        player.y = ny;
    }
    // Rotate
    if (ch == 'a' || ch == 'A')
      player.heading -= ROT_SPEED;
    if (ch == 'd' || ch == 'D')
      player.heading += ROT_SPEED;

    // Shoot pistol
    if (ch == 'f' || ch == 'F' || ch == ' ')
      shootPistol(&player, level, enemies, bosses, weapon);

    // Punch
    if (ch == 'e' || ch == 'E')
      punch(&player, enemies, bosses, weapon);

    // ── System updates ──────────────────────────────────────────────────────
    updateEnemies(enemies, &player, level, frame);
    updateBosses(bosses, &player, level, frame);
    updatePickups(pickups, &player, weapon);
    updateWeapon(weapon);

    // ── Win / lose checks ───────────────────────────────────────────────────
    bool allDead = true;
    for (int i = 0; i < (int)enemies.size(); i++)
      if (enemies[i].alive) {
        allDead = false;
        break;
      }
    if (allDead) {
      for (int i = 0; i < (int)bosses.size(); i++)
        if (bosses[i].alive) {
          allDead = false;
          break;
        }
    }

    clear();

    // ── Death ───────────────────────────────────────────────────────────────
    if (player.health <= 0) {
      int sw = getmaxx(stdscr), sh = getmaxy(stdscr);
      mvprintw(sh / 2 - 2, sw / 2 - 9, " +-----------+ ");
      mvprintw(sh / 2 - 1, sw / 2 - 9, " | GAME OVER | ");
      mvprintw(sh / 2, sw / 2 - 9, " | You died  | ");
      mvprintw(sh / 2 + 1, sw / 2 - 9, " +-----------+ ");
      mvprintw(sh / 2 + 3, sw / 2 - 8, "Press Q to quit");
      refresh();
      nodelay(stdscr, FALSE);
      while (getch() != 'q') {
      }
      break;
    }

    // ── All enemies + bosses dead ────────────────────────────────────────────
    if (allDead) {
      if (hasNextLevel()) {
        // ── Level transition ──────────────────────────────────────────────
        char msg[64];
        snprintf(msg, sizeof(msg), "Next level: LEVEL %d",
                 currentLevel + 2); // +2: 0-indexed + next
        showSplash(msg, "Get ready!", 3);

        level = nextLevel();
        loadLevel(level, player, enemies, bosses, pickups, mapW, mapH);
        weapon = WeaponState(); // fresh weapon state
        frame = 0;

        // re-render the new level before resuming the loop
        clear();
        drawLevel(level, &player, enemies, bosses, pickups, weapon);
        refresh();
        continue;
      } else {
        // ── Final level beaten — YOU'RE WINNER! ───────────────────────────
        int sw = getmaxx(stdscr), sh = getmaxy(stdscr);
        mvprintw(sh / 2 - 2, sw / 2 - 11, " +---------------------+ ");
        mvprintw(sh / 2 - 1, sw / 2 - 11, " | YOU'RE WINNER!      | ");
        mvprintw(sh / 2, sw / 2 - 11, " | All levels cleared  | ");
        mvprintw(sh / 2 + 1, sw / 2 - 11, " +---------------------+ ");
        mvprintw(sh / 2 + 3, sw / 2 - 8, "Quitting in 10s...");
        refresh();
        nodelay(stdscr, FALSE);
        sleep(10);
        break;
      }
    }

    drawLevel(level, &player, enemies, bosses, pickups, weapon);
    refresh();

    frame++;
    usleep(16667); // cap at ~60 fps
  }

  endwin();
  return 0;
}
