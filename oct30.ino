/*// TODO: 
- Invert explosion sprites color (so they look better)
- Draw enemy sprite masks
- Finish implementing player ammo (add reload fee in repair menu?)
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
  //Leg_Damage,
  //Arm_Damage
};

enum WeaponType : uint8_t {
  None = 0,
  Bullets = 1,
  HeavyBullets = 2,
  Rockets = 3,
  MediumRockets = 4,
  Laser = 5
};

// Define the weapon attributes in PROGMEM as a flat array
const uint8_t availableWeapons[] PROGMEM = {
  // Type,      Damage, Cost, Weight, Heat, MaxAmmo
  WeaponType::Bullets,          1, 100,  80, 30, 200, // Light MG
  WeaponType::HeavyBullets,     2, 150, 90, 40, 200, // Heavy MG
  WeaponType::Rockets,          4, 200, 100, 50,  60, // Light Rockets
  WeaponType::MediumRockets,    6, 250, 110, 60,  60, // Medium Rockets
  WeaponType::Laser,            2, 225, 90, 100,   0  // Laser
  // Add more weapons as needed
};

constexpr uint8_t WEAPON_ATTR_COUNT = 6; // Number of attributes per weapon
constexpr uint8_t NUM_AVAILABLE_WEAPONS = 5;

struct Weapon {
  WeaponType type;
  uint8_t damage;
  uint8_t cost;
  uint8_t weight;
  uint8_t heat;
  uint8_t maxAmmo;
};

// Function to retrieve Weapon from availableWeapons array safely
Weapon getAvailableWeapon(uint8_t index) {
  Weapon temp = { WeaponType::None, 0, 0, 0, 0, 0 };
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Calculate the correct starting address for the weapon
    uint16_t weaponAddress = index * WEAPON_ATTR_COUNT;
    memcpy_P(&temp, &availableWeapons[weaponAddress], sizeof(Weapon));
  }
  return temp;
}

// Function to retrieve the damage of a given WeaponType
inline uint8_t getWeaponDamage(WeaponType type) {
  if (type == WeaponType::None) return 0;
  
  uint8_t index = type - 1; // Adjust since None = 0
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Calculate the address for damage
    uint16_t address = index * WEAPON_ATTR_COUNT + 1;
    return pgm_read_byte(&availableWeapons[address]);
  }
  
  return 0;
}

inline uint8_t getWeaponHeat(WeaponType type) {
  if (type == WeaponType::None) return 0;
  
  uint8_t index = type - 1;
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Calculate the address for heat
    uint16_t address = index * WEAPON_ATTR_COUNT + 4;
    return pgm_read_byte(&availableWeapons[address]);
  }
  
  return 0;
}

inline uint8_t getWeaponCost(WeaponType type) {
  if (type == WeaponType::None) return 0;
  
  uint8_t index = type - 1;
  if (index < NUM_AVAILABLE_WEAPONS) {
    // Calculate the address for cost
    uint16_t address = index * WEAPON_ATTR_COUNT + 2;
    return pgm_read_byte(&availableWeapons[address]);
  }
  
  return 0;
}

struct Mech {
  bool isActive = false;  // Indicates if the mech is owned
  MechType type;
  MechStatus status;
  uint8_t health;
  uint8_t mechStats[6];   // Specific stats for the mech
  WeaponType weapons[3];  // Weapons equipped on the mech
  uint8_t currentAmmo[3]; // Track ammo for each weapon in RAM
  SQ7x8 moveSpeed;
  // Add other mech-specific attributes here
};

struct Player : Object {
  uint8_t dayCount = 0;
  uint24_t money = 0;
  Mech mechs[3];                  // Up to 3 mechs
  uint8_t currentMech = 0;        // Index of the currently active mech
  uint8_t currentWeaponSlot = 0;  // Active weapon slot for the current mech
  uint8_t heat = 0;
  uint8_t angle = 0;
  uint8_t viewAngle = 0;
} player;

void initializeAmmo(Mech &mech);
// Function to initialize ammo based on equipped weapons for a specific mech
void initializeAmmo(Mech &mech) {
  for (uint8_t slot = 0; slot < 3; ++slot) {
    WeaponType type = mech.weapons[slot];
    
    if (type == WeaponType::None) {
      mech.currentAmmo[slot] = 0;
    } else {
      uint8_t index = type - 1; // Adjust since None = 0
      if (index < NUM_AVAILABLE_WEAPONS) {
        // Calculate the address for maxAmmo
        uint16_t address = index * WEAPON_ATTR_COUNT + 5;
        mech.currentAmmo[slot] = pgm_read_byte(&availableWeapons[address]);
      } else {
        mech.currentAmmo[slot] = 0;
      }
    }
  }
}

void initializeMech(Mech &mech);

void initializeMech(Mech &mech) {
  mech.health = 100;
  mech.status = MechStatus::Normal;
  
  switch (mech.type) {
    case MechType::Mothra:
      mech.moveSpeed = 0.3;
      FX::readDataBytes(playerMothraStats, mech.mechStats, 6);
      mech.weapons[0] = WeaponType::Bullets;
      mech.weapons[1] = WeaponType::Laser;
      mech.weapons[2] = WeaponType::None;
      break;
    case MechType::Battle_Cat:
      mech.moveSpeed = 0.2;
      FX::readDataBytes(playerBattleCatStats, mech.mechStats, 6);
      mech.weapons[0] = WeaponType::Rockets;
      mech.weapons[1] = WeaponType::Rockets;
      mech.weapons[2] = WeaponType::Bullets;
      break;
    case MechType::Thor_Hammer:
      mech.moveSpeed = 0.1;
      FX::readDataBytes(playerThorHammerStats, mech.mechStats, 6);
      mech.weapons[0] = WeaponType::HeavyBullets;
      mech.weapons[1] = WeaponType::MediumRockets;
      mech.weapons[2] = WeaponType::Laser;
      break;
    // Add initialization for other mech types if needed
  }
  
  initializeAmmo(mech); // Initialize ammo for this specific mech
}


#define MAX_ENEMIES 3
#define APPROACH_DISTANCE_SQ (12 * 12)    // 144
#define CIRCLE_DISTANCE_SQ (4 * 4)        // 16
#define FLASH_DURATION 5

enum EnemyState : uint8_t {
  Active,
  Inactive,
  Exploding
};

struct Enemy : public Object {
  EnemyState state = EnemyState::Inactive;
  uint8_t scaledSize;
  SQ7x8 moveSpeed;
  uint16_t health;
  uint8_t damage;
  MechType mechType;
  uint8_t currentFrame;
  uint8_t maxFrames;
  uint8_t hitFlashTimer;
  bool circlingAssigned;
  bool circlingDirection;
  uint8_t wanderCounter;
  uint8_t wanderDirection;
};

Enemy enemies[MAX_ENEMIES];
uint8_t flashTimer;


void updateEnemies() {
  // Handle flash timer for player damage indication
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

  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    Enemy &enemy = enemies[i];

    if (enemy.state != Active) continue;

    // Compute the wrapped distance between the enemy and the player
    SQ7x8 dx = wrapDistance(player.x, enemy.x, worldWidth);
    SQ7x8 dy = wrapDistance(player.y, enemy.y, worldHeight);
    SQ15x16 distSq = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

    if (distSq <= CIRCLE_DISTANCE_SQ) {
      // **Close Distance** - Circling behavior

      // Attack chance (10% chance)
      if (random(10) == 0) {
        Mech &currentMech = player.mechs[player.currentMech];
        if (currentMech.health) {
          flashTimer = FLASH_DURATION;
          // Simplified health deduction
          currentMech.health = currentMech.health > enemy.damage ? currentMech.health - enemy.damage : 0;
        }
      }

      // Assign circling direction if not assigned
      if (!enemy.circlingAssigned) {
        enemy.circlingAssigned = true;
        enemy.circlingDirection = random(2);
      }

      // Move perpendicular to the player without division
      if ((dx >= 0 ? dx : -dx) > (dy >= 0 ? dy : -dy)) {
        enemy.y += enemy.circlingDirection ? enemy.moveSpeed : -enemy.moveSpeed;
      } else {
        enemy.x += enemy.circlingDirection ? -enemy.moveSpeed : enemy.moveSpeed;
      }
    } else if (distSq <= APPROACH_DISTANCE_SQ) {
      // **Medium Distance** - Approach behavior

      // Clear circling assigned flag
      enemy.circlingAssigned = false;

      // Calculate movement steps without overshooting and eliminate abs()
      SQ7x8 stepX = dx > enemy.moveSpeed ? enemy.moveSpeed : (dx < -enemy.moveSpeed ? -enemy.moveSpeed : dx);
      SQ7x8 stepY = dy > enemy.moveSpeed ? enemy.moveSpeed : (dy < -enemy.moveSpeed ? -enemy.moveSpeed : dy);

      // Apply movement steps
      enemy.x += stepX;
      enemy.y += stepY;
    } else {
      // **Far Distance** - Wandering behavior

      if (enemy.wanderCounter == 0) {
        enemy.wanderDirection = random(4);
        enemy.wanderCounter = random(5, 16);
      }
      enemy.wanderCounter--;

      // Direction arrays to replace switch-case
      static const int8_t dxs[] = { 0, 1, 0, -1 };
      static const int8_t dys[] = { -1, 0, 1, 0 };
      enemy.x += dxs[enemy.wanderDirection] * enemy.moveSpeed;
      enemy.y += dys[enemy.wanderDirection] * enemy.moveSpeed;
    }

    // Apply world coordinate wrapping
    enemy.x = wrapCoordinate(enemy.x, worldWidth);
    enemy.y = wrapCoordinate(enemy.y, worldHeight);
  }
}

// Define maximum constants
constexpr uint8_t MAX_MISSIONS = 3;       // Maximum number of missions available at any time
//constexpr uint8_t MAX_ENEMIES = 3;        // Assuming a max of 3 mechs per mission

// Struct to represent a mission
struct Mission {
  uint8_t numEnemies;
  MechType mechs[MAX_ENEMIES];
  uint16_t reward;
};

// Declare mission arrays
Mission availableMissions[MAX_MISSIONS];
Mission currentMission;

// Function prototype
Mission generateMission();

// Function to generate a mission
Mission generateMission() {
  Mission mission;
  uint8_t totalDifficulty = 0;
  uint8_t day = player.dayCount;

  // Determine mission parameters based on the current day
  if (day < 10) {
    mission.numEnemies = random(1, 3);  // 1 to 2 light mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
      totalDifficulty += 1;  // Difficulty value for Mothra
    }
  }
  else if (day < 20) {  // 10 to 19
    mission.numEnemies = random(2, 4);  // 2 to 3 light mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
      totalDifficulty += 1;
    }
  }
  else if (day < 30) {  // 20 to 29
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
  }
  else if (day < 40) {  // 30 to 39
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
  else {  // 40 and above
    mission.numEnemies = random(1, 4);  // 1 to 3 heavy mechs
    for (uint8_t i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Thor_Hammer;
      totalDifficulty += 3;
    }
  }

  // Set mission reward based on totalDifficulty
  mission.reward = (uint16_t)totalDifficulty * 1000;  // 1000 credits per difficulty point

  return mission;
}

// Function to populate the list of available missions
void populateMissionList() {
  for (uint8_t i = 0; i < MAX_MISSIONS; i++) {
    availableMissions[i] = generateMission();
  }
}


void initNewGame() {
  player.dayCount = 1;
  player.money = 50000;
  player.currentMech = 0;

  // Initialize the first mech
  player.mechs[0].isActive = true;
  player.mechs[0].type = MechType::Mothra;  // Starting mech
  initializeMech(player.mechs[0]);

  // Set other mechs as inactive
  player.mechs[1].isActive = false;
  player.mechs[2].isActive = false;

  // Other initialization code
  populateMissionList();  // Populate the list of available missions
}

struct SaveData {
  uint8_t dayCount = 0;
  uint24_t money = 0;
  Mission savedMissions[MAX_MISSIONS];  // Add mission list to save data
  Mech mechs[3];
};

SaveData saveData;

void initSaveGame() {
  saveData.dayCount = player.dayCount;
  saveData.money = player.money;

  for (uint8_t i = 0; i < 3; i++) {
    saveData.mechs[i] = player.mechs[i];
  }
  for (uint8_t i = 0; i < MAX_MISSIONS; i++) {
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


  for (uint8_t i = 0; i < 3; i++) {
    saveData.mechs[i] = player.mechs[i];
  }
  for (uint8_t i = 0; i < MAX_MISSIONS; i++) {
    availableMissions[i] = saveData.savedMissions[i];
  }
}

void updateNoSaveScreen() {
  FX::drawBitmap(5, 5, noSaveDataStr, 0, dbmNormal);
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

// Menu structure
struct Menu {
  uint24_t itemsAddress;     // Address in FX memory where menu items are stored
  uint8_t itemCount;         // Total number of items in the menu
  uint8_t currentSelection;  // Index of the currently selected item
};

Menu mainMenu = { mainMenuItems, 3, 0 };
Menu saveLoadMenu = { saveLoadMenuItems, 2, 0 };
Menu missionMenu = { missionMenuItems, 3, 0 };
Menu hangerMenu = { hangerMenuItems, 6, 0 };
Menu customizationMenu = { customizationMenuItems, 4, 0 };

void updateMenu(Menu &menu);

void updateMenu(Menu &menu) {
  // Maximum number of items across all menus
  const uint8_t maxItems = 6;  // Adjust if you have more items in any menu

  // Buffer to store menu item data: x, y, width, height
  uint8_t itemData[maxItems][4];

  // Read the item data from FX memory into RAM
  FX::readDataBytes(menu.itemsAddress, (uint8_t *)itemData, menu.itemCount * 4);

  // Draw the highlight rectangle around the currently selected item
  uint8_t *currentItem = itemData[menu.currentSelection];
  arduboy.drawRect(currentItem[0], currentItem[1], currentItem[2], currentItem[3], WHITE);

  // Handle directional input to update currentSelection
  const uint8_t buttons[] = { UP_BUTTON, DOWN_BUTTON, LEFT_BUTTON, RIGHT_BUTTON };
  const int8_t dirX[] = { 0, 0, -1, 1 };
  const int8_t dirY[] = { -1, 1, 0, 0 };

  for (uint8_t dir = 0; dir < 4; ++dir) {
    if (arduboy.justPressed(buttons[dir])) {
      int8_t bestIndex = -1;
      int16_t minDistanceSq = INT16_MAX;
      int16_t currentX = currentItem[0];
      int16_t currentY = currentItem[1];

      for (uint8_t i = 0; i < menu.itemCount; ++i) {
        if (i == menu.currentSelection) continue;
        uint8_t *item = itemData[i];
        int16_t dx = item[0] - currentX;
        int16_t dy = item[1] - currentY;

        // Only consider items in the desired direction
        if (dirX[dir] != 0 && dirX[dir] * dx <= 0) continue;
        if (dirY[dir] != 0 && dirY[dir] * dy <= 0) continue;

        int16_t distanceSq = dx * dx + dy * dy;

        if (distanceSq < minDistanceSq) {
          minDistanceSq = distanceSq;
          bestIndex = i;
        }
      }
      if (bestIndex != -1) menu.currentSelection = bestIndex;
      break;  // Only process one direction per update
    }
  }
}

// Your updateMainMenu function remains largely the same
void updateMainMenu() {
  // Explicitly set invert to false just to make sure
  FX::enableOLED();
  arduboy.invert(false);
  FX::disableOLED();

  FX::drawBitmap(0, 0, mainMenu128x64, 0, dbmMasked);

  font4x6.setCursor(30, 53);  // Adjust position as needed
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

void initializeMission(const Mission &mission);

void initializeMission(const Mission &mission) {
  player.x = random(worldWidth);   // Random player X position
  player.y = random(worldHeight);  // Random player Y position
  player.heat = 0;                 // Reset player heat to 0
  player.mechs[player.currentMech].status = MechStatus::Normal;

  currentMission.numEnemies = mission.numEnemies;

  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
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
      const uint8_t maxArmor = player.mechs[player.currentMech].mechStats[4];
      uint8_t playerArmor = player.mechs[player.currentMech].mechStats[3];
      enemies[i].damage = (uint8_t)((baseDamage * (maxArmor - playerArmor + 1)) / maxArmor);

      // Ensure that damage is at least 1
      if (enemies[i].damage < 1) {
        enemies[i].damage = 1;
      }
    }
  }
}

void completeMission() {
  // Make sure screen is set back to normal
  FX::enableOLED();
  arduboy.invert(false);
  FX::disableOLED();

  // Example logic for completing a mission
  player.mechs[player.currentMech].status = MechStatus::Normal;
  player.money += currentMission.reward;  // Award money to player (needs to be more randomized and populated when mission is generated)
  populateMissionList();                  // Generate new set of missions
  currentState = GameState::Main_Menu;    // Return to main menu
  ++player.dayCount;
}

uint8_t selectedMissionIndex = 0;  // Index of the currently selected mission

void updateMissionMenu() {
  FX::drawBitmap(0, 0, missionMenu128x64, 0, dbmMasked);
  updateMenu(missionMenu);

  FX::drawBitmap(40, 5, enemiesStr, 0, dbmNormal);
  font4x6.setCursor(80, 4);
  font4x6.print(availableMissions[selectedMissionIndex].numEnemies);

  // For enemy types, print directly in a loop
  for (uint8_t i = 0; i < availableMissions[selectedMissionIndex].numEnemies; i++) {
    uint8_t yPos = 15 + (i * 10);  // Adjust 10 to match your bitmap height + desired padding
    // Instead of storing type names in a buffer, print directly
    switch (availableMissions[selectedMissionIndex].mechs[i]) {
      case MechType::Mothra:
        FX::drawBitmap(42, yPos, mothraStr, 0, dbmNormal);
        break;
      case MechType::Battle_Cat:
        FX::drawBitmap(42, yPos, battleCatStr, 0, dbmNormal);
        break;
      case MechType::Thor_Hammer:
        FX::drawBitmap(42, yPos, thorHammerStr, 0, dbmNormal);
        break;
    }
  }
  font4x6.setCursor(52, 48);
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
          selectedMissionIndex = MAX_MISSIONS - 1;
        }
        break;
      case 2:  // Right arrow
        if (selectedMissionIndex < MAX_MISSIONS - 1) {
          selectedMissionIndex++;
        } else {
          selectedMissionIndex = 0;
        }
        break;
    }
  }
}

void switchToNextMech() {
  for (uint8_t i = 1; i < 3; ++i) {
    uint8_t nextMechIndex = (player.currentMech + i) % 3;
    if (player.mechs[nextMechIndex].isActive) {
      player.currentMech = nextMechIndex;
      return;
    }
  }
  // No other mechs are active; stay on the current mech
}

void switchToPreviousMech() {
  for (int8_t i = 1; i < 3; ++i) {
    int8_t prevMechIndex = (player.currentMech - i + 3) % 3;
    if (player.mechs[prevMechIndex].isActive) {
      player.currentMech = prevMechIndex;
      return;
    }
  }
  // No other mechs are active; stay on the current mech
}

void switchToNextAvailableMech() {
  for (uint8_t i = 0; i < 3; ++i) {
    if (player.mechs[i].isActive) {
      player.currentMech = i;
      return;
    }
  }
  // No mechs are active; handle this case appropriately
}

struct MechInfo {
  MechType type;
  uint16_t cost;
};

uint16_t getMechCost(MechType type) {
  switch (type) {
    case MechType::Mothra:
      return 10000;
    case MechType::Battle_Cat:
      return 20000;
    case MechType::Thor_Hammer:
      return 30000;
    // Add additional cases for other mech types if needed
    default:
      return 0;
  }
}

uint16_t getMechSellPrice(MechType type) {
  return getMechCost(type) / 2;  // Selling price is half of the cost
}

void buyMech(MechType type) {
  uint16_t cost = getMechCost(type);
  if (player.money >= cost) {
    // Find an empty slot
    for (uint8_t i = 0; i < 3; ++i) {
      if (!player.mechs[i].isActive) {
        player.mechs[i].isActive = true;
        player.mechs[i].type = type;
        initializeMech(player.mechs[i]);
        player.money -= cost;
        // Optionally set the newly purchased mech as the current mech
        player.currentMech = i;
        return;
      }
    }
    // No empty slots available; notify the player
    // with message like "No empty mech slots"
  } else {
    // Not enough money; notify the player
    // with message like "Not enough money"
  }
}

void sellMech(uint8_t mechIndex) {
  if (player.mechs[mechIndex].isActive) {
    uint16_t sellPrice = getMechSellPrice(player.mechs[mechIndex].type);
    player.money += sellPrice;
    player.mechs[mechIndex].isActive = false;

    // If the sold mech was the current mech, switch to another active mech
    if (player.currentMech == mechIndex) {
      switchToNextAvailableMech();
    }
  } else {
    // Mech slot is already empty; handle accordingly
  }
}

// Function to display "Not Enough Money" message
void displayNotEnoughMoney() {
  FX::drawBitmap(9, 39, notEnoughMoney86x20, 0, dbmNormal);
}

// Global Variables
bool inRepairMenu = false;
bool notEnoughMoneyMessage = false;
uint16_t repairCost = 0;

// Struct for Mech Data
struct MechData {
  uint24_t bitmap;
  uint24_t nameStr;
};

// Array of Mech Data indexed by MechType
const MechData mechData[] = {
  {Mothra30x40, mothraStr},       // MechType::Mothra
  {Battle_Cat30x40, battleCatStr}, // MechType::Battle_Cat
  {Thor_Hammer30x40, thorHammerStr} // MechType::Thor_Hammer
};

// Function to draw the mech
void drawMech(MechType type) {
  const MechData& data = mechData[static_cast<uint8_t>(type)];
  FX::drawBitmap(4, 4, data.bitmap, 0, dbmMasked);
  font4x6.setCursor(42, 4);
  font4x6.print(player.mechs[player.currentMech].health);
  FX::drawBitmap(4, 48, data.nameStr, 0, dbmNormal);
}

void updateHangerMenu() {
  FX::drawBitmap(0, 0, hangerMenu128x64, 0, dbmMasked);

  // Draw the current mech
  drawMech(player.mechs[player.currentMech].type);

  if (!inRepairMenu && !notEnoughMoneyMessage) {
    updateMenu(hangerMenu);
  }

  if (arduboy.justPressed(B_BUTTON) || arduboy.justPressed(A_BUTTON)) {
    if (notEnoughMoneyMessage) {
      notEnoughMoneyMessage = false;
    } else if (inRepairMenu) {
      if (arduboy.justPressed(B_BUTTON)) {
        inRepairMenu = false;
      } else if (arduboy.justPressed(A_BUTTON)) {
        // Confirm repair
        if (player.money >= repairCost) {
          player.money -= repairCost;
          player.mechs[player.currentMech].health = 100;
          inRepairMenu = false;
        } else {
          notEnoughMoneyMessage = true;
        }
      }
    } else {
      if (arduboy.justPressed(B_BUTTON)) {
        currentState = previousState;
      } else if (arduboy.justPressed(A_BUTTON)) {
        switch (hangerMenu.currentSelection) {
          case 0:
            inRepairMenu = true;
            initializeAmmo(player.mechs[player.currentMech]);
            {
              uint16_t repairCostPerHealthPoint = 100;
              uint8_t missingHealth = 100 - player.mechs[player.currentMech].health;
              repairCost = missingHealth * repairCostPerHealthPoint;
            }
            break;
          case 1:
            currentState = GameState::Customization_Submenu;
            break;
          case 2:
            {
              MechType mechToBuy = MechType::Thor_Hammer;
              uint16_t cost = getMechCost(mechToBuy);
              if (player.money >= cost) {
                buyMech(mechToBuy);
              } else {
                mechToBuy = MechType::Battle_Cat;
                cost = getMechCost(mechToBuy);
                if (player.money >= cost) {
                  buyMech(mechToBuy);
                }
              }
            }
            break;
          case 3:
            sellMech(player.currentMech);
            break;
          case 4:
            switchToNextMech();
            break;
          case 5:
            switchToPreviousMech();
            break;
        }
      }
    }
  }

  if (inRepairMenu) {
    // Fixed box dimensions to save code size
    uint8_t x = 32;
    uint8_t y = 39;
    uint8_t boxWidth = 64;

    // Display repair cost
    arduboy.drawRect(x - 1, y - 1, boxWidth + 2, 22, WHITE);
    arduboy.fillRect(x, y, boxWidth, 20, BLACK);

    FX::drawBitmap(x + 2, y + 1, costStr, 0, dbmNormal);
    font4x6.setCursor(x + 30, y);
    font4x6.print(repairCost);

    // Prompt for confirmation
    FX::drawBitmap(x + 2, y + 11, pressAStr, 0, dbmNormal);
  }

  if (notEnoughMoneyMessage) {
    displayNotEnoughMoney();
  }
}

// Global Variables for Weapon Menu
bool inWeaponMenu = false;
bool viewingAvailableWeapons = false;
bool sellingWeapon = false;
uint8_t selectedWeaponSlot = 0;
uint8_t selectedAvailableWeapon = 0;
bool justEnteredWeaponMenu = false;
bool adjustingStat = false;

// Function to display weapon slots
void displayWeaponSlots(uint8_t selectedSlot) {
  arduboy.fillRect(8, 8, 70, 50, BLACK);
  arduboy.drawRect(7, 7, 70, 50, WHITE);
  FX::drawBitmap(10, 10, weaponSlotsStr, 0, dbmNormal);

  for (uint8_t i = 0; i < 3; i++) {
    uint8_t y = 20 + i * 12;

    // Highlight selected slot
    if (i == selectedSlot) {
      arduboy.drawRect(8, y - 1, 68, 10, WHITE);
    }

    // Retrieve weapon type
    WeaponType type = player.mechs[player.currentMech].weapons[i];
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
      case WeaponType::Laser:
        weaponName = laserStr;
        break;
    }
    FX::drawBitmap(12, y, weaponName, 0, dbmNormal);
  }
}

// Function to display available weapons for purchase
void displayAvailableWeapons(uint8_t selectedWeapon) {
  // Draw the sprite containing all the weapon names
  FX::drawBitmap(8, 1, availableWeaponsMenu90x60, 0, dbmMasked);

  // Highlight the selected weapon
  if (selectedWeapon < NUM_AVAILABLE_WEAPONS) {
    uint8_t y = 10 + selectedWeapon * 8;
    arduboy.drawRect(8, y, 60, 8, WHITE);
  }

  // Display selected weapon stats
  if (selectedWeapon < NUM_AVAILABLE_WEAPONS) {
    Weapon selected = getAvailableWeapon(selectedWeapon);
    uint8_t yStats = 15;

    FX::drawBitmap(58, yStats - 2, weaponsStatsMenu50x40, 0, dbmNormal);

    font4x6.setCursor(92, yStats - 1);
    font4x6.print(selected.damage);

    font4x6.setCursor(92, yStats + 6);
    font4x6.print(selected.heat);

    font4x6.setCursor(92, yStats + 13);
    font4x6.print(selected.weight);

    font4x6.setCursor(92, yStats + 20);
    font4x6.print(selected.maxAmmo);

    font4x6.setCursor(92, yStats + 27);
    font4x6.print(selected.cost);
  }
}

// Function to display sell confirmation
void displaySellConfirmation() {
  // Clear and draw the confirmation box
  arduboy.fillRect(10, 25, 83, 20, BLACK);
  arduboy.drawRect(10, 25, 83, 20, WHITE);

  FX::drawBitmap(12, 27, sellThisWeaponStr, 0, dbmNormal);
  FX::drawBitmap(12, 35, AyesBnoStr, 0, dbmNormal);
}

// Updated updateCustomizationMenu() Function
void updateCustomizationMenu() {
  FX::drawBitmap(0, 0, customizationMenu128x64, 0, dbmMasked);

  // Display stats
  const uint8_t yPositions[] = {0, 7, 14, 21, 34};
  const uint8_t statsIndices[] = {3, 0, 1}; // Armor, Weight, Heatsink
  for (uint8_t i = 0; i < 3; i++) {
    font4x6.setCursor(50, yPositions[i]);
    font4x6.print(player.mechs[player.currentMech].mechStats[statsIndices[i]]);
  }

  // Movespeed
  font4x6.setCursor(50, yPositions[3]);
  font4x6.print(static_cast<float>(player.mechs[player.currentMech].moveSpeed));

  // Credits
  font4x6.setCursor(50, yPositions[4]);
  font4x6.print(static_cast<long>(player.money));

  if (!adjustingStat && !inWeaponMenu) {
    updateMenu(customizationMenu);
  }

  if (arduboy.justPressed(B_BUTTON)) {
    if (adjustingStat) {
      adjustingStat = false;
    } else if (inWeaponMenu) {
      viewingAvailableWeapons = false;
      sellingWeapon = false;
      inWeaponMenu = false;
    } else {
      currentState = previousState;
    }
  }

  if (arduboy.justPressed(A_BUTTON)) {
    if (!inWeaponMenu && !adjustingStat) {
      switch (customizationMenu.currentSelection) {
        case 0:
        case 2:
          adjustingStat = true;
          break;
        case 1:
          inWeaponMenu = true;
          selectedWeaponSlot = 0;
          justEnteredWeaponMenu = true;
          break;
      }
    }
  }

  // Handle adjusting stats
  if (adjustingStat && !inWeaponMenu) {
    uint8_t statIndex = (customizationMenu.currentSelection == 0) ? 3 : 1;
    uint8_t maxStatIndex = statIndex + 1;
    uint8_t xPos = (customizationMenu.currentSelection == 0) ? 1 : 32;

    if (arduboy.justPressed(RIGHT_BUTTON)) {
      if (player.mechs[player.currentMech].mechStats[statIndex] < player.mechs[player.currentMech].mechStats[maxStatIndex] &&
          player.mechs[player.currentMech].moveSpeed > 0 &&
          player.money >= 100) {
        FX::drawBitmap(112, xPos, rightArrowSmall, 0, dbmMasked);
        player.mechs[player.currentMech].mechStats[statIndex]++;
        player.mechs[player.currentMech].mechStats[0]++;
        player.mechs[player.currentMech].moveSpeed -= 0.01;
        player.money -= 100;

        if (player.mechs[player.currentMech].moveSpeed < 0) {
          player.mechs[player.currentMech].moveSpeed = 0;
        }
      }
    }
    if (arduboy.justPressed(LEFT_BUTTON)) {
      if (player.mechs[player.currentMech].mechStats[statIndex] > 1 &&
          player.mechs[player.currentMech].mechStats[0] > 1) {
        FX::drawBitmap(72, xPos, leftArrowSmall, 0, dbmMasked);
        player.mechs[player.currentMech].mechStats[statIndex]--;
        player.mechs[player.currentMech].mechStats[0]--;
        player.mechs[player.currentMech].moveSpeed += 0.01;
        player.money += 100;
      }
    }
  }

  // Handle Weapon Menu
  if (inWeaponMenu) {
    if (!viewingAvailableWeapons && !sellingWeapon) {
      displayWeaponSlots(selectedWeaponSlot);

      // Navigate through weapon slots
      if (arduboy.justPressed(UP_BUTTON)) {
        selectedWeaponSlot = (selectedWeaponSlot > 0) ? selectedWeaponSlot - 1 : 2;
      }
      if (arduboy.justPressed(DOWN_BUTTON)) {
        selectedWeaponSlot = (selectedWeaponSlot < 2) ? selectedWeaponSlot + 1 : 0;
      }

      // Handle selection
      if (arduboy.justPressed(A_BUTTON)) {
        if (justEnteredWeaponMenu) {
          justEnteredWeaponMenu = false;
        } else {
          WeaponType currentType = player.mechs[player.currentMech].weapons[selectedWeaponSlot];
          if (currentType == WeaponType::None) {
            viewingAvailableWeapons = true;
            selectedAvailableWeapon = 0;
          } else {
            sellingWeapon = true;
          }
        }
      }
    } else if (viewingAvailableWeapons) {
      displayAvailableWeapons(selectedAvailableWeapon);

      // Navigate through available weapons
      if (arduboy.justPressed(UP_BUTTON)) {
        selectedAvailableWeapon = (selectedAvailableWeapon > 0) ? selectedAvailableWeapon - 1 : NUM_AVAILABLE_WEAPONS - 1;
      }
      if (arduboy.justPressed(DOWN_BUTTON)) {
        selectedAvailableWeapon = (selectedAvailableWeapon < NUM_AVAILABLE_WEAPONS - 1) ? selectedAvailableWeapon + 1 : 0;
      }

      // Handle purchase
      if (arduboy.justPressed(A_BUTTON)) {
        Weapon selectedWeapon = getAvailableWeapon(selectedAvailableWeapon);
        if (player.money >= selectedWeapon.cost) {
          player.money -= selectedWeapon.cost;
          player.mechs[player.currentMech].weapons[selectedWeaponSlot] = selectedWeapon.type;
          viewingAvailableWeapons = false;
        } else {
          notEnoughMoneyMessage = true;
        }
      }
    } else if (sellingWeapon) {
      displaySellConfirmation();

      // Handle selling
      if (arduboy.justPressed(A_BUTTON)) {
        WeaponType weaponToSell = player.mechs[player.currentMech].weapons[selectedWeaponSlot];
        if (weaponToSell != WeaponType::None) {
          uint8_t sellPrice = getWeaponCost(weaponToSell) / 2;
          player.money += sellPrice;
          player.mechs[player.currentMech].weapons[selectedWeaponSlot] = WeaponType::None;
        }
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

bool to3DView(Object object, DrawParameters &result);

bool to3DView(Object object, DrawParameters &result) {
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
  const SpriteInfo *spriteArray = nullptr;
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
  SpriteInfo spriteInfo;
  memcpy_P(&spriteInfo, &spriteArray[spriteCount - 1], sizeof(SpriteInfo));
  return spriteInfo;
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
          --currentMission.numEnemies;
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
        if (currentEnemyIndex < MAX_ENEMIES) {
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
  uint8_t fillHeight = (barHeight - 1) * player.mechs[player.currentMech].health / maxHealth;

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
  uint8_t heatBarX = 110;                                           // X position of the heat bar
  uint8_t heatBarY = 8;                                             // Y position of the heat bar
  uint8_t heatBarWidth = 8;                                         // Width of the heat bar
  uint8_t heatBarHeight = 48;                                       // Height of the heat bar
  uint8_t maxHeat = player.mechs[player.currentMech].mechStats[1];  // Maximum heat capacity

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
  if (player.mechs[player.currentMech].type == MechType::Mothra) {
    FX::drawBitmap(0, 0, mothraCockpit128x64, 0, dbmMasked);
  } else if (player.mechs[player.currentMech].type == MechType::Battle_Cat) {
    FX::drawBitmap(0, 0, battleCatCockpit128x64, 0, dbmMasked);
  } else if (player.mechs[player.currentMech].type == MechType::Thor_Hammer) {
    FX::drawBitmap(0, 0, thorHammerCockpit128x64, 0, dbmMasked);
  }

  if (player.mechs[player.currentMech].status == MechStatus::Overheat) {
    FX::drawBitmap((WIDTH / 2) - (overheatStrWidth / 2), HEIGHT / 2, overheatStr, 0, dbmNormal);
  }

  if (minimapEnabled) {
    drawMinimap();
    drawEnemiesOnMinimap();
  }

  drawHealthBar();
  drawHeatBar();

  switch(player.mechs[player.currentMech].type){
    case MechType::Mothra:
      font4x6.setCursor(60, 52);
      break;
    case MechType::Battle_Cat:
      font4x6.setCursor(60, 52);
      break;
    case MechType::Thor_Hammer:
      font4x6.setCursor(4, 52);
      break;
  } 

  font4x6.print(player.mechs[player.currentMech].currentAmmo[player.currentWeaponSlot]);
}

const SQ7x8 bulletSpeed = 0.2;
const uint8_t maxBulletLifetime = 50;
uint8_t bulletActiveFlags = 0;

struct Bullet : public Object {
  uint8_t angle;     // 1 byte
  uint8_t lifetime;  // 1 byte
};

const uint8_t maxBullets = 5;
Bullet bullets[maxBullets];
const uint8_t maxBulletDistance = 10;

// Helper to avoid repetitive bitwise operations
#define isBulletActive(i) (bulletActiveFlags & (1 << i))
#define activateBullet(i) (bulletActiveFlags |= (1 << i))
#define deactivateBullet(i) (bulletActiveFlags &= ~(1 << i))

bool fireBullet() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (!isBulletActive(i)) {
      activateBullet(i);
      bullets[i].x = player.x;
      bullets[i].y = player.y;
      bullets[i].angle = player.angle;
      bullets[i].lifetime = 0;
      return true;  // Bullet successfully fired
    }
  }
  return false;  // No available bullet slots
}


void updateBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (isBulletActive(i)) {
      SQ7x8 cosAngle = Cos(bullets[i].angle);  // Avoid recalculating for both x and y
      SQ7x8 sinAngle = Sin(bullets[i].angle);

      bullets[i].x += bulletSpeed * cosAngle;
      bullets[i].y += bulletSpeed * sinAngle;

      bullets[i].x = wrapCoordinate(bullets[i].x, worldWidth);
      bullets[i].y = wrapCoordinate(bullets[i].y, worldHeight);

      if (++bullets[i].lifetime >= maxBulletLifetime) {
        deactivateBullet(i);
      }
    }
  }
}

void renderBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (isBulletActive(i)) {
      DrawParameters drawParameters;
      if (to3DView(bullets[i], drawParameters))
        drawBullet(drawParameters.x, drawParameters.y, drawParameters.distance);
    }
  }
}

void drawBullet(uint8_t x, uint8_t y, SQ7x8 distance) {
  const uint8_t minBulletSize = 1;
  const uint8_t maxBulletSize = 3;
  const SQ7x8 minBulletDistance = 0.5;
  const SQ7x8 maxBulletDistance = 16;  // Change to 10?

  uint8_t scaledSize = calculateSize(distance, minBulletSize, maxBulletSize, minBulletDistance, maxBulletDistance);
  arduboy.drawCircle(x, y, scaledSize, WHITE);
}

void checkBulletEnemyCollisions() {
  const uint8_t minBulletSize = 1;
  const uint8_t maxBulletSize = 3;
  const SQ7x8 minBulletDistance = 0.5;
  const SQ7x8 maxBulletDistance = 16;  // Change to 10?

  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (isBulletActive(i)) {
      for (uint8_t j = 0; j < MAX_ENEMIES; ++j) {
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
            deactivateBullet(i);

            // Retrieve the damage from the currently equipped weapon
            WeaponType currentWeaponType = player.mechs[player.currentMech].weapons[player.currentWeaponSlot];
            uint8_t damage = getWeaponDamage(currentWeaponType);

            hitEnemy(j, damage);
            break;
          }
        }
      }
    }
  }
}

// Define constants using macros to save program memory
#define ROCKET_SPEED 0.15
#define MAX_ROCKET_LIFETIME 80
#define MAX_ROCKETS 3
#define ROCKET_TURN_SPEED 4
#define ROCKET_FIRE_DELAY 6
#define INACTIVE_TARGET 255

// Cooldown timer for firing rockets
uint8_t rocketFireCooldown = 0;

// Structure representing a rocket
struct Rocket : public Object {
  uint8_t angle;        // 1 byte
  uint8_t lifetime;     // 1 byte
  uint8_t targetIndex;  // 1 byte: 255 if inactive
  // Position inherited from Object (x and y as SQ7x8)
};

// Array to hold active rockets
Rocket rockets[MAX_ROCKETS];

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

// Inline function to check if an enemy is targeted by any active rocket
inline bool isEnemyTargeted(uint8_t enemyIndex) {
  for (uint8_t i = 0; i < MAX_ROCKETS; ++i) {
    if (rockets[i].targetIndex == enemyIndex) {
      return true;
    }
  }
  return false;
}

// Function to draw a rectangle around a targeted enemy
void drawTargetedEnemyRectangle(uint8_t enemyIndex) {
  if (enemies[enemyIndex].state == EnemyState::Inactive) return;

  DrawParameters enemyDrawParams;
  if (!to3DView(enemies[enemyIndex], enemyDrawParams)) return;

  uint8_t screenX = enemyDrawParams.x;
  uint8_t screenY = enemyDrawParams.y;
  uint8_t rectSize = enemies[enemyIndex].scaledSize;
  int8_t halfSize = rectSize / 2;

  // Calculate rectangle coordinates with constraints
  int8_t rectX1 = constrain(screenX - halfSize, 0, WIDTH - 1);
  int8_t rectY1 = constrain(screenY - halfSize, 0, HEIGHT - 1);
  int8_t rectX2 = constrain(screenX + halfSize, 0, WIDTH - 1);
  int8_t rectY2 = constrain(screenY + halfSize, 0, HEIGHT - 1);

  arduboy.drawRect(rectX1, rectY1, rectX2 - rectX1, rectY2 - rectY1, WHITE);
}

// Function to fire a rocket if cooldown allows and an enemy is in view
bool fireRocket() {
    if (rocketFireCooldown != 0) return false;  // Correctly return false when cooldown is active

    uint8_t nearestEnemy = INACTIVE_TARGET;
    SQ15x16 nearestDistSq = SQ15x16(999999);  // Arbitrary large value

    // Find the nearest active enemy in view
    for (uint8_t j = 0; j < MAX_ENEMIES; ++j) {
        if (enemies[j].state == EnemyState::Inactive) continue;

        DrawParameters drawParams;
        if (!to3DView(enemies[j], drawParams)) continue;

        SQ7x8 dx = wrapDistance(enemies[j].x, player.x, worldWidth);
        SQ7x8 dy = wrapDistance(enemies[j].y, player.y, worldHeight);
        SQ15x16 distSq = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestEnemy = j;
        }
    }

    if (nearestEnemy == INACTIVE_TARGET) return false;  // No target found

    // Activate the first available rocket slot
    for (uint8_t i = 0; i < MAX_ROCKETS; ++i) {
        if (rockets[i].targetIndex != INACTIVE_TARGET) continue;  // Slot is active

        rockets[i].x = player.x;
        rockets[i].y = player.y;
        rockets[i].angle = player.angle;
        rockets[i].lifetime = 0;
        rockets[i].targetIndex = nearestEnemy;
        rocketFireCooldown = ROCKET_FIRE_DELAY;
        return true;  // Rocket successfully fired
    }

    return false;  // No available rocket slots
}

// Function to update all active rockets
void updateRockets() {
    if (rocketFireCooldown > 0) rocketFireCooldown--;

    for (uint8_t i = 0; i < MAX_ROCKETS; ++i) {
        Rocket &rocket = rockets[i];
        if (rocket.targetIndex == INACTIVE_TARGET) continue;

        uint8_t target = rocket.targetIndex;
        if (enemies[target].state == EnemyState::Inactive) {
            rocket.targetIndex = INACTIVE_TARGET;
            rocket.lifetime = 0;
            continue;
        }

        DrawParameters drawParams;
        if (!to3DView(enemies[target], drawParams)) {
            rocket.targetIndex = INACTIVE_TARGET;
            rocket.lifetime = 0;
            continue;
        }

        // Calculate position differences
        SQ7x8 dx = wrapDistance(enemies[target].x, rocket.x, worldWidth);
        SQ7x8 dy = wrapDistance(enemies[target].y, rocket.y, worldHeight);

        // Adjust rocket angle towards the target
        SQ7x8 cosTheta = Cos(rocket.angle);
        SQ7x8 sinTheta = Sin(rocket.angle);
        int16_t cross = (int16_t)(cosTheta * dy - sinTheta * dx);

        if (cross > 0) {
            rocket.angle += ROCKET_TURN_SPEED;
        } else if (cross < 0) {
            rocket.angle -= ROCKET_TURN_SPEED;
        }

        // Move the rocket forward
        rocket.x += ROCKET_SPEED * Cos(rocket.angle);
        rocket.y += ROCKET_SPEED * Sin(rocket.angle);

        // Wrap around the world coordinates
        rocket.x = wrapCoordinate(rocket.x, worldWidth);
        rocket.y = wrapCoordinate(rocket.y, worldHeight);

        // Update lifetime and deactivate if expired
        if (++rocket.lifetime >= MAX_ROCKET_LIFETIME) {
            rocket.targetIndex = INACTIVE_TARGET;
            rocket.lifetime = 0;
        }
    }
}

// Function to render all active rockets
void renderRockets() {
  for (uint8_t i = 0; i < MAX_ROCKETS; ++i) {
    Rocket &rocket = rockets[i];
    if (rocket.targetIndex == INACTIVE_TARGET) continue;

    DrawParameters drawParams;
    if (to3DView(rocket, drawParams)) {
      // Draw the rocket with its scaled size
      uint8_t scaledSize = calculateSize(
        drawParams.distance,
        1, 6,    // min and max size
        0.5, 24  // min and max distance
      );
      arduboy.fillCircle(drawParams.x, drawParams.y, scaledSize, WHITE);
    }
  }
}

// Function to render a rectangle around the nearest enemy
void renderTargetRectangle() {
  uint8_t nearestEnemy = INACTIVE_TARGET;
  SQ15x16 nearestDistSq = SQ15x16(999999);  // Arbitrary large value

  // Find the nearest active enemy in view
  for (uint8_t j = 0; j < MAX_ENEMIES; ++j) {
    if (enemies[j].state == EnemyState::Inactive) continue;

    DrawParameters drawParams;
    if (!to3DView(enemies[j], drawParams)) continue;

    SQ7x8 dx = wrapDistance(enemies[j].x, player.x, worldWidth);
    SQ7x8 dy = wrapDistance(enemies[j].y, player.y, worldHeight);
    SQ15x16 distSq = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

    if (distSq < nearestDistSq) {
      nearestDistSq = distSq;
      nearestEnemy = j;
    }
  }

  if (nearestEnemy != INACTIVE_TARGET) {
    drawTargetedEnemyRectangle(nearestEnemy);
  }
}

// Function to check collisions between rockets and their target enemies
void checkRocketEnemyCollisions() {
  for (uint8_t i = 0; i < MAX_ROCKETS; ++i) {
    Rocket &rocket = rockets[i];
    if (rocket.targetIndex == INACTIVE_TARGET) continue;

    uint8_t target = rocket.targetIndex;
    if (enemies[target].state == EnemyState::Inactive) continue;

    SQ7x8 dx = wrapDistance(rocket.x, enemies[target].x, worldWidth);
    SQ7x8 dy = wrapDistance(rocket.y, enemies[target].y, worldHeight);
    SQ15x16 distSq = (SQ15x16)dx * dx + (SQ15x16)dy * dy;

    // Calculate collision distance dynamically
    SQ7x8 rocketDistance = rocket.lifetime * ROCKET_SPEED;
    uint8_t rocketSize = calculateSize(rocketDistance, 1, 6, 0.5, 24);
    SQ7x8 collisionDist = (enemies[target].scaledSize / 24) + rocketSize;
    SQ15x16 collisionThreshold = (SQ15x16)collisionDist * collisionDist;

    if (distSq <= collisionThreshold) {
      // Deactivate the rocket
      rocket.targetIndex = INACTIVE_TARGET;

      // Apply damage to the enemy
      WeaponType currentWeapon = player.mechs[player.currentMech].weapons[player.currentWeaponSlot];
      uint8_t damage = getWeaponDamage(currentWeapon);
      hitEnemy(target, damage);
    }
  }
}

const SQ7x8 laserRange = 16;  // Maximum range of the laser

void updateLaser() {
  // Laser direction vector
  SQ7x8 dx = Cos(player.angle);
  SQ7x8 dy = Sin(player.angle);

  // Retrieve the damage from the currently equipped weapon
  WeaponType currentWeaponType = player.mechs[player.currentMech].weapons[player.currentWeaponSlot];
  uint8_t damage = getWeaponDamage(currentWeaponType);

  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].state != EnemyState::Active) continue;

    // Vector from player to enemy with wrapping
    SQ7x8 ex_px = wrapDistance(enemies[i].x, player.x, worldWidth);
    SQ7x8 ey_py = wrapDistance(enemies[i].y, player.y, worldHeight);

    // Project enemy position onto laser direction
    SQ15x16 dot = (SQ15x16)(ex_px * dx + ey_py * dy);

    if (dot < 0 || dot > laserRange) continue;  // Enemy is behind or out of range

    // Compute perpendicular distance squared from enemy to laser line
    SQ15x16 perpDist = (SQ15x16)(dy * ex_px - dx * ey_py);
    if (perpDist * perpDist <= 1) {  // laserWidthSquared is 4, smaller value means higher accuracy
      // Enemy is hit by laser
      hitEnemy(i, damage);
    }
  }
}

void renderLaser() {
  // The laser starts at the weapon position on the screen
  uint8_t weaponScreenX = WIDTH / 3;
  uint8_t weaponScreenY = HEIGHT;  // Adjust based on your HUD

  // Calculate laser end point in world coordinates
  SQ7x8 laserEndX = wrapCoordinate(player.x + laserRange * Cos(player.angle), worldWidth);
  SQ7x8 laserEndY = wrapCoordinate(player.y + laserRange * Sin(player.angle), worldHeight);

  // Create an Object for the laser end point
  Object laserEnd = { laserEndX, laserEndY };

  DrawParameters endParams;
  if (to3DView(laserEnd, endParams)) {
    // Draw the laser line from the weapon to the laser end point
    arduboy.drawLine(weaponScreenX, weaponScreenY, endParams.x, endParams.y, WHITE);
    //} else {
    // Draw the laser to the top of the screen
    //arduboy.drawLine(weaponScreenX, weaponScreenY, weaponScreenX, 0, WHITE);
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
const uint8_t baseMovementHeat = 80;  // Base heat increase for movement (replace with individual mech stats)

// Player action flags
bool playerIsMoving = false;
bool playerIsShooting = false;

// Overheat management
uint16_t overheatTimer = 0;

// Update heat based on player actions
void updateHeat() {
  if (player.mechs[player.currentMech].status != MechStatus::Overheat) {
    // Calculate heat increase adjusted by heat sink value
    uint8_t movementHeatIncrease = calculateHeatIncrease(baseMovementHeat, player.mechs[player.currentMech].mechStats[1]);

    // Retrieve the current weapon's heat usage
    WeaponType currentWeaponType = player.mechs[player.currentMech].weapons[player.currentWeaponSlot];
    uint8_t shootingHeatUsage = getWeaponHeat(currentWeaponType);
    uint8_t shootingHeatIncrease = calculateHeatIncrease(shootingHeatUsage, player.mechs[player.currentMech].mechStats[1]);

    // Movement heat accumulation
    uint8_t movementHeatThreshold = 2 * (player.mechs[player.currentMech].mechStats[1] / 3);  // 60% threshold

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
    if (player.heat >= player.mechs[player.currentMech].mechStats[1]) {
      player.heat = player.mechs[player.currentMech].mechStats[1];
      player.mechs[player.currentMech].status = MechStatus::Overheat;
      overheatTimer = calculateOverheatDuration(player.mechs[player.currentMech].mechStats[1]);
    }
  } else {
    // Overheat state management
    if (overheatTimer > 0) {
      overheatTimer--;
    } else {
      player.mechs[player.currentMech].status = MechStatus::Normal;
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
  uint8_t dissipationRate = calculateDissipationRate(player.mechs[player.currentMech].mechStats[1]);

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

uint8_t findNextWeaponSlot(uint8_t currentSlot) {
  // Attempt to find the next occupied slot
  for (uint8_t i = 1; i < 3; ++i) {  // Check the other two slots
    uint8_t nextSlot = (currentSlot + i) % 3;
    if (player.mechs[player.currentMech].weapons[nextSlot] != WeaponType::None) {
      return nextSlot;
    }
  }
  // If no other slots are occupied, return the current slot
  return currentSlot;
}

bool laserActive = false;  // Indicates whether the laser is currently firing

void handleInput() {
  // Block player actions if in overheat state
  if (player.mechs[player.currentMech].status == MechStatus::Overheat) {
    laserActive = false;  // Ensure laser is deactivated during overheat
    return;               // Skip input handling during overheat
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
    WeaponType currentWeapon = player.mechs[player.currentMech].weapons[player.currentWeaponSlot];
    uint8_t &currentAmmo = player.mechs[player.currentMech].currentAmmo[player.currentWeaponSlot];

    if ((currentWeapon == WeaponType::Bullets || currentWeapon == WeaponType::HeavyBullets) && currentAmmo > 0) {
      if (fireBullet()) {          // Only decrement if fired
        playerIsShooting = true;
        --currentAmmo;
      }
    } else if ((currentWeapon == WeaponType::Rockets || currentWeapon == WeaponType::MediumRockets) && currentAmmo > 0) {
      if(fireRocket()) {
        playerIsShooting = true;
        --currentAmmo;
      }
    } else if (currentWeapon == WeaponType::Laser) {
      laserActive = true;
      playerIsShooting = true;
    }
  } else {
    laserActive = false;
  }

  // Handle rotation and movement as usual
  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= player.mechs[player.currentMech].mechStats[5];      // Rotate left
    player.viewAngle += player.mechs[player.currentMech].mechStats[5];  // Rotate right
    playerIsMoving = true;
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += player.mechs[player.currentMech].mechStats[5];      // Rotate right
    player.viewAngle -= player.mechs[player.currentMech].mechStats[5];  // Rotate left
    playerIsMoving = true;
  }

  // Handle movement with UP and DOWN buttons
  if (arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) {
    int8_t direction = (arduboy.pressed(UP_BUTTON) ? 1 : -1);

    player.x += direction * player.mechs[player.currentMech].moveSpeed * Cos(player.angle);
    player.y += direction * player.mechs[player.currentMech].moveSpeed * Sin(player.angle);

    player.x = wrapCoordinate(player.x, worldWidth);
    player.y = wrapCoordinate(player.y, worldHeight);
    playerIsMoving = true;
  }
}

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
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.clear();
  arduboy.pollButtons();

  // Change this switch case to if statements instead of nesting switch cases
  switch (currentState) {
    case GameState::Title_Menu:
      updateTitleMenu();
      break;
    case GameState::Init_New_Game:
      initNewGame();
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
      if (currentMission.numEnemies == 0) {
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

      updateBullets();               // Update bullet positions
      checkBulletEnemyCollisions();  // Check for bullet-enemy collisions
      renderBullets();               // Render bullets

      if (player.mechs[player.currentMech].weapons[player.currentWeaponSlot] == WeaponType::Rockets ||
        player.mechs[player.currentMech].weapons[player.currentWeaponSlot] == WeaponType::MediumRockets){
        renderTargetRectangle();
      }
      updateRockets();
      checkRocketEnemyCollisions();
      renderRockets();

      if (laserActive) {
        updateLaser();  // Check for laser-enemy collisions
        renderLaser();  // Render the laser beam
      }

      updateHeat();
      drawHUD();

      if (player.mechs[player.currentMech].health <= 0) {
        // Make sure screen is set back to normal (from taking damage)
        FX::enableOLED();
        arduboy.invert(false);
        FX::disableOLED();
        currentState = GameState::Game_Over;
      }
      break;
    case GameState::Game_Over:

      FX::drawBitmap(40, 30, gameOverStr, 0, dbmNormal);

      if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
        // Reset player day count before going back to title menu
        player.dayCount = 0;
        currentState = GameState::Title_Menu;
      }
      break;
  }

  FX::display(CLEAR_BUFFER);
}