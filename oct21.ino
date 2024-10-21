/*// TODO: 
- Invert explosion sprites color (so they look better)
- Mask enemy sprites
- Implement player ammo (and reload fee in repair menu)
- Add multiple mech slots
- Finish customization menu implementation (so player can buy/sell mechs)
- Maybe: Add mech engine(s) and adjust other stats (moveSpeed, rotationSpeed, heatSink, weight, etc.) dependent on it?
- Add sounds (if I have enough RAM after everything else)
- Add jump jets for player dodging mechanic (if enough RAM is left over)*/

#include <Arduboy2.h>
#include <ArduboyFX.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "Trig.h"   
#include "fxdata.h"  
#include "src/fonts/Font4x6.h"

Arduboy2 arduboy;

Font4x6 font4x6 = Font4x6(7);

bool minimapEnabled = true;  

const uint8_t worldWidth = 32;
const uint8_t worldHeight = 32;

const uint8_t cellSize = 4;

const uint8_t gridX = worldWidth / cellSize;   // Number of cells along the X-axis
const uint8_t gridY = worldHeight / cellSize;  // Number of cells along the Y-axis

// Define the Object struct
struct Object {
  SQ7x8 x;
  SQ7x8 y;
};

Object gridCells[gridX * gridY];

enum class GameState : uint8_t {
  Title_Menu,
  Init_New_Game,
  Save_Game,
  Init_Load_Game,
  No_Save_Screen,
  Main_Menu,
  Save_Load_Submenu,
  Mission_Submenu,
  Hanger_Submenu,
  Customization_Submenu,
  Game_Play,
  Game_Over
};
GameState currentState = GameState::Title_Menu;
GameState previousState;

// TODO: Change to actual mech names, add maxStats to MechStats struct further down below
enum class MechType : uint8_t {
  Mothra,  // Light
  //Akira (Ryuken) // Light
  Battle_Cat,  // Medium
  //Doberman // Medium
  Thor_Hammer  // Heavy
};

enum class MechStatus : uint8_t {
  Normal,
  Overheat,
  Leg_Damage,
  Arm_Damage
};

struct PlayerMechStats /*: MechStats*/ {
  uint8_t weight;
  uint8_t heatSink;
  uint8_t maxHeatSink;
  uint8_t armor;  
  uint8_t maxArmor;
  SQ7x8 moveSpeed;        
  uint8_t rotationSpeed;  
};

// Definitions of MechStats for different mech types
constexpr PlayerMechStats playerMothraStats{
  100,  // weight
  100,  // heatSink (maximum heat capacity)
  115,  // maxHeatSink capacity (for customization)
  100,  // armor
  150,  // maxArmor
  0.3,  // moveSpeed
  3     // rotationSpeed
};

constexpr PlayerMechStats playerBattleCatStats{
  115,  // weight
  115,  // heatSink (maximum heat capacity)
  125,  // maxHeatSink capacity (for customization)
  115,  // armor
  175,  // maxArmor
  0.2,  // moveSpeed
  2     // rotationSpeed
};

constexpr PlayerMechStats playerThorHammerStats{
  127,  // weight
  120,  // heatsink (max heat capacity)
  135,  // maxHeatSink capacity (for customization)
  127,  // armor
  200,  // maxArmor
  0.1,  // moveSpeed
  1     // rotationSpeed
};

enum class WeaponType : uint8_t {
  None,
  Bullets,
  HeavyBullets,
  Rockets,
  MediumRockets,
  Laser
};

struct Weapon {
  WeaponType type;
  uint8_t damage;
  uint16_t cost;
  uint8_t weight;
  uint8_t heat;
  uint8_t ammo;
  // Add more attributes as needed
};

// Define available weapons for purchase
constexpr Weapon availableWeapons[] PROGMEM = {
  //            Type, damage, cost, weight, heat usage, ammo
  { WeaponType::Bullets, 1, 100, 80, 30, 0 },        // Light MG
  { WeaponType::HeavyBullets, 2, 200, 150, 40, 0 },  // Heavy MG
  { WeaponType::Rockets, 4, 300, 120, 50, 0 },       // Light Rockets
  { WeaponType::MediumRockets, 6, 500, 130, 60, 0 }, // Medium Rockets
  { WeaponType::Laser, 5, 250, 100, 90, 0 }          // Laser
  // Add more weapons as needed
};

constexpr uint8_t NUM_AVAILABLE_WEAPONS = sizeof(availableWeapons) / sizeof(availableWeapons[0]);

// Function to retrieve the damage of a given WeaponType
uint8_t getWeaponDamage(WeaponType type) {
  if (type == WeaponType::None) return 0;
  uint8_t index = static_cast<uint8_t>(type) - 1;
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Read the damage byte from PROGMEM
    return pgm_read_byte(&(availableWeapons[index].damage));
  }
  return 0;
}

// Function to retrieve the heat usage of a given WeaponType
// Should be inlined maybe?
uint8_t getWeaponHeat(WeaponType type) {
  if (type == WeaponType::None) return 0;
  uint8_t index = static_cast<uint8_t>(type) - 1;
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Read the heat byte from PROGMEM
    return pgm_read_byte(&(availableWeapons[index].heat));
  }
  return 0;  // Default heat if WeaponType is invalid
}

// Function to get weapon cost from WeaponType
uint8_t getWeaponCost(WeaponType type) {
  for (uint8_t i = 0; i < NUM_AVAILABLE_WEAPONS; ++i) {
    Weapon temp;
    memcpy_P(&temp, &availableWeapons[i], sizeof(Weapon));
    if (temp.type == type) {
      return temp.cost;
    }
  }
  return 0;
}

struct Player : Object {
  uint8_t dayCount = 0;
  uint24_t money = 0;
  MechType mechType;
  MechStatus mechStatus;
  PlayerMechStats playerMechStats;
  uint8_t health = 0;  // Player health
  
  uint8_t heat = 0;
  uint8_t angle = 0;
  uint8_t viewAngle = 0;

  // Initialize all weapon slots to None
  WeaponType playerWeapons[3] = { WeaponType::None, WeaponType::None, WeaponType::None };

  // Index of the current active weapon slot (0, 1, or 2)
  uint8_t currentWeaponSlot = 0;
} player;

void initializePlayerWeapons() {
  player.playerWeapons[0] = WeaponType::Bullets;
  player.playerWeapons[1] = WeaponType::Rockets;
  player.playerWeapons[2] = WeaponType::Laser;
}

uint8_t findNextWeaponSlot(uint8_t currentSlot) {
  // Attempt to find the next occupied slot
  for (uint8_t i = 1; i < 3; ++i) {  // Check the other two slots
    uint8_t nextSlot = (currentSlot + i) % 3;
    if (player.playerWeapons[nextSlot] != WeaponType::None) {
      return nextSlot;
    }
  }
  // If no other slots are occupied, return the current slot
  return currentSlot;
}

enum class EnemyState : uint8_t {
  Active,
  Inactive,
  Exploding
};

// Update Enemy struct to include MechType
struct Enemy : public Object {
  EnemyState state = EnemyState::Inactive;
  SQ7x8 moveSpeed = 0.0;  // Should be populated based on mechType during mission initialization
  uint8_t scaledSize = 0;
  uint16_t health = 0;  // Should be populated based on mechType during mission initialization
  uint8_t damage = 0;
  MechType mechType;

  uint8_t currentFrame = 0;
  uint8_t maxFrames = 0;
  uint8_t hitFlashTimer = 0;

  uint8_t flags;
};

// Initialize objects
const uint8_t maxEnemies = 3;  // Example enemy count
Enemy enemies[maxEnemies];     // Example enemy array

// Flag definitions for enemy circling behavior
#define ENEMY_CIRCLING_DIRECTION_BIT 0x01  // Bit 0: 0 for clockwise, 1 for counterclockwise
#define ENEMY_CIRCLING_ASSIGNED_BIT 0x02   // Bit 1: Circling direction assigned

// Global variable to track screen flash timer
uint8_t flashTimer = 0;

// Function to update enemy positions with approach and circling behavior
void updateEnemies() {
  const SQ7x8 enemySpeed = 0.05;      // Adjust as needed for enemy speed
  const SQ7x8 approachDistance = 20;  // Distance at which enemies start approaching the player
  const SQ7x8 circleDistance = 10;     // Distance at which enemies start circling the player
  const SQ15x16 approachDistanceSquared = (SQ15x16)approachDistance * approachDistance;
  const SQ15x16 circleDistanceSquared = (SQ15x16)circleDistance * circleDistance;

  // If flashTimer is active, handle screen inversion
  if (flashTimer > 0) {
    FX::enableOLED();
    arduboy.invert(true);
    FX::disableOLED();
    flashTimer--;
  } else {
    FX::enableOLED();
    arduboy.invert(false);
    FX::disableOLED();
  }

  for (uint8_t i = 0; i < maxEnemies; ++i) {
    if (enemies[i].state != EnemyState::Inactive && enemies[i].state != EnemyState::Exploding) {
      // Compute the difference in positions, accounting for world wrapping
      SQ7x8 dx = wrapDistance(player.x, enemies[i].x, worldWidth);
      SQ7x8 dy = wrapDistance(player.y, enemies[i].y, worldHeight);

      // Calculate distance squared between enemy and player
      SQ15x16 distanceSquared = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

      if (distanceSquared > approachDistanceSquared) {
        // Enemy is too far from player, they can wander or stand still
        // Reset circling assigned flag
        enemies[i].flags &= ~ENEMY_CIRCLING_ASSIGNED_BIT;

      } else if (distanceSquared > circleDistanceSquared) {
        // Enemy moves towards the player
        enemies[i].flags &= ~ENEMY_CIRCLING_ASSIGNED_BIT;

        // Compute the maximum of the absolute values of dx and dy
        SQ7x8 absDx = (dx >= 0) ? dx : -dx;
        SQ7x8 absDy = (dy >= 0) ? dy : -dy;
        SQ7x8 maxDelta = (absDx > absDy) ? absDx : absDy;

        if (maxDelta != 0) {
          // Normalize dx and dy to get direction vector
          SQ7x8 normDx = dx / maxDelta;
          SQ7x8 normDy = dy / maxDelta;

          // Move enemy towards player
          enemies[i].x += normDx * enemySpeed;
          enemies[i].y += normDy * enemySpeed;
        }

      } else {
        // Enemy circles around the player
        if (random(0, 100) < 10) {  // 10% chance to attack each frame
          if (player.health > 0)
            if (player.health < enemies[i].damage) {
              flashTimer = 5;  // Set flash timer for 5 frames (adjust as needed)
              player.health = 0;
            } else {
              flashTimer = 5;  // Set flash timer for 5 frames (adjust as needed)
              // Damage level should be based on mech type and player armor
              player.health -= enemies[i].damage;  // Reduce player health
            }
        }

        // Assign circling direction if not already assigned
        if (!(enemies[i].flags & ENEMY_CIRCLING_ASSIGNED_BIT)) {
          // Randomly assign circling direction
          if (random(0, 2) == 0) {
            enemies[i].flags &= ~ENEMY_CIRCLING_DIRECTION_BIT;  // Set direction to 0 (clockwise)
          } else {
            enemies[i].flags |= ENEMY_CIRCLING_DIRECTION_BIT;  // Set direction to 1 (counterclockwise)
          }
          enemies[i].flags |= ENEMY_CIRCLING_ASSIGNED_BIT;  // Set assigned flag
        }

        // Compute the perpendicular vector to (dx, dy)
        SQ7x8 perpDx, perpDy;

        if (enemies[i].flags & ENEMY_CIRCLING_DIRECTION_BIT) {
          perpDx = dy;
          perpDy = -dx;
        } else {
          perpDx = -dy;
          perpDy = dx;
        }

        // Compute the maximum of the absolute values of perpDx and perpDy
        SQ7x8 absPerpDx = (perpDx >= 0) ? perpDx : -perpDx;
        SQ7x8 absPerpDy = (perpDy >= 0) ? perpDy : -perpDy;
        SQ7x8 maxPerpDelta = (absPerpDx > absPerpDy) ? absPerpDx : absPerpDy;

        if (maxPerpDelta != 0) {
          // Normalize the perpendicular vector
          SQ7x8 normPerpDx = perpDx / maxPerpDelta;
          SQ7x8 normPerpDy = perpDy / maxPerpDelta;

          // Move enemy along the perpendicular vector
          enemies[i].x += normPerpDx * enemySpeed;
          enemies[i].y += normPerpDy * enemySpeed;
        }
      }

      // Wrap coordinates at world boundaries
      enemies[i].x = wrapCoordinate(enemies[i].x, worldWidth);
      enemies[i].y = wrapCoordinate(enemies[i].y, worldHeight);
    }
  }
}

// TODO: Simplify mission by removing activeEnemies and decrementing numEnemies instead?
struct Mission {
  uint8_t numEnemies;
  uint8_t activeEnemies;
  MechType mechs[maxEnemies];  // Assuming a max of 3 mechs per mission
  uint16_t reward;
};

const uint8_t maxMissions = 3;  // Maximum number of missions available at any time
Mission availableMissions[maxMissions];
Mission currentMission;

// Modify generateMission to include MechType
Mission generateMission();

// Function to generate a mission
Mission generateMission() {
  Mission mission;
  uint8_t totalDifficulty = 0;  // Accumulate difficulty here

  if (player.dayCount < 10) {
    mission.numEnemies = random(1, 3);  // 1 to 2 light mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
      totalDifficulty += 1;  // Difficulty value for Mothra
    }
  } else if (player.dayCount >= 10 && player.dayCount < 20) {
    mission.numEnemies = random(2, 4);  // 2 to 3 light mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
      totalDifficulty += 1;
    }
  } else if (player.dayCount >= 15 && player.dayCount < 30) {
    mission.numEnemies = random(1, 4);  // 1 to 3 light or medium mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      if (random(0, 2) == 0) {
        mission.mechs[i] = MechType::Battle_Cat;
        totalDifficulty += 2;  // Difficulty value for Battle_Cat
      } else {
        mission.mechs[i] = MechType::Mothra;
        totalDifficulty += 1;
      }
    }
  } else if (player.dayCount >= 30 && player.dayCount < 40) {
    mission.numEnemies = random(1, 4);  // 1 to 3 medium or heavy mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      if (random(0, 2) == 0) {
        mission.mechs[i] = MechType::Battle_Cat;
        totalDifficulty += 2;
      } else {
        mission.mechs[i] = MechType::Thor_Hammer;
        totalDifficulty += 3;  // Difficulty value for Thor_Hammer
      }
    }
  }
  // Add more conditions for other day count ranges as needed

  // Set mission reward based on totalDifficulty
  mission.reward = totalDifficulty * 1000;  // For example, 50 credits per difficulty point

  return mission;
}

// Function to populate the list of available missions
void populateMissionList() {
  for (uint8_t i = 0; i < maxMissions; i++) {
    availableMissions[i] = generateMission();
  }
}

void initNewGame() {
  player.dayCount = 1;
  player.money = 20000;
  //player.mechType = MechType::Mothra;
  //player.playerMechStats = playerMothraStats;

  //player.mechType = MechType::Battle_Cat;
  //player.playerMechStats = playerBattleCatStats;

  player.mechType = MechType::Thor_Hammer;
  player.playerMechStats = playerThorHammerStats;

  initializePlayerWeapons();
  player.mechStatus = MechStatus::Normal;
  player.health = 100;

  populateMissionList();  // Populate the list of available missions
}

struct SaveData {
  uint8_t dayCount = 0;
  uint24_t money = 0;
  MechType mechType;
  PlayerMechStats saveMechStats;
  uint8_t health = 0;
  Mission savedMissions[maxMissions];  // Add mission list to save data
  WeaponType weapons[3];
};

SaveData saveData;

void initSaveGame() {
  saveData.dayCount = player.dayCount;
  saveData.money = player.money;
  saveData.mechType = player.mechType;
  saveData.saveMechStats = player.playerMechStats;
  saveData.health = player.health;

  for (uint8_t i = 0; i < 3; i++) {
    saveData.weapons[i] = player.playerWeapons[i];
  }
  for (uint8_t i = 0; i < maxMissions; i++) {
    saveData.savedMissions[i] = availableMissions[i];
  }
}

void updateSaveGame() {
  FX::saveGameState(saveData);
  currentState = GameState::Main_Menu;
}

void initLoadGame() {
  // If there is no save data, display message. If there is save data, load it.
  if (!FX::loadGameState(saveData)) {
    currentState = GameState::No_Save_Screen;  // You should add this new state to your GameState enum.
  } else {
    updateLoadGame();  // Proceed to load the game data if it exists.
    currentState = GameState::Main_Menu;
  }
}

void updateLoadGame() {
  FX::loadGameState(saveData);
  player.dayCount = saveData.dayCount;
  player.money = saveData.money;
  player.mechType = saveData.mechType;
  player.playerMechStats = saveData.saveMechStats;
  player.health = saveData.health;

  for (uint8_t i = 0; i < 3; i++) {
    player.playerWeapons[i] = saveData.weapons[i];
  }
  for (uint8_t i = 0; i < maxMissions; i++) {
    availableMissions[i] = saveData.savedMissions[i];
  }
}

void updateNoSaveScreen() {
  //FX::setFont(font_4x6, dcmNormal); // select default font
  //FX::setCursor(5, 5);
  //FX::setCursorRange(0,80);         // set cursor left and wrap positions
  FX::drawBitmap(5, 5, noSaveDataStr, 0, dbmNormal);
  //FX::drawString(noSaveDataStr);
  if (player.dayCount == 0 && arduboy.justPressed(B_BUTTON)) {
    currentState = GameState::Title_Menu;
  } else if (player.dayCount > 0 && arduboy.justPressed(B_BUTTON)) {
    currentState = GameState::Main_Menu;
  }
}

void updateTitleMenu() {
  FX::drawBitmap(0, 0, titleScreen128x64, 0, dbmMasked);

  if (arduboy.justPressed(A_BUTTON)) {
    currentState = GameState::Init_New_Game;
  }
  if (arduboy.justPressed(B_BUTTON)) {
    currentState = GameState::Init_Load_Game;
  }
}

// Place all struct and enum declarations at the top
struct MenuItem {
  int16_t x, y, width, height;  // Position and size for the selection rectangle
};

struct Menu {
  const MenuItem* items;     // Pointer to array of menu items in PROGMEM
  uint8_t itemCount;         // Total number of items in the menu
  uint8_t currentSelection;  // Index of the currently selected item
};

enum class Direction { UP,
                       DOWN,
                       LEFT,
                       RIGHT };

// Provide function prototypes
bool rangesOverlap(int16_t a_start, int16_t a_end, int16_t b_start, int16_t b_end);
uint8_t findNextMenuItem(Menu& menu, uint8_t currentSelection, Direction dir);
void updateMenu(Menu& menu);
void updateMainMenu();

// Example of defining menus
const MenuItem mainMenuItems[] PROGMEM = {
  { 10, 18, 26, 26 },  // First menu item
  { 51, 18, 26, 26 },  // Second menu item
  { 93, 18, 26, 26 }   // Third menu item
};
Menu mainMenu = { mainMenuItems, 3, 0 };

const MenuItem saveLoadMenuItems[] PROGMEM = {
  { 20, 28, 23, 9 },
  { 84, 28, 23, 9 }
};
Menu saveLoadMenu = { saveLoadMenuItems, 2, 0 };

const MenuItem missionMenuItems[] PROGMEM = {
  { 1, 1, 33, 17 },
  { 1, 18, 33, 17 },
  { 1, 35, 33, 17 }
};
Menu missionMenu = { missionMenuItems, 3, 0 };

const MenuItem hangerMenuItems[] PROGMEM = {
  { 61, 0, 33, 17 },   // Repair/reload
  { 61, 17, 33, 17 },  // Customize menu
  { 94, 0, 33, 17 },   // Buy Mech
  { 94, 17, 33, 17 },  // Sell Mech
  { 61, 34, 33, 17 },  // Cycle mech left arrow
  { 94, 34, 33, 17 }   // Cycle mech right arrow
};
Menu hangerMenu = { hangerMenuItems, 6, 0 };

const MenuItem customizationMenuItems[] PROGMEM = {
  { 79, 1, 33, 15 },   // Armor
  { 79, 16, 33, 15 },  // Buy/Sell Weapons
  { 79, 31, 33, 15 },  // Heatsink
  { 79, 46, 33, 15 }   // Engine
};
Menu customizationMenu = { customizationMenuItems, 4, 0 };

// Implement the helper function
bool rangesOverlap(int16_t a_start, int16_t a_end, int16_t b_start, int16_t b_end) {
  return (a_start < b_end) && (b_start < a_end);
}

// Implement findNextMenuItem function
uint8_t findNextMenuItem(Menu& menu, uint8_t currentSelection, Direction dir) {
  MenuItem currentItem;
  memcpy_P(&currentItem, &menu.items[currentSelection], sizeof(MenuItem));

  uint8_t bestIndex = currentSelection;
  int16_t bestDistance = INT16_MAX;  // Max possible value
  for (uint8_t i = 0; i < menu.itemCount; i++) {
    if (i == currentSelection) continue;
    MenuItem candidateItem;
    memcpy_P(&candidateItem, &menu.items[i], sizeof(MenuItem));

    bool isCandidate = false;
    int16_t distance = 0;

    switch (dir) {
      case Direction::UP:
        if (candidateItem.y + candidateItem.height <= currentItem.y && rangesOverlap(currentItem.x, currentItem.x + currentItem.width, candidateItem.x, candidateItem.x + candidateItem.width)) {
          isCandidate = true;
          distance = currentItem.y - (candidateItem.y + candidateItem.height);
        }
        break;
      case Direction::DOWN:
        if (candidateItem.y >= currentItem.y + currentItem.height && rangesOverlap(currentItem.x, currentItem.x + currentItem.width, candidateItem.x, candidateItem.x + candidateItem.width)) {
          isCandidate = true;
          distance = candidateItem.y - (currentItem.y + currentItem.height);
        }
        break;
      case Direction::LEFT:
        if (candidateItem.x + candidateItem.width <= currentItem.x && rangesOverlap(currentItem.y, currentItem.y + currentItem.height, candidateItem.y, candidateItem.y + candidateItem.height)) {
          isCandidate = true;
          distance = currentItem.x - (candidateItem.x + candidateItem.width);
        }
        break;
      case Direction::RIGHT:
        if (candidateItem.x >= currentItem.x + currentItem.width && rangesOverlap(currentItem.y, currentItem.y + currentItem.height, candidateItem.y, candidateItem.y + candidateItem.height)) {
          isCandidate = true;
          distance = candidateItem.x - (currentItem.x + currentItem.width);
        }
        break;
    }

    if (isCandidate && distance < bestDistance) {
      bestDistance = distance;
      bestIndex = i;
    }
  }
  return bestIndex;
}

// Update the updateMenu function
void updateMenu(Menu& menu) {
  MenuItem selectedItem;
  memcpy_P(&selectedItem, &menu.items[menu.currentSelection], sizeof(MenuItem));
  arduboy.drawRect(selectedItem.x, selectedItem.y, selectedItem.width, selectedItem.height, WHITE);

  uint8_t newSelection = menu.currentSelection;
  if (arduboy.justPressed(UP_BUTTON)) {
    newSelection = findNextMenuItem(menu, menu.currentSelection, Direction::UP);
  } else if (arduboy.justPressed(DOWN_BUTTON)) {
    newSelection = findNextMenuItem(menu, menu.currentSelection, Direction::DOWN);
  } else if (arduboy.justPressed(LEFT_BUTTON)) {
    newSelection = findNextMenuItem(menu, menu.currentSelection, Direction::LEFT);
  } else if (arduboy.justPressed(RIGHT_BUTTON)) {
    newSelection = findNextMenuItem(menu, menu.currentSelection, Direction::RIGHT);
  }

  menu.currentSelection = newSelection;
}

// Your updateMainMenu function remains largely the same
void updateMainMenu() {
  // Explicitly set invert to false just to make sure
  FX::enableOLED();
  arduboy.invert(false);
  FX::disableOLED();

  FX::drawBitmap(0, 0, mainMenu128x64, 0, dbmMasked);

  //FX::setCursor(10, 50);  // Adjust position as needed
  //FX::drawString(dayStr);
  FX::drawBitmap(10, 50, dayStr, 0, dbmNormal);
  font4x6.setCursor(30, 49);  // Adjust position as needed
  font4x6.print(player.dayCount);

  updateMenu(mainMenu);

  if (arduboy.justPressed(A_BUTTON)) {
    switch (mainMenu.currentSelection) {
      case 0:
        currentState = GameState::Save_Load_Submenu;
        break;
      case 1:
        currentState = GameState::Mission_Submenu;
        break;
      case 2:
        currentState = GameState::Hanger_Submenu;
        break;
    }
  }
}

void updateSaveLoadMenu() {
  FX::drawBitmap(0, 0, saveLoadMenu128x64, 0, dbmMasked);
  updateMenu(saveLoadMenu);
  if (arduboy.justPressed(B_BUTTON)) {
    currentState = previousState;
  }

  if (arduboy.justPressed(A_BUTTON)) {
    // Handle selection based on mainMenu.currentSelection
    switch (saveLoadMenu.currentSelection) {
      case 0:
        initSaveGame();
        updateSaveGame();
        break;
      case 1:
        initLoadGame();
        break;
    }
  }
}

void initializeMission(const Mission& mission);

void initializeMission(const Mission& mission) {
  player.x = random(worldWidth);   // Random player X position
  player.y = random(worldHeight);  // Random player Y position
  player.heat = 0;                 // Reset player heat to 0
  player.mechStatus = MechStatus::Normal;

  // Simplify by just using numEnemies directly?
  currentMission.activeEnemies = mission.numEnemies;

  for (uint8_t i = 0; i < maxEnemies; ++i) {
    if (i < mission.numEnemies) {
      enemies[i].state = EnemyState::Active;
      enemies[i].mechType = mission.mechs[i];
      // Randomly set enemy positions within world bounds
      enemies[i].x = random(worldWidth);
      enemies[i].y = random(worldHeight);
      enemies[i].hitFlashTimer = 0;  // Ensure no flash on spawn

      uint8_t baseDamage;

      switch (enemies[i].mechType) {
        case MechType::Mothra:
          enemies[i].health = 100;
          enemies[i].moveSpeed = 0.1;
          baseDamage = 5;  // Base damage for Mothra
          break;
        case MechType::Battle_Cat:
          enemies[i].health = 200;
          enemies[i].moveSpeed = 0.05;
          baseDamage = 10;  // Base damage for BattleCat
          break;
        case MechType::Thor_Hammer:
          enemies[i].health = 300;
          enemies[i].moveSpeed = 0.02;
          baseDamage = 15;  // Base damage for Thor Hammer
          break;
      }

      // Calculate enemy damage based on baseDamage and player armor
      const uint8_t maxArmor = player.playerMechStats.maxArmor;
      uint8_t playerArmor = player.playerMechStats.armor;
      enemies[i].damage = (uint8_t)((baseDamage * (maxArmor - playerArmor + 1)) / maxArmor);

      // Ensure that damage is at least 1
      if (enemies[i].damage < 1) {
        enemies[i].damage = 1;
      }
    } /*else {
      enemies[i].state = EnemyState::Inactive;
    }*/
  }
}

void completeMission() {
  // Make sure screen is set back to normal
  FX::enableOLED();
  arduboy.invert(false);
  FX::disableOLED();

  // Example logic for completing a mission
  player.mechStatus = MechStatus::Normal;
  player.money += currentMission.reward;  // Award money to player (needs to be more randomized and populated when mission is generated)
  populateMissionList();                  // Generate new set of missions
  currentState = GameState::Main_Menu;    // Return to main menu
  ++player.dayCount;
}

/*static void printString(Font4x6 &font, uint24_t addr, uint8_t x, uint8_t y) {
    uint8_t index = 0;
    unsigned char character;
    FX::seekData(addr);
    while ((character = FX::readPendingUInt8()) != '\0') {
        font.printChar(character, x, y);
        x += 5;
    }
    (void)FX::readEnd();
}*/

uint8_t selectedMissionIndex = 0;  // Index of the currently selected mission

void updateMissionMenu() {
  FX::drawBitmap(0, 0, missionMenu128x64, 0, dbmMasked);
  updateMenu(missionMenu);

  // Directly print the label for enemies
  //FX::setCursor(38, 10);  // Adjust position as needed
  //FX::drawString(enemiesStr);
  FX::drawBitmap(40, 5, enemiesStr, 0, dbmNormal);
  font4x6.setCursor(80, 4);
  font4x6.print(availableMissions[selectedMissionIndex].numEnemies);

  // For enemy types, print directly in a loop
  for (uint8_t i = 0; i < availableMissions[selectedMissionIndex].numEnemies; i++) {
    uint8_t yPos = 15 + (i * 10);  // Adjust 10 to match your bitmap height + desired padding
    // Instead of storing type names in a buffer, print directly
    switch (availableMissions[selectedMissionIndex].mechs[i]) {
      case MechType::Mothra:
        //printString(font4x6, mothraStr, 38, 25);
        //font4x6.print(F("\n Mothra"));
        //font4x6.print(F("\n "));
        FX::drawBitmap(42, yPos, mothraStr, 0, dbmNormal);
        //FX::drawString(mothraStr);
        break;
      case MechType::Battle_Cat:
        //printString(font4x6, battleCatStr, 38, 25);
        //font4x6.print(F("\n BattleCat"));
        //font4x6.print(F("\n "));
        FX::drawBitmap(42, yPos, battleCatStr, 0, dbmNormal);
        //FX::drawString(battleCatStr);
        break;
      case MechType::Thor_Hammer:
        //printString(font4x6, battleCatStr, 38, 25);
        //font4x6.print(F("\n ThorHammer"));
        //font4x6.print(F("\n "));
        FX::drawBitmap(42, yPos, thorHammerStr, 0, dbmNormal);
        //FX::drawString(thorHammerStr);        
        break;
        // Add cases for other mech types as needed
    }
  }
  font4x6.setCursor(52, 40);
  font4x6.print(availableMissions[selectedMissionIndex].reward);

  if (arduboy.justPressed(B_BUTTON)) {
    currentState = previousState;
  }
  if (arduboy.justPressed(A_BUTTON)) {
    switch (missionMenu.currentSelection) {
      case 0:  // Deploy
        currentMission = availableMissions[selectedMissionIndex];
        initializeMission(currentMission);
        currentState = GameState::Game_Play;
        break;
      case 1:  // Left arrow
        if (selectedMissionIndex > 0) {
          selectedMissionIndex--;
        } else {
          selectedMissionIndex = maxMissions - 1;
        }
        break;
      case 2:  // Right arrow
        if (selectedMissionIndex < maxMissions - 1) {
          selectedMissionIndex++;
        } else {
          selectedMissionIndex = 0;
        }
        break;
    }
  }
}

// TO DO: Replace some menu elements with sprites to save RAM?
bool inRepairMenu = false;
bool notEnoughMoneyMessage = false;
uint16_t repairCost = 0;  // Global variable to store repair cost

void updateHangerMenu() {
  FX::drawBitmap(0, 0, hangerMenu128x64, 0, dbmMasked);

  switch (player.mechType) {
    case MechType::Mothra:
      // Use larger size and center it
      FX::drawBitmap(4, 4, Mothra30x40, 0, dbmMasked);
      font4x6.setCursor(42, 4);
      font4x6.print(player.health);
      FX::drawBitmap(4, 48, mothraStr, 0, dbmNormal);
      break;
    // Use larger size and center it
    case MechType::Battle_Cat:
      FX::drawBitmap(4, 4, Battle_Cat30x40, 0, dbmMasked);
      font4x6.setCursor(42, 4);
      font4x6.print(player.health);
      FX::drawBitmap(4, 48, battleCatStr, 0, dbmNormal);
      break;
    case MechType::Thor_Hammer:
      FX::drawBitmap(4, 4, Thor_Hammer30x40, 0, dbmMasked);
      font4x6.setCursor(42, 4);
      font4x6.print(player.health);
      FX::drawBitmap(4, 48, thorHammerStr, 0, dbmNormal);      
      break;
  }

  if (!inRepairMenu && !notEnoughMoneyMessage) {
    updateMenu(hangerMenu);
  }

  if (arduboy.justPressed(B_BUTTON)) {
    if (notEnoughMoneyMessage) {
      notEnoughMoneyMessage = false;  // Exit "Not enough money!" message
    } else if (inRepairMenu) {
      inRepairMenu = false;  // Exit repair menu
    } else {
      currentState = previousState;
    }
  }

  if (arduboy.justPressed(A_BUTTON)) {
    if (notEnoughMoneyMessage) {
      notEnoughMoneyMessage = false;  // Exit "Not enough money!" message
    } else if (!inRepairMenu) {
      switch (hangerMenu.currentSelection) {
        case 0:
          // Enter repair menu
          inRepairMenu = true;
          // Calculate repair cost once
          {
            uint16_t repairCostPerHealthPoint = 100;  // Adjust as needed
            uint8_t missingHealth = 100 - player.health;
            repairCost = missingHealth * repairCostPerHealthPoint;
          }
          break;
        case 1:
          currentState = GameState::Customization_Submenu;
          break;
        case 2:
          // Buy mech logic here
          break;
        case 3:
          // Sell mech logic here
          break;
        case 4: 
          // Next mech logic here
          break;
        case 5:
          // Prev mech logic here
          break;
      }
    } else {
      // Confirm repair
      if (player.money >= repairCost) {
        player.money -= repairCost;
        player.health = 100;
        inRepairMenu = false;  // Exit repair menu
      } else {
        // Not enough money message
        notEnoughMoneyMessage = true;
      }
    }
  }

  if (inRepairMenu) {
    // Calculate the number of digits in repairCost
    uint8_t digits;
    if (repairCost == 0) {
      digits = 2;
    } else if (repairCost < 10) {
      digits = 3;
    } else if (repairCost < 100) {
      digits = 4;
    } else if (repairCost < 1000) {
      digits = 5;
    } else if (repairCost < 10000) {
      digits = 6;
    } else {
      digits = 7;  // Max value for uint16_t is 65535
    }

    // Total text length: "Cost: " (6 characters) + number of digits
    uint8_t textLength = 6 + digits;

    // Each character advances the cursor by 4 pixels
    uint8_t textWidth = textLength * 4;

    // Add padding to the box width
    uint8_t boxWidth = textWidth + 6;  // 2 pixels padding on each side

    // Position the box (centered horizontally)
    uint8_t x = (WIDTH - boxWidth) / 2;
    uint8_t y = 39;  // Same Y position as before

    // Display repair cost
    arduboy.drawRect(x - 1, y - 1, boxWidth + 2, 22, WHITE);  // Clear rectangle area for the message
    arduboy.fillRect(x, y, boxWidth, 20, BLACK);              // Clear rectangle area for the message

    FX::drawBitmap(x + 2, y + 1, costStr, 0, dbmNormal);
    font4x6.setCursor(x + 30, y);  // Add padding inside the box
    font4x6.print(repairCost);

    // Prompt for confirmation
    FX::drawBitmap(x + 2, y + 11, pressAStr, 0, dbmNormal);
  }

  if (notEnoughMoneyMessage) {
    // Clear previous text if necessary
    arduboy.fillRect(9, 39, 86, 20, BLACK);  // Clear a rectangle area for the message

    // Display the "Not enough money!" message
    FX::drawBitmap(10, 40, notEnoughMoneyStr, 0, dbmNormal);
    FX::drawBitmap(10, 50, AorBStr, 0, dbmNormal);
  }
}

// Global Weapon Menu Flags and Variables
bool inWeaponMenu = false;             // Indicates if the player is in the weapon menu
bool viewingAvailableWeapons = false;  // Indicates if the player is viewing available weapons for purchase
bool sellingWeapon = false;            // Indicates if the player is in the process of selling a weapon
uint8_t selectedWeaponSlot = 0;        // Currently selected weapon slot (0, 1, or 2)
uint8_t selectedAvailableWeapon = 0;   // Currently selected weapon in the available weapons list
// Add this flag at the top of your code (e.g., in the global variables section)
bool justEnteredWeaponMenu = false;

// Existing flags
bool adjustingStat = false;

// Function to retrieve Weapon from availableWeapons array safely
Weapon getAvailableWeapon(uint8_t index) {
  Weapon temp = { WeaponType::None, 0, 0, 0, 0, 0 };  // Initialize with default values
  if (index < NUM_AVAILABLE_WEAPONS) {
    memcpy_P(&temp, &availableWeapons[index], sizeof(Weapon));
  }
  return temp;
}

// Function to display weapon slots
void displayWeaponSlots(uint8_t selectedSlot) {
  arduboy.fillRect(8, 8, 70, 50, BLACK);
  arduboy.drawRect(7, 7, 70, 50, WHITE);
  //font4x6.setCursor(10, 10);
  //font4x6.print(F("Weapon Slots:"));
  FX::drawBitmap(10, 10, weaponSlotsStr, 0, dbmNormal);

  for (uint8_t i = 0; i < 3; i++) {
    uint8_t y = 20 + i * 12;

    // Highlight selected slot
    if (i == selectedSlot) {
      arduboy.drawRect(8, y - 1, 68, 10, WHITE);
    }

    // Retrieve weapon type
    WeaponType type = player.playerWeapons[i];
    //const __FlashStringHelper* weaponName;
    uint24_t weaponName;
    switch (type) {
      case WeaponType::None:
        weaponName = emptyStr;
        break;
      case WeaponType::Bullets:
        weaponName = lightMGStr;
        break;
      case WeaponType::HeavyBullets:
        weaponName = heavyMGStr;
        break;
      case WeaponType::Rockets:
        weaponName = rocketStr;
        break;
      case WeaponType::MediumRockets:
        weaponName = medRocketStr;
        break;
      // Add cases for additional weapon types here
      case WeaponType::Laser:
        weaponName = laserStr;
        break;
      //default:
        //weaponName = F("Unknown");
        //break;
    }
    FX::drawBitmap(12, y, weaponName, 0, dbmNormal);
    //font4x6.setCursor(12, y);
    //font4x6.print(weaponName);
  }
}

// Function to display available weapons for purchase
/*void displayAvailableWeapons(uint8_t selectedWeapon) {
  arduboy.fillRect(8, 1, 90, 60, BLACK);
  arduboy.drawRect(7, 1, 90, 60, WHITE);
  //font4x6.setCursor(10, 2);
  //font4x6.print(F("Available Weapons:"));
  FX::drawBitmap(10, 2, availableWeaponsStr, 0, dbmNormal);

  // Limit the number of weapons displayed based on the screen size
  // For simplicity, assume all weapons fit; implement pagination if necessary
  for (uint8_t i = 0; i < NUM_AVAILABLE_WEAPONS; i++) {
    uint8_t y = 11 + i * 10;

    // Highlight selected weapon
    if (i == selectedWeapon) {
      arduboy.drawRect(8, y - 1, 60, 10, WHITE);
    }

    // Retrieve weapon name directly from the availableWeapons array
    Weapon tempWeapon = getAvailableWeapon(i);
    uint24_t weaponName;

    switch (tempWeapon.type) {
      case WeaponType::None:
        weaponName = emptyStr;
        break;
      case WeaponType::Bullets:
        weaponName = lightMGStr;
        break;
      case WeaponType::HeavyBullets:
        weaponName = heavyMGStr;
        break;
      case WeaponType::Rockets:
        weaponName = rocketStr;
        break;
      case WeaponType::MediumRockets:
        weaponName = medRocketStr;
        break;
      // Add cases for additional weapon types here
      case WeaponType::Laser:
        weaponName = laserStr;
        break;
      //default:
        //weaponName = F("Unknown");
        //break;
    }

    FX::drawBitmap(12, y, weaponName, 0, dbmNormal);
  }

  // Display selected weapon stats
  if (selectedWeapon < NUM_AVAILABLE_WEAPONS) {
    Weapon selected = getAvailableWeapon(selectedWeapon);
    uint8_t yStats = 15;
    arduboy.fillRect(60, yStats - 2, 38, 24, BLACK);
    arduboy.drawRect(60, yStats - 2, 38, 24, WHITE);
    FX::drawBitmap(62, yStats, dmgStr, 0, dbmNormal);
    font4x6.setCursor(86, yStats - 1);
    font4x6.print(selected.damage);

    FX::drawBitmap(62, yStats + 8, heatStr, 0, dbmNormal);
    font4x6.setCursor(86, yStats + 7);
    font4x6.print(selected.heat);

  }
}*/

// Function to display available weapons for purchase
void displayAvailableWeapons(uint8_t selectedWeapon) {
  // Draw the sprite containing all the weapon names
  FX::drawBitmap(8, 1, availableWeaponsMenu90x60, 0, dbmMasked);

  // Highlight the selected weapon
  if (selectedWeapon < NUM_AVAILABLE_WEAPONS) {
    uint8_t y = 10 + selectedWeapon * 8; // Adjust this if the positions in your sprite differ
    arduboy.drawRect(8, y, 60, 8, WHITE);
  }

  // Display selected weapon stats
  if (selectedWeapon < NUM_AVAILABLE_WEAPONS) {
    Weapon selected = getAvailableWeapon(selectedWeapon);
    uint8_t yStats = 15;
    arduboy.fillRect(58, yStats - 2, 33, 24, BLACK);
    arduboy.drawRect(58, yStats - 2, 33, 24, WHITE);

    FX::drawBitmap(60, yStats, dmgStr, 0, dbmNormal);
    font4x6.setCursor(84, yStats - 1);
    font4x6.print(selected.damage);

    FX::drawBitmap(60, yStats + 8, heatStr, 0, dbmNormal);
    font4x6.setCursor(80, yStats + 7);
    font4x6.print(selected.heat);
  }
}

// Function to display sell confirmation
void displaySellConfirmation(uint8_t slot) {
  // Clear and draw the confirmation box
  arduboy.fillRect(10, 25, 83, 20, BLACK);  // Clear area
  arduboy.drawRect(10, 25, 83, 20, WHITE);

  //font4x6.setCursor(12, 27);
  //font4x6.print(F("Sell this weapon?"));
  FX::drawBitmap(12, 27, sellThisWeaponStr, 0, dbmNormal);
  //font4x6.setCursor(12, 35);
  //font4x6.print(F("A: Yes  B: No"));
  FX::drawBitmap(12, 35, AyesBnoStr, 0, dbmNormal);
}

// Function to display "Not Enough Money" message
void displayNotEnoughMoney() {
  // Clear and draw the message box
  arduboy.fillRect(9, 39, 86, 20, BLACK);  // Clear a rectangle area for the message
  arduboy.drawRect(9, 39, 86, 20, WHITE);

  //font4x6.setCursor(10, 40);
  //font4x6.print(F("Not enough money!"));
  FX::drawBitmap(10, 40, notEnoughMoneyStr, 0, dbmNormal);
  //font4x6.setCursor(10, 50);
  //font4x6.print(F("Press A/ or B"));
  FX::drawBitmap(10, 50, AorBStr, 0, dbmNormal);
}

// Updated updateCustomizationMenu() Function
void updateCustomizationMenu() {
  FX::drawBitmap(0, 0, customizationMenu128x64, 0, dbmMasked);

  //FX::drawBitmap(2, 1, armorStr, 0, dbmNormal);
  //font4x6.print(F("Armor     "));
  font4x6.setCursor(50, 0);
  font4x6.print(player.playerMechStats.armor);

  //FX::drawBitmap(2, 8, weightStr, 0, dbmNormal);
  font4x6.setCursor(50, 7);
  //font4x6.print(F("Weight    "));
  font4x6.print(player.playerMechStats.weight);

  //FX::drawBitmap(2, 15, heatSinkStr, 0, dbmNormal);
  font4x6.setCursor(50, 14);
  //font4x6.print(F("Heatsink  "));
  font4x6.print(player.playerMechStats.heatSink);

  //FX::drawBitmap(2, 22, moveSpeedStr, 0, dbmNormal);
  font4x6.setCursor(50, 21);
  //font4x6.print(F("MoveSpeed "));
  font4x6.print(static_cast<float>(player.playerMechStats.moveSpeed));

  //FX::drawBitmap(2, 36, creditsStr, 0, dbmNormal);
  font4x6.setCursor(50, 34);
  //font4x6.print(F("Credits  "));
  font4x6.print(static_cast<long>(player.money));

  // Update the main customization menu if not adjusting stats or in weapon menu
  if (!adjustingStat && !inWeaponMenu) {
    updateMenu(customizationMenu);
  }

  // Handle B_BUTTON presses
  if (arduboy.justPressed(B_BUTTON)) {
    if (adjustingStat) {
      adjustingStat = false;
    } else if (inWeaponMenu) {
      if (viewingAvailableWeapons || sellingWeapon) {
        // Exit submenus within the weapon menu
        viewingAvailableWeapons = false;
        sellingWeapon = false;
      } else {
        // Exit weapon menu
        inWeaponMenu = false;
      }
    } else {
      currentState = previousState;
    }
  }

  // Handle A_BUTTON presses
  // Inside the A_BUTTON handling for the customization menu
  if (!inWeaponMenu && !adjustingStat) {
    if (arduboy.justPressed(A_BUTTON)) {
      switch (customizationMenu.currentSelection) {
        case 0:
          adjustingStat = true;  // Adjust Armor
          break;
        case 1:
          inWeaponMenu = true;  // Enter Weapon Menu
          selectedWeaponSlot = 0;
          justEnteredWeaponMenu = true;  // Set the debounce flag
          break;
        case 2:
          adjustingStat = true;  // Adjust Heatsink
          break;
      }
    }
  }

  // Handle adjusting stats (Armor and Heatsink)
  if (adjustingStat && !inWeaponMenu) {
    switch (customizationMenu.currentSelection) {
      case 0:
        {  // Armor up/down (weight up/down, speed down/up)
          if (arduboy.justPressed(RIGHT_BUTTON)) {
            if (player.playerMechStats.armor < player.playerMechStats.maxArmor && player.playerMechStats.weight > 1 && player.playerMechStats.moveSpeed >= 0.01 && player.money >= 100) {  // Ensure player has enough money
              FX::drawBitmap(112, 1, rightArrowSmall, 0, dbmMasked);
              player.playerMechStats.armor++;
              player.playerMechStats.weight++;
              player.playerMechStats.moveSpeed -= 0.01;
              player.money -= 100;

              // Clamp moveSpeed to zero if it goes negative
              if (player.playerMechStats.moveSpeed < 0) {
                player.playerMechStats.moveSpeed = 0;
              }
            }
          }
          if (arduboy.justPressed(LEFT_BUTTON)) {
            if (player.playerMechStats.armor > 1) {
              FX::drawBitmap(72, 1, leftArrowSmall, 0, dbmMasked);
              player.playerMechStats.armor--;
              player.playerMechStats.weight--;
              player.playerMechStats.moveSpeed += 0.01;
              player.money += 100;
            }
          }
          break;
        }
      case 2:
        {  // Heatsink up/down (weight up/down, speed down/up)
          if (arduboy.justPressed(RIGHT_BUTTON)) {
            if (player.playerMechStats.heatSink < player.playerMechStats.maxHeatSink && player.playerMechStats.weight > 1 && player.playerMechStats.moveSpeed >= 0.01 && player.money >= 100) {  // Ensure player has enough money
              FX::drawBitmap(112, 32, rightArrowSmall, 0, dbmMasked);
              player.playerMechStats.heatSink++;
              player.playerMechStats.weight++;
              player.playerMechStats.moveSpeed -= 0.01;
              player.money -= 100;

              // Clamp moveSpeed to zero if it goes negative
              if (player.playerMechStats.moveSpeed < 0) {
                player.playerMechStats.moveSpeed = 0;
              }
            }
          }
          if (arduboy.justPressed(LEFT_BUTTON)) {
            if (player.playerMechStats.heatSink > 1) {
              FX::drawBitmap(72, 32, leftArrowSmall, 0, dbmMasked);
              player.playerMechStats.heatSink--;
              player.playerMechStats.weight--;
              player.playerMechStats.moveSpeed += 0.01;
              player.money += 100;
            }
          }
          break;
        }
    }
  }

  // Handle Weapon Menu
  // Inside the Weapon Menu handling
  if (inWeaponMenu) {
    if (!viewingAvailableWeapons && !sellingWeapon) {
      // Display Weapon Slots
      displayWeaponSlots(selectedWeaponSlot);

      // Navigate through weapon slots
      if (arduboy.justPressed(UP_BUTTON)) {
        if (selectedWeaponSlot > 0) selectedWeaponSlot--;
        else selectedWeaponSlot = 2;  // Wrap around
      }
      if (arduboy.justPressed(DOWN_BUTTON)) {
        if (selectedWeaponSlot < 2) selectedWeaponSlot++;
        else selectedWeaponSlot = 0;  // Wrap around
      }

      // Handle selection
      if (arduboy.justPressed(A_BUTTON)) {
        if (justEnteredWeaponMenu) {
          // Ignore the A_BUTTON press that just entered the weapon menu
          justEnteredWeaponMenu = false;  // Reset the flag
        } else {
          WeaponType currentType = player.playerWeapons[selectedWeaponSlot];
          if (currentType == WeaponType::None) {
            // Slot is empty, view available weapons for purchase
            viewingAvailableWeapons = true;
            selectedAvailableWeapon = 0;
          } else {
            // Slot has a weapon, initiate selling
            sellingWeapon = true;
          }
        }
      }
    } else if (viewingAvailableWeapons) {
      // Existing code for viewing available weapons
      displayAvailableWeapons(selectedAvailableWeapon);

      // Navigate through available weapons
      if (arduboy.justPressed(UP_BUTTON)) {
        if (selectedAvailableWeapon > 0) selectedAvailableWeapon--;
        else selectedAvailableWeapon = NUM_AVAILABLE_WEAPONS - 1;  // Wrap around
      }
      if (arduboy.justPressed(DOWN_BUTTON)) {
        if (selectedAvailableWeapon < NUM_AVAILABLE_WEAPONS - 1) selectedAvailableWeapon++;
        else selectedAvailableWeapon = 0;  // Wrap around
      }

      // Handle purchase
      if (arduboy.justPressed(A_BUTTON)) {
        if (selectedAvailableWeapon < NUM_AVAILABLE_WEAPONS) {
          Weapon selectedWeapon = getAvailableWeapon(selectedAvailableWeapon);
          if (player.money >= selectedWeapon.cost) {
            player.money -= selectedWeapon.cost;
            player.playerWeapons[selectedWeaponSlot] = selectedWeapon.type;
            viewingAvailableWeapons = false;
          } else {
            // Not enough money message
            notEnoughMoneyMessage = true;
          }
        }
      }

      // Handle cancellation
      if (arduboy.justPressed(B_BUTTON)) {
        viewingAvailableWeapons = false;
      }
    } else if (sellingWeapon) {
      // Existing code for selling weapons
      displaySellConfirmation(selectedWeaponSlot);

      // Handle selling
      if (arduboy.justPressed(A_BUTTON)) {
        WeaponType weaponToSell = player.playerWeapons[selectedWeaponSlot];
        if (weaponToSell != WeaponType::None) {
          uint8_t sellPrice = getWeaponCost(weaponToSell) / 2;  // Example: Sell for half the cost
          player.money += sellPrice;
          player.playerWeapons[selectedWeaponSlot] = WeaponType::None;
        }
        sellingWeapon = false;
      }

      // Handle cancellation
      if (arduboy.justPressed(B_BUTTON)) {
        sellingWeapon = false;
      }
    }

    // Display "Not Enough Money" message if needed
    if (notEnoughMoneyMessage) {
      displayNotEnoughMoney();
      if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
        notEnoughMoneyMessage = false;
      }
    }
  }
  // Handle adjusting stats if applicable (unchanged from existing code)
  // Note: This section is already handled above within the adjustingStat check
}

SQ7x8 wrapCoordinate(SQ7x8 coordinate, SQ7x8 max) {
  if (coordinate < 0) {
    return max + coordinate;
  } else if (coordinate >= max) {
    return coordinate - max;
  }
  return coordinate;
}

struct DrawParameters {
  uint8_t x = 0;
  uint8_t y = 0;
  SQ7x8 distance = 0;
};

bool to3DView(Object object, DrawParameters& result);

bool to3DView(Object object, DrawParameters& result) {
  constexpr uint8_t screenCenterX = WIDTH / 2;
  constexpr uint8_t horizon = 10;
  constexpr uint8_t depth = 120;

  constexpr SQ7x8 minDistance = 1;
  constexpr SQ7x8 maxDistance = 32;
  constexpr SQ7x8 maxAngleRatio = 1;

  SQ7x8 worldX = object.y;
  SQ7x8 worldY = object.x;

  SQ7x8 relX = worldX - player.y;
  SQ7x8 relY = worldY - player.x;

  // Handle wrapping around world edges
  while (relX < -worldWidth / 2) relX += worldWidth;
  while (relX >= worldWidth / 2) relX -= worldWidth;
  while (relY < -worldHeight / 2) relY += worldHeight;
  while (relY >= worldHeight / 2) relY -= worldHeight;

  SQ7x8 rotatedX = relX * Cos(player.viewAngle) + relY * Sin(player.viewAngle);
  SQ7x8 rotatedY = relY * Cos(player.viewAngle) - relX * Sin(player.viewAngle);

  // Approximate distance calculation
  SQ7x8 distance = rotatedY;

  if ((distance <= minDistance) || (distance > maxDistance))
    // Indicate that the object should not be drawn
    return false;

  if (abs(rotatedX / rotatedY) > maxAngleRatio)
    // Indicate that the object should not be drawn
    return false;

  uint8_t screenX = screenCenterX + (uint8_t)(rotatedX / rotatedY * depth);
  uint8_t screenY = horizon + (uint8_t)(depth / rotatedY);

  // If the object is in bounds
  if ((screenX >= 0) && (screenX < WIDTH) && (screenY >= 0) && (screenY < HEIGHT)) {
    // Set up the result parameters
    result.x = screenX;
    result.y = screenY;
    result.distance = distance;

    // Indicate that the drawing should go ahead
    return true;
  }

  // Otherwise, indicate that the object should not be drawn
  return false;
}

void drawGridCell(uint8_t x, uint8_t y) {
  arduboy.drawPixel(x, y, WHITE);
}

uint8_t calculateSize(SQ7x8 distance, uint8_t minSize, uint8_t maxSize, SQ7x8 minDistance, SQ7x8 maxDistance) {
  SQ7x8 scaleFactor = (distance - minDistance) / (maxDistance - minDistance);
  scaleFactor = 1 - scaleFactor;

  if (scaleFactor < 0) scaleFactor = 0;
  if (scaleFactor > 1) scaleFactor = 1;

  return minSize + (uint8_t)((maxSize - minSize) * scaleFactor);
}

int8_t currentEnemyIndex = -1;  // Global index for the currently rendered enemy

struct SpriteInfo {
  uint24_t sprite;
  uint8_t width;
  uint8_t height;
  int8_t maxFrames;  // Use -1 for no frames
};

// Sprite data stored in PROGMEM
const SpriteInfo mothraSprites[] PROGMEM = {
  { enemy2x3, enemy2x3Width, enemy2x3Height, 0 },
  { mech6x8, mech6x8Width, mech6x8Height, 0 },
  { Mothra8x11, Mothra8x11Width, Mothra8x11Height, Mothra8x11Frames - 1 },
  { Mothra16x21, Mothra16x21Width, Mothra16x21Height, Mothra16x21Frames - 1 },
  { Mothra30x40, Mothra30x40Width, Mothra30x40Height, Mothra30x40Frames - 1 },
  { Mothra50x67, Mothra50x67Width, Mothra50x67Height, Mothra50x67Frames - 1 },
  { Mothra70x94, Mothra70x94Width, Mothra70x94Height, 0 },
  { Mothra90x121, Mothra90x121Width, Mothra90x121Height, 0 },
  { Mothra110x148, Mothra110x148Width, Mothra110x148Height, 0 },
};

const SpriteInfo battleCatSprites[] PROGMEM = {
  { enemy2x3, enemy2x3Width, enemy2x3Height, 0 },
  { mech6x8, mech6x8Width, mech6x8Height, 0 },
  { Battle_Cat8x11, Battle_Cat8x11_width, Battle_Cat8x11_height, 0 },
  { Battle_Cat16x21, Battle_Cat16x21_width, Battle_Cat16x21_height, 0 },
  { Battle_Cat30x40, Battle_Cat30x40_width, Battle_Cat30x40_height, Battle_Cat30x40_frames - 1 },
  { Battle_Cat50x67, Battle_Cat50x67_width, Battle_Cat50x67_height, Battle_Cat50x67_frames - 1 },
  { Battle_Cat70x94, Battle_Cat70x94_width, Battle_Cat70x94_height, Battle_Cat70x94_frames - 1 },
  { Battle_Cat90x121, Battle_Cat90x121_width, Battle_Cat90x121_height, 0 },
  { Battle_Cat110x148, Battle_Cat110x148_width, Battle_Cat110x148_height, 0 },
};

const SpriteInfo thorHammerSprites[] PROGMEM = {
  { enemy2x3, enemy2x3Width, enemy2x3Height, 0 },
  { mech6x8, mech6x8Width, mech6x8Height, 0 },
  { Thor_Hammer8x11, Thor_Hammer8x11_width, Thor_Hammer8x11_height, 0 },
  { Thor_Hammer16x21, Thor_Hammer16x21_width, Thor_Hammer16x21_height, 0 },
  { Thor_Hammer30x40, Thor_Hammer30x40_width, Thor_Hammer30x40_height, Thor_Hammer30x40_frames - 1 },
  { Thor_Hammer50x67, Thor_Hammer50x67_width, Thor_Hammer50x67_height, Thor_Hammer50x67_frames - 1 },
  { Thor_Hammer70x94, Thor_Hammer70x94_width, Thor_Hammer70x94_height, Thor_Hammer70x94_frames - 1 },
  { Thor_Hammer90x121, Thor_Hammer90x121_width, Thor_Hammer90x121_height, 0 },
  { Thor_Hammer110x148, Thor_Hammer110x148_width, Thor_Hammer110x148_height, 0 },
};

// Size thresholds stored in PROGMEM
const uint8_t sizeThresholds[] PROGMEM = { 8, 15, 20, 30, 40, 60, 80, 110 };

SpriteInfo getEnemySprite(MechType mechType, uint8_t size);

SpriteInfo getEnemySprite(MechType mechType, uint8_t size) {
  const SpriteInfo* spriteArray = nullptr;
  uint8_t spriteCount = 0;

  if (mechType == MechType::Mothra) {
    spriteArray = mothraSprites;
    spriteCount = sizeof(mothraSprites) / sizeof(SpriteInfo);
  } else if (mechType == MechType::Battle_Cat) {
    spriteArray = battleCatSprites;
    spriteCount = sizeof(battleCatSprites) / sizeof(SpriteInfo);
  } else if (mechType == MechType::Thor_Hammer) {
    spriteArray = thorHammerSprites;
    spriteCount = sizeof(thorHammerSprites) / sizeof(SpriteInfo);
  }

  // Find the appropriate sprite based on size
  for (uint8_t i = 0; i < spriteCount - 1; ++i) {
    uint8_t threshold = pgm_read_byte(&sizeThresholds[i]);
    if (size < threshold) {
      SpriteInfo spriteInfo;
      memcpy_P(&spriteInfo, &spriteArray[i], sizeof(SpriteInfo));
      return spriteInfo;
    }
  }

  // Return the last sprite if size exceeds all thresholds
  /*SpriteInfo spriteInfo;
    memcpy_P(&spriteInfo, &spriteArray[spriteCount - 1], sizeof(SpriteInfo));
    return spriteInfo;*/
}

void drawMech(uint8_t x, uint8_t y) {
  SpriteInfo spriteInfo = getEnemySprite(enemies[currentEnemyIndex].mechType, enemies[currentEnemyIndex].scaledSize);

  enemies[currentEnemyIndex].maxFrames = spriteInfo.maxFrames;

  // Frame progression
  if (enemies[currentEnemyIndex].maxFrames <= 0) {
    enemies[currentEnemyIndex].currentFrame = 0;
  } else if (arduboy.everyXFrames(3)) {
    if (enemies[currentEnemyIndex].currentFrame >= enemies[currentEnemyIndex].maxFrames) {
      enemies[currentEnemyIndex].currentFrame = 0;
    } else {
      ++enemies[currentEnemyIndex].currentFrame;
    }
  }

  // Flash white if hit recently
  if (enemies[currentEnemyIndex].hitFlashTimer > 0) {
    // Reduce flash timer
    enemies[currentEnemyIndex].hitFlashTimer--;

    // Draw the enemy sprite in white to simulate a flash
    arduboy.fillRect(x - spriteInfo.width / 2, y - spriteInfo.height / 2, spriteInfo.width, spriteInfo.height, WHITE);
  } else {
    // Draw the normal enemy sprite
    FX::drawBitmap(x - spriteInfo.width / 2, y - spriteInfo.height / 2, spriteInfo.sprite, enemies[currentEnemyIndex].currentFrame, dbmMasked);
  }
}


void updateExplosions() {
  // Update the explosion frames for all exploding enemies
  for (uint8_t i = 0; i < currentMission.numEnemies; ++i) {
    if (enemies[i].state == EnemyState::Exploding) {
      // Frame progression
      if (arduboy.everyXFrames(8)) {
        if (enemies[i].currentFrame >= enemies[i].maxFrames) {
          // Decrement active enemies and mark the enemy as inactive when the explosion finishes
          --currentMission.activeEnemies;
          enemies[i].state = EnemyState::Inactive;
          enemies[i].currentFrame = 0;  // Reset frame for future use
          enemies[i].maxFrames = 0;     // Reset maxFrames
        } else {
          ++enemies[i].currentFrame;
        }
      }
    }
  }
}

// Explosion sprite data stored in PROGMEM
const SpriteInfo explosionSprites[] PROGMEM = {
  { explosion23x23, 23, 23, explosion23x23Frames - 1 },
  { explosion46x46, 46, 46, explosion46x46Frames - 1 },
  { explosion92x92, 92, 92, explosion92x92Frames - 1 },
  { explosion110x110, 110, 110, explosion110x110Frames - 1 },
};

// Explosion size thresholds stored in PROGMEM
const uint8_t explosionSizeThresholds[] PROGMEM = { 23, 46, 92, 110 };

SpriteInfo getExplosionSprite(uint8_t size);

SpriteInfo getExplosionSprite(uint8_t size) {
  const uint8_t spriteCount = sizeof(explosionSprites) / sizeof(SpriteInfo);

  // Find the appropriate sprite based on size
  for (uint8_t i = 0; i < spriteCount - 1; ++i) {
    uint8_t threshold = pgm_read_byte(&explosionSizeThresholds[i]);
    if (size < threshold) {
      SpriteInfo spriteInfo;
      memcpy_P(&spriteInfo, &explosionSprites[i], sizeof(SpriteInfo));
      return spriteInfo;
    }
  }

  // Return the last sprite if size exceeds all thresholds
  SpriteInfo spriteInfo;
  memcpy_P(&spriteInfo, &explosionSprites[spriteCount - 1], sizeof(SpriteInfo));
  return spriteInfo;
}

void drawExplosion(uint8_t x, uint8_t y) {
  SpriteInfo spriteInfo = getExplosionSprite(enemies[currentEnemyIndex].scaledSize);

  // Render the explosion sprite at the given position
  FX::drawBitmap(
    x - spriteInfo.width / 2,
    y - spriteInfo.height / 2,
    spriteInfo.sprite,
    enemies[currentEnemyIndex].currentFrame,
    dbmMasked);
}

void animateAndRenderEnemies() {
  const uint8_t minEnemySize = 2;
  const uint8_t maxEnemySize = 120;
  const SQ7x8 minEnemyDistance = 0.5;
  const SQ7x8 maxEnemyDistance = 16;

  for (uint8_t i = 0; i < currentMission.numEnemies; ++i) {
    if (enemies[i].state != EnemyState::Inactive) {
      currentEnemyIndex = i;
      DrawParameters drawParameters;

      if (to3DView(enemies[currentEnemyIndex], drawParameters)) {
        if (currentEnemyIndex < maxEnemies) {
          enemies[currentEnemyIndex].scaledSize = calculateSize(
            drawParameters.distance, minEnemySize, maxEnemySize, minEnemyDistance, maxEnemyDistance);
        }

        if (enemies[currentEnemyIndex].state == EnemyState::Active) {
          drawMech(drawParameters.x, drawParameters.y);
        } else if (enemies[currentEnemyIndex].state == EnemyState::Exploding) {
          drawExplosion(drawParameters.x, drawParameters.y);
        }
      }
    }
  }
}

void drawMinimap() {
  arduboy.fillRect(0, 0, gridX * cellSize, gridY * cellSize, BLACK);
  for (uint8_t i = 0; i < gridX; i++) {
    for (uint8_t j = 0; j < gridY; j++) {
      arduboy.drawPixel(i * cellSize, j * cellSize, WHITE);
    }
  }

  // Draw the player as a triangle
  constexpr uint8_t triangleSize = 2;
  constexpr uint8_t triangleAngleOffset = 96;  // In binary radians

  // Calculate and draw the player triangle
  int8_t frontX = static_cast<int8_t >(player.x + Cos(player.angle) * triangleSize);
  int8_t frontY = static_cast<int8_t >(player.y + Sin(player.angle) * triangleSize);
  int8_t rearLeftX = static_cast<int8_t >(player.x + Cos(player.angle - triangleAngleOffset) * triangleSize);
  int8_t rearLeftY = static_cast<int8_t >(player.y + Sin(player.angle - triangleAngleOffset) * triangleSize);
  int8_t rearRightX = static_cast<int8_t >(player.x + Cos(player.angle + triangleAngleOffset) * triangleSize);
  int8_t rearRightY = static_cast<int8_t >(player.y + Sin(player.angle + triangleAngleOffset) * triangleSize);

  arduboy.fillTriangle(frontX, frontY, rearLeftX, rearLeftY, rearRightX, rearRightY, WHITE);
}

void drawEnemiesOnMinimap() {
  for (uint8_t i = 0; i < currentMission.numEnemies; ++i) {
    if (enemies[i].state == EnemyState::Active) {
      arduboy.drawRect((uint8_t)(enemies[i].x - 1), (uint8_t)(enemies[i].y - 1), 3, 3, WHITE);
    }
  }
}

void drawHealthBar() {
  // Draw player health bar
  uint8_t barX = 119;       // X position of the health bar
  uint8_t barY = 8;         // Y position of the health bar
  uint8_t barWidth = 8;     // Width of the health bar
  uint8_t barHeight = 48;   // Height of the health bar
  uint8_t maxHealth = 100;  // Maximum health value

  // Calculate the height of the filled portion of the health bar
  uint8_t fillHeight = (barHeight - 1) * player.health / maxHealth;

  // Draw the outline of the health bar
  arduboy.drawRect(barX, barY, barWidth, barHeight, WHITE);

  // Draw the filled portion of the health bar (fill from bottom up)
  if (fillHeight > 0) {
    arduboy.fillRect(
      barX + 1,
      barY + barHeight - 1 - fillHeight,
      barWidth - 2,
      fillHeight,
      WHITE);
  }
}

void drawHeatBar() {
  // Draw player heat bar
  uint8_t heatBarX = 110;                             // X position of the heat bar
  uint8_t heatBarY = 8;                               // Y position of the heat bar
  uint8_t heatBarWidth = 8;                           // Width of the heat bar
  uint8_t heatBarHeight = 48;                         // Height of the heat bar
  uint8_t maxHeat = player.playerMechStats.heatSink;  // Maximum heat capacity

  // Calculate the height of the filled portion of the heat bar
  uint8_t heatFillHeight = (heatBarHeight - 2) * player.heat / maxHeat;

  // Ensure heatFillHeight does not exceed the drawable area
  if (heatFillHeight > (heatBarHeight - 2)) {
    heatFillHeight = heatBarHeight - 2;
  }

  // Draw the outline of the heat bar
  arduboy.drawRect(heatBarX, heatBarY, heatBarWidth, heatBarHeight, WHITE);

  // Draw the filled portion of the heat bar (fill from bottom up)
  if (heatFillHeight > 0) {
    arduboy.fillRect(
      heatBarX + 1,
      heatBarY + heatBarHeight - 1 - heatFillHeight,
      heatBarWidth - 2,
      heatFillHeight,
      WHITE);
  }
}

void drawHUD() {
  // Draw player cockpit type based on mech type
  if (player.mechType == MechType::Mothra) {
    FX::drawBitmap(0, 0, mothraCockpit128x64, 0, dbmMasked);
  } else if (player.mechType == MechType::Battle_Cat) {
    FX::drawBitmap(0, 0, battleCatCockpit128x64, 0, dbmMasked);
  } else if (player.mechType == MechType::Thor_Hammer) {
    FX::drawBitmap(0, 0, thorHammerCockpit128x64, 0, dbmMasked);
  }

  if (player.mechStatus == MechStatus::Overheat) {
    arduboy.setCursor(40, 30);  // Adjust positioning as needed
    arduboy.print(F("Overheat"));
  }

  if (minimapEnabled) {
    drawMinimap();
    drawEnemiesOnMinimap();
  }

  drawHealthBar();
  drawHeatBar();
}

const SQ7x8 bulletSpeed = 0.2;
const uint8_t maxBulletLifetime = 50;  // Adjusted for bulletSpeed
uint8_t bulletActiveFlags = 0;         // Each bit represents a bullet's active state

struct Bullet : public Object {
  uint8_t angle;     // 1 byte
  uint8_t lifetime;  // 1 byte
  // Position inherited from Object (x and y as SQ7x8)
};

const uint8_t maxBullets = 5;  // Max number of bullets
Bullet bullets[maxBullets];    // Bullet array
const uint8_t maxBulletDistance = 10;

void fireBullet() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (!(bulletActiveFlags & (1 << i))) {
      bulletActiveFlags |= (1 << i);
      bullets[i].x = player.x;
      bullets[i].y = player.y;
      bullets[i].angle = player.angle;
      bullets[i].lifetime = 0;
      break;
    }
  }
}

void updateBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bulletActiveFlags & (1 << i)) {
      bullets[i].x += bulletSpeed * Cos(bullets[i].angle);
      bullets[i].y += bulletSpeed * Sin(bullets[i].angle);
      bullets[i].x = wrapCoordinate(bullets[i].x, worldWidth);
      bullets[i].y = wrapCoordinate(bullets[i].y, worldHeight);

      bullets[i].lifetime++;
      if (bullets[i].lifetime >= maxBulletLifetime) {
        bulletActiveFlags &= ~(1 << i);
        bullets[i].lifetime = 0;
      }
    }
  }
}

void renderBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bulletActiveFlags & (1 << i)) {
      DrawParameters drawParameters;
      if (to3DView(bullets[i], drawParameters))
        drawBullet(drawParameters.x, drawParameters.y, drawParameters.distance, i);
    }
  }
}

void drawBullet(uint8_t x, uint8_t y, SQ7x8 distance, uint8_t bulletIndex) {
  const uint8_t minBulletSize = 1;
  const uint8_t maxBulletSize = 3;
  const SQ7x8 minBulletDistance = 0.5;
  const SQ7x8 maxBulletDistance = 16;

  uint8_t scaledSize = calculateSize(distance, minBulletSize, maxBulletSize, minBulletDistance, maxBulletDistance);
  arduboy.drawCircle(x, y, scaledSize, WHITE);
}

void checkBulletEnemyCollisions() {
  const uint8_t minBulletSize = 1;
  const uint8_t maxBulletSize = 3;
  const SQ7x8 minBulletDistance = 0.5;
  const SQ7x8 maxBulletDistance = 16;

  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bulletActiveFlags & (1 << i)) {
      for (uint8_t j = 0; j < maxEnemies; ++j) {
        if (enemies[j].state != EnemyState::Inactive) {
          SQ7x8 dx = bullets[i].x - enemies[j].x;
          SQ7x8 dy = bullets[i].y - enemies[j].y;
          SQ15x16 distanceSquared = SQ15x16(dx) * dx + SQ15x16(dy) * dy;

          // Calculate scaled size on-the-fly
          SQ7x8 bulletDistance = bullets[i].lifetime * bulletSpeed;
          uint8_t bulletScaledSize = calculateSize(bulletDistance, minBulletSize, maxBulletSize, minBulletDistance, maxBulletDistance);

          SQ7x8 collisionDistance = (enemies[j].scaledSize / 24) + bulletScaledSize;
          SQ15x16 collisionThreshold = SQ15x16(collisionDistance) * collisionDistance;

          if (distanceSquared <= collisionThreshold) {
            bulletActiveFlags &= ~(1 << i);

            // Retrieve the damage from the currently equipped weapon
            WeaponType currentWeaponType = player.playerWeapons[player.currentWeaponSlot];
            uint8_t damage = getWeaponDamage(currentWeaponType);

            hitEnemy(j, damage);
            break;
          }
        }
      }
    }
  }
}

// Constants and variables for rockets
const SQ7x8 rocketSpeed = 0.15;
const uint8_t maxRocketLifetime = 80;
const uint8_t maxRockets = 3;       // Max number of rockets
uint8_t rocketActiveFlags = 0;      // Each bit represents a rocket's active state
const uint8_t rocketTurnSpeed = 4;  // Adjust as needed
const uint8_t rocketFireDelay = 6;  // Frames to wait between firing rockets

// Cooldown timer for firing rockets
uint8_t rocketFireCooldown = 0;

struct Rocket : public Object {
  uint8_t angle;        // 1 byte
  uint8_t lifetime;     // 1 byte
  uint8_t targetIndex;  // 1 byte: Index of the targeted enemy (255 if no target)
  // Position inherited from Object (x and y as SQ7x8)
};

Rocket rockets[maxRockets];  // Rocket array

// Helper function for coordinate wrapping difference
SQ7x8 wrapDistance(SQ7x8 a, SQ7x8 b, SQ7x8 maxValue) {
  SQ7x8 diff = a - b;
  if (diff > maxValue / 2) {
    diff -= maxValue;
  } else if (diff < -maxValue / 2) {
    diff += maxValue;
  }
  return diff;
}

// Function to check if an enemy is targeted by any active rocket
bool isEnemyTargeted(uint8_t enemyIndex) {
  for (uint8_t i = 0; i < maxRockets; ++i) {
    if ((rocketActiveFlags & (1 << i)) && rockets[i].targetIndex == enemyIndex) {
      return true;
    }
  }
  return false;
}

// Function to draw a rectangle around a targeted enemy
void drawTargetedEnemyRectangle(uint8_t enemyIndex) {
  // Ensure the enemy is active
  if (enemies[enemyIndex].state == EnemyState::Inactive) {
    return;
  }

  DrawParameters enemyDrawParams;
  if (!to3DView(enemies[enemyIndex], enemyDrawParams)) {
    return;  // Enemy is not in the player's field of view
  }

  uint8_t screenX = enemyDrawParams.x;
  uint8_t screenY = enemyDrawParams.y;
  SQ7x8 distance = enemyDrawParams.distance;

  // Calculate the size of the rectangle based on the enemy's scaledSize
  uint8_t rectSize = enemies[enemyIndex].scaledSize;

  // Define the rectangle's top-left and bottom-right coordinates
  int8_t halfSize = rectSize / 2;
  int8_t rectX1 = screenX - halfSize;
  int8_t rectY1 = screenY - halfSize;
  int8_t rectX2 = screenX + halfSize;
  int8_t rectY2 = screenY + halfSize;

  // Draw the rectangle using Arduboy's drawRect function
  // Ensure coordinates are within screen bounds (0 to 127 for x, 0 to 63 for y)
  rectX1 = constrain(rectX1, 0, WIDTH - 1);
  rectY1 = constrain(rectY1, 0, HEIGHT - 1);
  rectX2 = constrain(rectX2, 0, WIDTH - 1);
  rectY2 = constrain(rectY2, 0, HEIGHT - 1);

  arduboy.drawRect(rectX1, rectY1, rectX2 - rectX1, rectY2 - rectY1, WHITE);
}

// Function to fire a rocket only if an enemy is in view
void fireRocket() {
  if (rocketFireCooldown == 0) {
    // Find the nearest enemy in the player's field of view
    uint8_t nearestEnemyIndex = 255;                   // Invalid index
    SQ15x16 nearestDistanceSquared = SQ15x16(999999);  // Arbitrarily large value

    for (uint8_t j = 0; j < maxEnemies; ++j) {
      if (enemies[j].state != EnemyState::Inactive) {
        DrawParameters enemyDrawParams;
        if (to3DView(enemies[j], enemyDrawParams)) {
          // Calculate distance from player to enemy
          SQ7x8 dx = wrapDistance(enemies[j].x, player.x, worldWidth);
          SQ7x8 dy = wrapDistance(enemies[j].y, player.y, worldHeight);
          SQ15x16 distanceSquared = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

          if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            nearestEnemyIndex = j;
          }
        }
      }
    }

    if (nearestEnemyIndex != 255) {
      // Fire a rocket targeting the nearest enemy in view
      for (uint8_t i = 0; i < maxRockets; ++i) {
        if (!(rocketActiveFlags & (1 << i))) {
          rocketActiveFlags |= (1 << i);
          rockets[i].x = player.x;
          rockets[i].y = player.y;
          rockets[i].angle = player.angle;
          rockets[i].lifetime = 0;
          rockets[i].targetIndex = nearestEnemyIndex;  // Assign target
          rocketFireCooldown = rocketFireDelay;        // Set the cooldown
          break;
        }
      }
    }
  }
}

// Update rockets with homing behavior and cooldown timer
void updateRockets() {
  // Decrease the rocket fire cooldown timer
  if (rocketFireCooldown > 0) {
    rocketFireCooldown--;
  }

  for (uint8_t i = 0; i < maxRockets; ++i) {
    if (rocketActiveFlags & (1 << i)) {
      uint8_t target = rockets[i].targetIndex;

      if (target != 255 && enemies[target].state != EnemyState::Inactive) {
        DrawParameters enemyDrawParams;
        if (to3DView(enemies[target], enemyDrawParams)) {
          // Enemy is still in player's field of view

          // Calculate difference in position
          SQ7x8 dx = wrapDistance(enemies[target].x, rockets[i].x, worldWidth);
          SQ7x8 dy = wrapDistance(enemies[target].y, rockets[i].y, worldHeight);
          SQ15x16 distanceSquared = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

          // Adjust rocket's angle toward the enemy using the cross product method
          SQ7x8 cosTheta = Cos(rockets[i].angle);
          SQ7x8 sinTheta = Sin(rockets[i].angle);

          int16_t cross = (int16_t)(cosTheta * dy - sinTheta * dx);

          if (cross > 0) {
            rockets[i].angle += rocketTurnSpeed;
          } else if (cross < 0) {
            rockets[i].angle -= rocketTurnSpeed;
          }
          // No adjustment needed if cross == 0
        } else {
          // Target is no longer in view; deactivate rocket
          rocketActiveFlags &= ~(1 << i);
          rockets[i].lifetime = 0;
          continue;
        }
      } else {
        // Target is invalid; deactivate rocket
        rocketActiveFlags &= ~(1 << i);
        rockets[i].lifetime = 0;
        continue;
      }

      // Move the rocket forward
      rockets[i].x += rocketSpeed * Cos(rockets[i].angle);
      rockets[i].y += rocketSpeed * Sin(rockets[i].angle);

      // Wrap coordinates
      rockets[i].x = wrapCoordinate(rockets[i].x, worldWidth);
      rockets[i].y = wrapCoordinate(rockets[i].y, worldHeight);

      // Update lifetime and check expiration
      rockets[i].lifetime++;
      if (rockets[i].lifetime >= maxRocketLifetime) {
        rocketActiveFlags &= ~(1 << i);
        rockets[i].lifetime = 0;
      }
    }
  }
}

// Render rockets
void renderRockets() {
  for (uint8_t i = 0; i < maxRockets; ++i) {
    if (rocketActiveFlags & (1 << i)) {
      DrawParameters drawParameters;
      if (to3DView(rockets[i], drawParameters))
        drawRocket(drawParameters.x, drawParameters.y, drawParameters.distance, i);
    }
  }
}

// Draw a single rocket
void drawRocket(uint8_t x, uint8_t y, SQ7x8 distance, uint8_t rocketIndex) {
  const uint8_t minRocketSize = 1;
  const uint8_t maxRocketSize = 6;
  const SQ7x8 minRocketDistance = 0.5;
  const SQ7x8 maxRocketDistance = 24;

  uint8_t scaledSize = calculateSize(distance, minRocketSize, maxRocketSize, minRocketDistance, maxRocketDistance);
  arduboy.fillCircle(x, y, scaledSize, WHITE);
  //arduboy.drawTriangle();
}

// Function to render rectangles around the nearest enemy
void renderTargetRectangle() {
  uint8_t nearestEnemyIndex = 255;                   // Initialize with an invalid index
  SQ15x16 nearestDistanceSquared = SQ15x16(999999);  // Start with a large distance

  // Iterate through all enemies to find the nearest active one in view
  for (uint8_t j = 0; j < maxEnemies; ++j) {
    if (enemies[j].state != EnemyState::Inactive) {
      DrawParameters enemyDrawParams;
      if (to3DView(enemies[j], enemyDrawParams)) {
        // Calculate distance from player to enemy
        SQ7x8 dx = wrapDistance(enemies[j].x, player.x, worldWidth);
        SQ7x8 dy = wrapDistance(enemies[j].y, player.y, worldHeight);
        SQ15x16 distanceSquared = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

        // Update nearest enemy if this one is closer
        if (distanceSquared < nearestDistanceSquared) {
          nearestDistanceSquared = distanceSquared;
          nearestEnemyIndex = j;
        }
      }
    }
  }

  // If a nearest enemy is found, draw a rectangle around it
  if (nearestEnemyIndex != 255) {
    drawTargetedEnemyRectangle(nearestEnemyIndex);
  }
}

void checkRocketEnemyCollisions() {
  for (uint8_t i = 0; i < maxRockets; ++i) {
    if (rocketActiveFlags & (1 << i)) {
      uint8_t target = rockets[i].targetIndex;
      if (target != 255 && enemies[target].state != EnemyState::Inactive) {
        SQ7x8 dx = wrapDistance(rockets[i].x, enemies[target].x, worldWidth);
        SQ7x8 dy = wrapDistance(rockets[i].y, enemies[target].y, worldHeight);
        SQ15x16 distanceSquared = SQ15x16(dx) * dx + SQ15x16(dy) * dy;

        // Calculate scaled size on-the-fly
        SQ7x8 rocketDistance = rockets[i].lifetime * rocketSpeed;
        uint8_t rocketScaledSize = calculateSize(rocketDistance, 1, 6, 0.5, 24);

        SQ7x8 collisionDistance = (enemies[target].scaledSize / 24) + rocketScaledSize;
        SQ15x16 collisionThreshold = SQ15x16(collisionDistance) * collisionDistance;

        if (distanceSquared <= collisionThreshold) {
          rocketActiveFlags &= ~(1 << i);

          // Retrieve the damage from the currently equipped weapon
          WeaponType currentWeaponType = player.playerWeapons[player.currentWeaponSlot];
          uint8_t damage = getWeaponDamage(currentWeaponType);

          hitEnemy(target, damage);
        }
      }
    }
  }
}

const SQ7x8 laserRange = 16;     // Maximum range of the laser
const uint8_t laserWidth = 1;    // Width of the laser beam

void updateLaser() {
  // Laser direction vector
  SQ7x8 dx = Cos(player.angle);
  SQ7x8 dy = Sin(player.angle);

  SQ15x16 laserWidthSquared = SQ15x16(laserWidth) * laserWidth;  // For distance comparison

  // Retrieve the damage from the currently equipped weapon
  WeaponType currentWeaponType = player.playerWeapons[player.currentWeaponSlot];
  uint8_t damage = getWeaponDamage(currentWeaponType);

  for (uint8_t i = 0; i < maxEnemies; ++i) {
    if (enemies[i].state != EnemyState::Inactive) {
      // Vector from player to enemy with wrapping
      SQ7x8 ex_px = wrapDistance(enemies[i].x, player.x, worldWidth);
      SQ7x8 ey_py = wrapDistance(enemies[i].y, player.y, worldHeight);

      // Project enemy position onto laser direction
      SQ15x16 dot = SQ15x16(ex_px) * dx + SQ15x16(ey_py) * dy;

      if (dot < 0 || dot > laserRange) {
        // Enemy is behind the player or beyond laser range
        continue;
      }

      // Closest point on laser line to enemy in relative coordinates
      SQ15x16 closestX = dot * dx;
      SQ15x16 closestY = dot * dy;

      // Compute squared distance from enemy to closest point on laser line
      SQ15x16 distanceSquared = (SQ15x16(ex_px) - closestX) * (SQ15x16(ex_px) - closestX)
                              + (SQ15x16(ey_py) - closestY) * (SQ15x16(ey_py) - closestY);

      if (distanceSquared <= laserWidthSquared) {
        // Enemy is hit by laser
        hitEnemy(i, damage);
      }
    }
  }
}

void renderLaser() {
  // Calculate laser end point in world coordinates
  SQ7x8 laserEndX = player.x + laserRange * Cos(player.angle);
  SQ7x8 laserEndY = player.y + laserRange * Sin(player.angle);

  laserEndX = wrapCoordinate(laserEndX, worldWidth);
  laserEndY = wrapCoordinate(laserEndY, worldHeight);

  // Create an Object for the laser end point
  Object laserEnd;
  laserEnd.x = laserEndX;
  laserEnd.y = laserEndY;

  DrawParameters endParams;

  if (to3DView(laserEnd, endParams)) {
    // The laser starts at the weapon position on the screen
    uint8_t weaponScreenX = WIDTH / 3;
    uint8_t weaponScreenY = HEIGHT - 10;  // Adjust based on your HUD

    // Draw the laser line from the weapon to the laser end point
    arduboy.drawLine(weaponScreenX, weaponScreenY, endParams.x, endParams.y, WHITE);
  } else {
    // If the laser end point is not within view, draw it to the maximum range
    uint8_t weaponScreenX = WIDTH / 2;
    uint8_t weaponScreenY = HEIGHT - 10;  // Adjust based on your HUD

    // Project a point at maximum distance in the current direction
    uint8_t maxX = weaponScreenX;
    uint8_t maxY = 0;  // Top of the screen

    arduboy.drawLine(weaponScreenX, weaponScreenY, maxX, maxY, WHITE);
  }
}

// Maybe change to function that returns bool (so logic can be continued outside of function)
// and separate enemy health decrement and state change to be handled elsewhere?
// So there can also be visual confirmation of hits by inverting colors in rendering logic?
void hitEnemy(uint8_t enemyIndex, uint8_t damage) {
  if (enemies[enemyIndex].health <= 0 && enemies[enemyIndex].state != EnemyState::Exploding) {
    // Transition to the "Exploding" state
    enemies[enemyIndex].state = EnemyState::Exploding;
    enemies[enemyIndex].currentFrame = 0;
    SpriteInfo spriteInfo = getExplosionSprite(enemies[enemyIndex].scaledSize);
    enemies[enemyIndex].maxFrames = spriteInfo.maxFrames;
  } else {
    // Reduce health by damage but ensure health does not wrap around
    if (enemies[enemyIndex].health > damage) {
      enemies[enemyIndex].health -= damage;
    } else {
      enemies[enemyIndex].health = 0;
    }

    // Set hit flash state and timer (adjust flash time as needed)
    enemies[enemyIndex].hitFlashTimer = 1;  // e.g., flash for 1 frame
  }
}

// Constants
const uint8_t BASE_MOVEMENT_HEAT = 80;  // Base heat increase for movement (replace with individual mech stats)
//const uint8_t BASE_SHOOTING_HEAT = 80;  // Base heat increase for shooting (phased out in favor of individual weapon increase)

// Player action flags
bool playerIsMoving = false;
bool playerIsShooting = false;

// Overheat management
uint16_t overheatTimer = 0;

// Update heat based on player actions
// Update heat based on player actions
void updateHeat() {
  if (player.mechStatus != MechStatus::Overheat) {
    // Calculate heat increase adjusted by heat sink value
    uint8_t movementHeatIncrease = calculateHeatIncrease(BASE_MOVEMENT_HEAT, player.playerMechStats.heatSink);

    // Retrieve the current weapon's heat usage
    WeaponType currentWeaponType = player.playerWeapons[player.currentWeaponSlot];
    uint8_t shootingHeatUsage = getWeaponHeat(currentWeaponType);
    uint8_t shootingHeatIncrease = calculateHeatIncrease(shootingHeatUsage, player.playerMechStats.heatSink);

    // Movement heat accumulation
    uint8_t movementHeatThreshold = 2 * (player.playerMechStats.heatSink / 3);  // 60% threshold

    if (playerIsMoving && player.heat < movementHeatThreshold) {
      player.heat += movementHeatIncrease;
      if (player.heat > movementHeatThreshold) {
        player.heat = movementHeatThreshold;  // Clamp to threshold
      }
    }

    // Shooting heat accumulation
    if (playerIsShooting) {
      player.heat += shootingHeatIncrease;
    }

    // Continuous heat dissipation
    dissipateHeat();

    // Overheat check
    if (player.heat >= player.playerMechStats.heatSink) {
      player.heat = player.playerMechStats.heatSink;
      player.mechStatus = MechStatus::Overheat;
      overheatTimer = calculateOverheatDuration(player.playerMechStats.heatSink);
    }
  } else {
    // Overheat state management
    if (overheatTimer > 0) {
      overheatTimer--;
    } else {
      player.mechStatus = MechStatus::Normal;
      player.heat = 0;
    }
  }
}

// Calculate heat increase adjusted by heat sink value
uint8_t calculateHeatIncrease(uint8_t baseHeat, uint8_t heatSinkValue) {
  // Ensure heatSinkValue is not zero to avoid division by zero
  if (heatSinkValue == 0) heatSinkValue = 1;

  // Higher heat sink value reduces heat accumulation
  uint8_t heatIncrease = (baseHeat * 10) / heatSinkValue;

  // Ensure heat increase is at least 1
  if (heatIncrease == 0) heatIncrease = 1;

  return heatIncrease;
}

// Calculate heat dissipation rate based on heat sink value
uint8_t calculateDissipationRate(uint8_t heatSinkValue) {
  if (heatSinkValue == 0) return 0;
  return heatSinkValue / 10;  // Adjust as needed
}

// Heat dissipation function
void dissipateHeat() {
  uint8_t dissipationRate = calculateDissipationRate(player.playerMechStats.heatSink);

  if (playerIsMoving || playerIsShooting) {
    dissipationRate /= 2;
    if (dissipationRate == 0) dissipationRate = 1;  // Ensure at least minimal dissipation
  }

  if (player.heat >= dissipationRate) {
    player.heat -= dissipationRate;
  } else {
    player.heat = 0;
  }
}

// Calculate overheat duration based on heat sink value
uint16_t calculateOverheatDuration(uint8_t heatSinkValue) {
  if (heatSinkValue == 0) heatSinkValue = 1;
  return 1000 / (heatSinkValue / 10);  // Adjust as needed
}

bool laserActive = false;  // Indicates whether the laser is currently firing

void handleInput() {
  // Block player actions if in overheat state
  if (player.mechStatus == MechStatus::Overheat) {
    laserActive = false;  // Ensure laser is deactivated during overheat
    return;  // Skip input handling during overheat
  }

  // Reset movement and shooting flags at the start
  playerIsMoving = false;
  playerIsShooting = false;

  // Toggle minimap with A + B pressed
  if (arduboy.justPressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
    minimapEnabled = !minimapEnabled;
  }

  // Handle weapon cycling with A_BUTTON
  if (arduboy.justPressed(A_BUTTON)) {
    uint8_t newWeaponSlot = findNextWeaponSlot(player.currentWeaponSlot);
    // Only change if a different slot is found
    if (newWeaponSlot != player.currentWeaponSlot) {
      player.currentWeaponSlot = newWeaponSlot;
      laserActive = false;  // Deactivate laser when switching weapons
      // Optionally, add feedback (sound, vibration, etc.)
    }
  }

  // Handle firing with B_BUTTON (only if A_BUTTON is not pressed)
  if (arduboy.pressed(B_BUTTON) && !arduboy.pressed(A_BUTTON)) {
    WeaponType currentWeapon = player.playerWeapons[player.currentWeaponSlot];
    switch (currentWeapon) {
      case WeaponType::Bullets:
      case WeaponType::HeavyBullets:
        fireBullet();
        playerIsShooting = true;
        break;
      case WeaponType::Rockets:
      case WeaponType::MediumRockets:
        fireRocket();
        playerIsShooting = true;
        break;
      case WeaponType::Laser:
        laserActive = true;  // Start firing the laser
        playerIsShooting = true;
        break;
      default:
        // No weapon equipped
        break;
    }
  } else {
    laserActive = false;  // Deactivate laser when B_BUTTON is released
  }

  // Handle rotation and movement as usual
  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= player.playerMechStats.rotationSpeed;      // Rotate left
    player.viewAngle += player.playerMechStats.rotationSpeed;  // Rotate right
    playerIsMoving = true;
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += player.playerMechStats.rotationSpeed;      // Rotate right
    player.viewAngle -= player.playerMechStats.rotationSpeed;  // Rotate left
    playerIsMoving = true;
  }

  // Handle movement with UP and DOWN buttons
  if (arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) {
    int8_t direction = (arduboy.pressed(UP_BUTTON) ? 1 : -1);

    player.x += direction * player.playerMechStats.moveSpeed * Cos(player.angle);
    player.y += direction * player.playerMechStats.moveSpeed * Sin(player.angle);

    player.x = wrapCoordinate(player.x, worldWidth);
    player.y = wrapCoordinate(player.y, worldHeight);
    playerIsMoving = true;
  }
}


/*void handleInput() {
  if (arduboy.justPressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
    minimapEnabled = !minimapEnabled;
  }
  if (arduboy.pressed(B_BUTTON)) {
    //if (arduboy.justPressed(B_BUTTON)) {
    fireBullet();
  }
  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= player.playerMechStats.rotationSpeed;      // Rotate left
    player.viewAngle += player.playerMechStats.rotationSpeed;  // Rotate right
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += player.playerMechStats.rotationSpeed;      // Rotate right
    player.viewAngle -= player.playerMechStats.rotationSpeed;  // Rotate left
  }
  if (arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) {
    int8_t direction = (arduboy.pressed(UP_BUTTON) ? 1.0f : -1.0f);

    player.x += direction * player.playerMechStats.moveSpeed * Cos(player.angle);
    player.y += direction * player.playerMechStats.moveSpeed * Sin(player.angle);

    player.x = wrapCoordinate(player.x, worldWidth);
    player.y = wrapCoordinate(player.y, worldHeight);
  }
}*/

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();

  FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);  // // initialise FX chip
  FX::loadGameState(saveData);
  //FX::setFont(font_4x6, dcmNormal); // select default font
  //FX::setCursorRange(0,80);         // set cursor left and wrap positions
  // Initialize grid cells
  for (uint8_t x = 0; x < gridX; ++x) {
    for (uint8_t y = 0; y < gridY; ++y) {
      gridCells[x * gridY + y].x = x * cellSize;
      gridCells[x * gridY + y].y = y * cellSize;
    }
  }

  // Initialize enemies with example positions
  /*for (uint8_t i = 0; i < maxEnemies; ++i) {
    enemies[i].state = EnemyState::Active;
    enemies[i].x = random(worldWidth);   // Random X position
    enemies[i].y = random(worldHeight);  // Random Y position
  }*/
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.clear();
  arduboy.pollButtons();

  // Change this switch case to if statements instead of nesting switch cases
  switch (currentState) {
    case GameState::Title_Menu:
      updateTitleMenu();
      // Display title menu
      break;
    case GameState::Init_New_Game:
      initNewGame();
      //updateInitialiseGameState();
      currentState = GameState::Main_Menu;
      break;
    case GameState::Save_Game:
      initSaveGame();
      updateSaveGame();
      break;
    case GameState::Init_Load_Game:
      initLoadGame();
      break;
    case GameState::No_Save_Screen:
      updateNoSaveScreen();
      break;
    case GameState::Main_Menu:
      updateMainMenu();
      break;
    case GameState::Save_Load_Submenu:
      previousState = GameState::Main_Menu;
      updateSaveLoadMenu();
      break;
    case GameState::Mission_Submenu:
      previousState = GameState::Main_Menu;
      updateMissionMenu();
      break;
    case GameState::Hanger_Submenu:
      previousState = GameState::Main_Menu;
      updateHangerMenu();
      break;
    case GameState::Customization_Submenu:
      previousState = GameState::Hanger_Submenu;
      updateCustomizationMenu();
      break;
    case GameState::Game_Play:
      handleInput();
      // Render each grid cell
      if (currentMission.activeEnemies == 0) {
        completeMission();
      }
      for (uint8_t i = 0; i < gridX * gridY; ++i) {
        // Prepare an empty DrawParameters object
        DrawParameters drawParameters;

        // Determine whether the object should be drawn
        if (to3DView(gridCells[i], drawParameters))
          // If the object is onscreen, draw it
          drawGridCell(drawParameters.x, drawParameters.y);
      }

      updateEnemies();
      updateExplosions();
      animateAndRenderEnemies();

      if (player.playerWeapons[player.currentWeaponSlot] == WeaponType::Bullets || player.playerWeapons[player.currentWeaponSlot] == WeaponType::HeavyBullets) {
        // Bullet handling:
        updateBullets();               // Update bullet positions
        checkBulletEnemyCollisions();  // Check for bullet-enemy collisions
        renderBullets();               // Render bullets
      } else if (player.playerWeapons[player.currentWeaponSlot] == WeaponType::Rockets || player.playerWeapons[player.currentWeaponSlot] == WeaponType::MediumRockets) {
        // Rocket handling:
        renderTargetRectangle();
        updateRockets();
        checkRocketEnemyCollisions();
        renderRockets();
      } else if (player.playerWeapons[player.currentWeaponSlot] == WeaponType::Laser) {
        if (laserActive) {
          updateLaser();  // Check for laser-enemy collisions
          renderLaser();  // Render the laser beam
        }
      }

      updateHeat();
      drawHUD();

      if (player.health <= 0) {
        // Make sure screen is set back to normal (from taking damage)
        FX::enableOLED();
        arduboy.invert(false);
        FX::disableOLED();
        currentState = GameState::Game_Over;
      }
      break;
    case GameState::Game_Over:

      arduboy.setCursor(40, 30);  // Adjust positioning as needed
      arduboy.print(F("Game Over"));
      if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
        // Reset player day count before going back to title menu
        player.dayCount = 0;
        currentState = GameState::Title_Menu;
      }
      break;
  }

  FX::display(CLEAR_BUFFER);
}