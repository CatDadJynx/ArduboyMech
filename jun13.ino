// TODO: 
// - Fix customization menu player stats (because moveSpeed can currently become negative, this needs fixed)
// - Make missions give different amount of money based on difficulty
// - Add multiple mech slots
// - Add multiple weapon slots
// - Enemy AI (movement, attacking)
// - Implement player reload/repair (hanger menu)

#include <Arduboy2.h>
#include <ArduboyFX.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "Trig.h"    // Assuming Trig.h is in the same directory
#include "fxdata.h"  // this file contains all the references to FX data
#include "src/fonts/Font4x6.h"

Arduboy2 arduboy;

Font4x6 font4x6 = Font4x6(7);

bool minimapEnabled = true;  // New variable to track minimap state

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
  Game_Play
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

/*struct MechStats {
  MechStatus status;
  //SQ7x8 moveSpeed;
  //uint8_t rotationSpeed;
};*/

struct PlayerMechStats /*: MechStats*/ {
  uint8_t weight;
  uint8_t heat;
  uint8_t armor;          // Initialized to 1 for testing
  SQ7x8 moveSpeed;        // Initialized to 1 for testing, should be populated elsewhere
  uint8_t rotationSpeed;  // Initialized to 0.1 for testing,  should be populated elsewhere
  //uint8_t weaponSlots[3];  // Array for weapon slots

};

// Example logic for equipping weapons
//void equipWeapon(uint8_t mechIndex, uint8_t weaponIndex) {
//  player.weaponSlots[mechIndex] = weaponIndex;
//}

// Definitions of MechStats for different mech types
constexpr PlayerMechStats playerMothraStats{
  //MechStatus::Normal,
  100,
  1,
  100,
  0.3,
  3
};

constexpr PlayerMechStats playerBattleCatStats{
  //MechStatus::Normal,
  115,
  2,
  115,
  0.2,
  2
};

constexpr PlayerMechStats playerThorHammerStats{
  //MechStatus::Normal,
  127,
  3,
  127,
  0.1,
  1
};

struct Player : Object {
  uint8_t dayCount = 0;
  uint24_t money = 0;
  MechType mechType;
  MechStatus mechStatus;
  PlayerMechStats playerMechStats;
  uint24_t cockpitSprite;
  uint8_t health = 0;  // Player health
  uint8_t angle = 0;
  uint8_t viewAngle = 0;
  uint8_t bulletDamage = 1;  // Bullet damage
} player;



enum class EnemyState : uint8_t {
  Active,
  Inactive,
  Exploding
};

// Update Enemy struct to include MechType
struct Enemy : public Object {
  EnemyState state = EnemyState::Inactive;
  SQ7x8 moveSpeed = 0.1;
  uint8_t scaledSize = 0;
  uint8_t health = 1;
  MechType mechType;  // Add MechType to Enemy struct

  uint8_t currentFrame = 0;
  uint8_t maxFrames = 0;

  SQ7x8 targetX;
  SQ7x8 targetY;
};

// Initialize objects
const uint8_t maxEnemies = 3;  // Example enemy count
Enemy enemies[maxEnemies];     // Example enemy array

void updateEnemyMovement(Enemy& enemy);

void updateEnemyMovement(Enemy& enemy) {
  if (enemy.state == EnemyState::Active) {
    SQ7x8 dx = player.x - enemy.x;
    SQ7x8 dy = player.y - enemy.y;

    if (abs(dx) > abs(dy)) {
      if (dx > 0) enemy.x += enemy.moveSpeed;
      else enemy.x -= enemy.moveSpeed;
    } else {
      if (dy > 0) enemy.y += enemy.moveSpeed;
      else enemy.y -= enemy.moveSpeed;
    }
  }
}

void updateEnemyAttack(Enemy& enemy);

void updateEnemyAttack(Enemy& enemy) {
  if (enemy.state == EnemyState::Active) {
    SQ7x8 dx = player.x - enemy.x;
    SQ7x8 dy = player.y - enemy.y;
    SQ7x8 squaredDistance = (dx * dx) + (dy * dy);

    // Check if enemy is within attack range (using squared distance)
    const SQ7x8 attackRange = 5;
    const SQ7x8 squaredAttackRange = attackRange * attackRange;
    if (squaredDistance < squaredAttackRange) {
      // Random chance to attack
      if (random(0, 100) < 25) {  // 25% chance to attack
        player.health -= 1;  // Reduce player health
        // Flash screen to register hits
      }
    }
  }
}

// TODO: Simplify mission by removing activeEnemies and decrememnting numEnemies instead?
struct Mission {
  uint8_t numEnemies;
  uint8_t activeEnemies;
  MechType mechs[maxEnemies];  // Assuming a max of 3 mechs per mission
};

// Existing code...

const uint8_t maxMissions = 3;  // Maximum number of missions available at any time
Mission availableMissions[maxMissions];
Mission currentMission;

// Modify generateMission to include MechType
Mission generateMission();

Mission generateMission() {
  Mission mission;
  //mission.dayCount = player.dayCount;

  if (player.dayCount < 10) {
    mission.numEnemies = random(1, 3);  // 1 to 2 light mechs
    for (int i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
    }
  } else if (player.dayCount >= 10 && player.dayCount < 20) {
    mission.numEnemies = random(2, 4);  // 2 to 3 mechs
    for (int i = 0; i < mission.numEnemies; i++) {
      mission.mechs[i] = MechType::Mothra;
    }
  } else if (player.dayCount >= 20 && player.dayCount < 30) {
    mission.numEnemies = random(1, 4);  // 1 to 3 mechs
    for (int i = 0; i < mission.numEnemies; i++) {
      if (random(0, 2) == 0) {
        mission.mechs[i] = MechType::Battle_Cat;
      } else {
        mission.mechs[i] = MechType::Mothra;
      }
    }
  }
  // Add more conditions for other day count ranges as needed

  return mission;
}

// Function to populate the list of available missions
void populateMissionList() {
  for (int i = 0; i < maxMissions; i++) {
    availableMissions[i] = generateMission();
  }
}

void initNewGame() {
  player.dayCount = 1;
  player.money = 20000;
  player.mechType = MechType::Battle_Cat;
  player.playerMechStats = playerBattleCatStats;
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

};

SaveData saveData;

void initSaveGame() {
  saveData.dayCount = player.dayCount;
  saveData.money = player.money;
  saveData.mechType = player.mechType;
  saveData.saveMechStats = player.playerMechStats;
  saveData.health = player.health;
  for (int i = 0; i < maxMissions; i++) {
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
  for (int i = 0; i < maxMissions; i++) {
    availableMissions[i] = saveData.savedMissions[i];
  }
}

void updateNoSaveScreen() {
  font4x6.setCursor(5, 5);
  font4x6.print("No save data");
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

struct MenuItem {
  int16_t x, y, width, height;  // Position and size for the selection rectangle
};

struct Menu {
  MenuItem* items;           // Array of menu items
  uint8_t itemCount;         // Total number of items in the menu
  uint8_t currentSelection;  // Index of the currently selected item
};

// Example of defining a main menu
const MenuItem mainMenuItems[] PROGMEM = {
  { 10, 18, 26, 26 },  // First menu item
  { 51, 18, 26, 26 },  // Second menu item
  { 93, 18, 26, 26 }   // Third menu item
};
Menu mainMenu = { mainMenuItems, 3, 0 };  // Main menu with 3 items, starting selection at 0

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
  { 61, 1, 33, 17 },
  { 61, 18, 33, 17 },
  { 94, 1, 33, 17 },
  { 94, 18, 33, 17 }
};
Menu hangerMenu = { hangerMenuItems, 4, 0 };

const MenuItem customizationMenuItems[] PROGMEM = {
  { 79, 1, 33, 15 },
  { 79, 16, 33, 15 },
  { 79, 31, 33, 15 },
  { 79, 46, 33, 15 }
};
Menu customizationMenu = { customizationMenuItems, 4, 0 };

void updateMenu(Menu& menu);

void updateMenu(Menu& menu) {
  MenuItem selectedItem;
  memcpy_P(&selectedItem, &menu.items[menu.currentSelection], sizeof(MenuItem));
  // Now use selectedItem as before
  arduboy.drawRect(selectedItem.x, selectedItem.y, selectedItem.width, selectedItem.height, WHITE);

  // Navigation logic
  if (arduboy.justPressed(UP_BUTTON)) {
    if (menu.currentSelection > 0) menu.currentSelection--;
  } else if (arduboy.justPressed(DOWN_BUTTON)) {
    if (menu.currentSelection < menu.itemCount - 1) menu.currentSelection++;
  }
  // Selection logic can be handled individually in each state's update function
}

void updateMainMenu() {
  FX::drawBitmap(0, 0, mainMenu128x64, 0, dbmMasked);

  font4x6.setCursor(10, 50);  // Adjust position as needed
  font4x6.print(F("Day: "));
  font4x6.print(player.dayCount);

  updateMenu(mainMenu);

  if (arduboy.justPressed(A_BUTTON)) {
    // Handle selection based on mainMenu.currentSelection
    //previousState = currentState;
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
/* ********NEEDS TO BE MOVED FROM SETUP TO MISSION INITIALIZATION*******
  // Initialize enemies with example positions
  for (uint8_t i = 0; i < maxEnemies; ++i) {
    enemies[i].state = Active;
    enemies[i].x = random(worldWidth);   // Random X position
    enemies[i].y = random(worldHeight);  // Random Y position
  }
*/
void initializeMission(const Mission& mission) {
  player.x = random(worldWidth);  // Random player X position
  player.y = random(worldHeight); // Random player Y position
  
  currentMission.activeEnemies = currentMission.numEnemies;
  for (uint8_t i = 0; i < maxEnemies; ++i) {
    if (i < mission.numEnemies) {
      enemies[i].state = EnemyState::Active;
      enemies[i].mechType = mission.mechs[i];
      // Randomly set enemy positions within world bounds
      enemies[i].x = random(worldWidth);
      enemies[i].y = random(worldHeight);
    } else {
      enemies[i].state = EnemyState::Inactive;
    }
    switch (enemies[i].mechType) {
      case MechType::Mothra:
        enemies[i].health = 1;
        enemies[i].moveSpeed = 0.1;
        break;
      case MechType::Battle_Cat:
        enemies[i].health = 2;
        enemies[i].moveSpeed = 0.2;
        break;
    }
  }
}

void completeMission() {
  // Example logic for completing a mission
  player.money += 1000;  // Award money to player
  populateMissionList();  // Generate new set of missions
  currentState = GameState::Main_Menu;  // Return to main menu
  ++player.dayCount;
}

static void printString(Font4x6 &font, uint24_t addr, uint8_t x, uint8_t y) {
    uint8_t index = 0;
    unsigned char character;
    FX::seekData(addr);
    while ((character = FX::readPendingUInt8()) != '\0') {
        font.printChar(character, x, y);
        x += 5;
    }
    (void)FX::readEnd();
}

uint8_t selectedMissionIndex = 0;  // Index of the currently selected mission

void updateMissionMenu() {
  FX::drawBitmap(0, 0, missionMenu128x64, 0, dbmMasked);
  updateMenu(missionMenu);

  // Directly print the label for enemies
  font4x6.setCursor(38, 10);  // Adjust position as needed
  font4x6.print(F("Enemies: "));
  //printString(font4x6, enemiesStr, 38, 10);

  //font4x6.setCursor(38, 10);  // Adjust position as needed
  // Directly print the number of enemies without using a buffer
  font4x6.print(availableMissions[selectedMissionIndex].numEnemies);
  //printString(font4x6, availableMissions[selectedMissionIndex].numEnemies, 38, 16);

  // For enemy types, print directly in a loop
  for (int i = 0; i < availableMissions[selectedMissionIndex].numEnemies; i++) {
    // Instead of storing type names in a buffer, print directly
    switch (availableMissions[selectedMissionIndex].mechs[i]) {
      case MechType::Mothra:
        //printString(font4x6, mothraStr, 38, 25);
        font4x6.print(F("\n Mothra"));
        break;
      case MechType::Battle_Cat:
        //printString(font4x6, battleCatStr, 38, 25);
        font4x6.print(F("\n BattleCat"));
        break;
      // Add cases for other mech types as needed
    }
  }

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

void updateHangerMenu() {
  FX::drawBitmap(0, 0, hangerMenu128x64, 0, dbmMasked);
  updateMenu(hangerMenu);

  if (arduboy.justPressed(B_BUTTON)) {
    currentState = previousState;
  }
  if (arduboy.justPressed(A_BUTTON)) {
    // Handle selection based on mainMenu.currentSelection
    //previousState = currentState;
    switch (hangerMenu.currentSelection) {
      case 0:
        // Repair/reload (if the player has multiple mechs slots, only repair/reload current mech)
        break;
      case 1:
        // Mech customization menu (change previous state to current state before changing to next menu state)
        currentState = GameState::Customization_Submenu;
        break;
      case 2:
        // Buy mech
        break;
      case 3:
        // Sell mech
        break;
    }
  }
}

bool adjustingStat = false;

void updateCustomizationMenu() {
  FX::drawBitmap(0, 0, customizationMenu128x64, 0, dbmMasked);
  font4x6.setCursor(2, 1);
  font4x6.print("Armor     ");
  font4x6.print(player.playerMechStats.armor);
  font4x6.setCursor(2, 8);
  font4x6.print("Weight    ");
  font4x6.print(player.playerMechStats.weight);
  font4x6.setCursor(2, 15);
  font4x6.print("Heatsink  ");
  font4x6.print(player.playerMechStats.heat);
  font4x6.setCursor(2, 22);
  font4x6.print("MoveSpeed ");
  font4x6.print(static_cast<float>(player.playerMechStats.moveSpeed));
  font4x6.setCursor(2, 36);
  font4x6.print("Credits  ");
  font4x6.print(static_cast<long>(player.money));

  if (!adjustingStat) {
    updateMenu(customizationMenu);
  }

  if (arduboy.justPressed(B_BUTTON)) {
    if (adjustingStat) {
      adjustingStat = false;
    } else {
      currentState = previousState;
    }
  }

  if (arduboy.justPressed(A_BUTTON) && !adjustingStat) {
    adjustingStat = true;
  } else if (adjustingStat) {
    switch (customizationMenu.currentSelection) {
      case 0:  // Armor up/down (weight up/down, speed down/up)
        if (arduboy.justPressed(RIGHT_BUTTON) && player.playerMechStats.armor < 120) {
          FX::drawBitmap(112, 1, rightArrowSmall, 0, dbmMasked);
          player.playerMechStats.armor++;
          player.playerMechStats.weight++;
          player.playerMechStats.moveSpeed -= 0.01;
          player.money -= 100;
        } else if (arduboy.justPressed(LEFT_BUTTON) && player.playerMechStats.armor > 1) {
          FX::drawBitmap(72, 1, leftArrowSmall, 0, dbmMasked);
          player.playerMechStats.armor--;
          player.playerMechStats.weight--;
          player.playerMechStats.moveSpeed += 0.01;
          player.money += 100;
        }
        break;
      case 1:
        // Buy/Sell weapons
        break;
      case 2:  // Heatsink up/down (weight up/down, speed down/up)
        if (arduboy.justPressed(RIGHT_BUTTON) && player.playerMechStats.heat < 100) {
          FX::drawBitmap(112, 32, rightArrowSmall, 0, dbmMasked);
          player.playerMechStats.heat++;
          player.playerMechStats.weight++;
          player.playerMechStats.moveSpeed -= 0.01;
          player.money -= 100;
        } else if (arduboy.justPressed(LEFT_BUTTON) && player.playerMechStats.heat > 1) {
          FX::drawBitmap(72, 32, leftArrowSmall, 0, dbmMasked);
          player.playerMechStats.heat--;
          player.playerMechStats.weight--;
          player.playerMechStats.moveSpeed += 0.01;
          player.money += 100;
        }
        break;
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

void drawEnemy(uint8_t x, uint8_t y) {
  // Draw the enemy with the calculated size
  arduboy.drawRect(x - enemies[currentEnemyIndex].scaledSize / 2, y - enemies[currentEnemyIndex].scaledSize / 2, enemies[currentEnemyIndex].scaledSize, enemies[currentEnemyIndex].scaledSize, WHITE);

  // Choose a sprite based on the enemy's MechType and size
  uint24_t sprite;
  uint8_t spriteWidth, spriteHeight;
  //uint8_t frame = 0;

  switch (enemies[currentEnemyIndex].mechType) {
    case MechType::Mothra:
      // Select Mothra sprites based on size
      if (enemies[currentEnemyIndex].scaledSize < 8) {
        sprite = enemy2x3;
        spriteWidth = enemy2x3Width;
        spriteHeight = enemy2x3Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      } else if (enemies[currentEnemyIndex].scaledSize < 15) {
        sprite = mech6x8;
        spriteWidth = mech6x8Width;
        spriteHeight = mech6x8Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      } else if (enemies[currentEnemyIndex].scaledSize < 20) {
        sprite = Mothra8x11;
        spriteWidth = Mothra8x11Width;
        spriteHeight = Mothra8x11Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      } else if (enemies[currentEnemyIndex].scaledSize < 30) {
        sprite = Mothra16x21;
        spriteWidth = Mothra16x21Width;
        spriteHeight = Mothra16x21Height;
        enemies[currentEnemyIndex].maxFrames = Mothra16x21Frames;
      } else if (enemies[currentEnemyIndex].scaledSize < 40) {
        sprite = Mothra30x40;
        spriteWidth = Mothra30x40Width;
        spriteHeight = Mothra30x40Height;
        enemies[currentEnemyIndex].maxFrames = Mothra30x40Frames;
      } else if (enemies[currentEnemyIndex].scaledSize < 60) {
        sprite = Mothra50x67;
        spriteWidth = Mothra50x67Width;
        spriteHeight = Mothra50x67Height;
        enemies[currentEnemyIndex].maxFrames = Mothra50x67Frames;
      } else if (enemies[currentEnemyIndex].scaledSize < 80) {
        sprite = Mothra70x94;
        spriteWidth = Mothra70x94Width;
        spriteHeight = Mothra70x94Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      } else if (enemies[currentEnemyIndex].scaledSize < 100) {
        sprite = Mothra90x121;
        spriteWidth = Mothra90x121Width;
        spriteHeight = Mothra90x121Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      } else {
        sprite = Mothra110x148;
        spriteWidth = Mothra110x148Width;
        spriteHeight = Mothra110x148Height;
        enemies[currentEnemyIndex].maxFrames = 0;
      }
      break;
    case MechType::Battle_Cat:
      // Select Battle_Cat sprites based on size
      // Use the same sprite sizes as for Mothra for now, adjust as needed
      if (enemies[currentEnemyIndex].scaledSize < 8) {
        sprite = enemy2x3;
        spriteWidth = enemy2x3Width;
        spriteHeight = enemy2x3Height;
      } else if (enemies[currentEnemyIndex].scaledSize < 15) {
        sprite = mech6x8;
        spriteWidth = mech6x8Width;
        spriteHeight = mech6x8Height;
      } else if (enemies[currentEnemyIndex].scaledSize < 20) {
        sprite = Battle_Cat8x11;
        spriteWidth = Battle_Cat8x11_width;
        spriteHeight = Battle_Cat8x11_height;
      } else if (enemies[currentEnemyIndex].scaledSize < 30) {
        sprite = Battle_Cat16x21;
        spriteWidth = Battle_Cat16x21_width;
        spriteHeight = Battle_Cat16x21_height;
      } else if (enemies[currentEnemyIndex].scaledSize < 40) {
        sprite = Battle_Cat30x40;
        spriteWidth = Battle_Cat30x40_width;
        spriteHeight = Battle_Cat30x40_height;
      } else if (enemies[currentEnemyIndex].scaledSize < 60) {
        sprite = Battle_Cat50x67;
        spriteWidth = Battle_Cat50x67_width;
        spriteHeight = Battle_Cat50x67_height;
      } else if (enemies[currentEnemyIndex].scaledSize < 80) {
        sprite = Battle_Cat70x94;
        spriteWidth = Battle_Cat70x94_width;
        spriteHeight = Battle_Cat70x94_height;
      } else if (enemies[currentEnemyIndex].scaledSize < 100) {
        sprite = Battle_Cat90x121;
        spriteWidth = Battle_Cat90x121_width;
        spriteHeight = Battle_Cat90x121_height;
      } else {
        sprite = Battle_Cat110x148;
        spriteWidth = Battle_Cat110x148_width;
        spriteHeight = Battle_Cat110x148_height;
      }
      break;
    }

    if (arduboy.everyXFrames(3)) {
      // If current frame is greater than max, reset to 0 and exit (to loop animation)
      /*if (enemies[currentEnemyIndex].currentFrame > enemies[currentEnemyIndex].maxFrames) {
        enemies[currentEnemyIndex].currentFrame = 0;
        return;
      }*/

      if (enemies[currentEnemyIndex].currentFrame >= enemies[currentEnemyIndex].maxFrames) {
        enemies[currentEnemyIndex].currentFrame = 0;
        return;
      }
      // If crrent frame is less than max, increment frame
      if (enemies[currentEnemyIndex].currentFrame < enemies[currentEnemyIndex].maxFrames){
        ++enemies[currentEnemyIndex].currentFrame;
      }
    }
    FX::drawBitmap(x - spriteWidth / 2, y - spriteWidth / 2, sprite, enemies[currentEnemyIndex].currentFrame, dbmMasked);
}

void updateAndDrawExplosion(uint8_t x, uint8_t y) {

  uint24_t sprite;
  uint8_t spriteSize;
  //Reset enemy's current animation frame?
  //enemies[currentEnemyIndex].currentFrame = 0;

  if (enemies[currentEnemyIndex].scaledSize < 23) {
    sprite = explosion23x23;
    spriteSize = 23;
    enemies[currentEnemyIndex].maxFrames = explosion23x23Frames;
  } else if (enemies[currentEnemyIndex].scaledSize < 46) {
    sprite = explosion46x46;
    spriteSize = 46;
    enemies[currentEnemyIndex].maxFrames = explosion46x46Frames;
  } else if (enemies[currentEnemyIndex].scaledSize < 92) {
    sprite = explosion92x92;
    spriteSize = 92;
    enemies[currentEnemyIndex].maxFrames = explosion92x92Frames;
  } else {
    sprite = explosion110x110;
    spriteSize = 110;
    enemies[currentEnemyIndex].maxFrames = explosion110x110Frames;
  }

  // Increment frame in controlled intervals
  if (arduboy.everyXFrames(10)) {
    // If current frame is greater than max frames
    if (enemies[currentEnemyIndex].currentFrame > enemies[currentEnemyIndex].maxFrames) {
      //Reset counter to 0?
      //enemies[currentEnemyIndex].currentFrame = 0;

      //Decrement current mission's active enemies then change enemy's state to inactive
      --currentMission.activeEnemies;
      enemies[currentEnemyIndex].state = EnemyState::Inactive;
      return; // Stop further processing if the enemy is now inactive
    }
    ++enemies[currentEnemyIndex].currentFrame;
  }

  // Draw the current frame of the explosion every frame
  FX::drawBitmap(x - spriteSize / 2, y - spriteSize / 2, sprite, enemies[currentEnemyIndex].currentFrame, dbmMasked);
}

void updateEnemies() {
  for (uint8_t i = 0; i < maxEnemies; ++i) {
    if (enemies[i].state == EnemyState::Active) {
      //updateEnemyMovement(enemies[i]);
      updateEnemyAttack(enemies[i]);
    }
  }
}

void updateAndRenderEnemies() {
  const uint8_t minEnemySize = 2;
  const uint8_t maxEnemySize = 120;
  const SQ7x8 minEnemyDistance = 0.5;
  const SQ7x8 maxEnemyDistance = 16;
  // Move this whole part to its own updateDrawEnemies function:
  for (uint8_t i = 0; i < currentMission.numEnemies; ++i) {
    if (enemies[i].state != EnemyState::Inactive) {

      // Update current enemy index
      currentEnemyIndex = i;

      // Prepare an empty DrawParameters object
      DrawParameters drawParameters;

      // Determine whether the object should be drawn
      if (to3DView(enemies[currentEnemyIndex], drawParameters)) {
        // If the object is onscreen, update scaled size and draw it
        if (currentEnemyIndex >= 0 && currentEnemyIndex < maxEnemies) {
          enemies[currentEnemyIndex].scaledSize = calculateSize(drawParameters.distance, minEnemySize, maxEnemySize, minEnemyDistance, maxEnemyDistance);
        }
        if (enemies[currentEnemyIndex].state == EnemyState::Active) {
          drawEnemy(drawParameters.x, drawParameters.y);
        } else if (enemies[i].state == EnemyState::Exploding) {
          updateAndDrawExplosion(drawParameters.x, drawParameters.y);
        }
      }
    }
  }
}

void drawMinimap() {
  arduboy.fillRect(0, 0, gridX * cellSize, gridY * cellSize, BLACK);
  for (int i = 0; i < gridX; i++) {
    for (int j = 0; j < gridY; j++) {
      arduboy.drawPixel(i * cellSize, j * cellSize, WHITE);
    }
  }

  // Draw the player as a triangle
  constexpr uint8_t triangleSize = 2;
  constexpr uint8_t triangleAngleOffset = 96;  // In binary radians

  // Calculate and draw the player triangle
  int frontX = static_cast<int8_t >(player.x + Cos(player.angle) * triangleSize);
  int frontY = static_cast<int8_t >(player.y + Sin(player.angle) * triangleSize);
  int rearLeftX = static_cast<int8_t >(player.x + Cos(player.angle - triangleAngleOffset) * triangleSize);
  int rearLeftY = static_cast<int8_t >(player.y + Sin(player.angle - triangleAngleOffset) * triangleSize);
  int rearRightX = static_cast<int8_t >(player.x + Cos(player.angle + triangleAngleOffset) * triangleSize);
  int rearRightY = static_cast<int8_t >(player.y + Sin(player.angle + triangleAngleOffset) * triangleSize);

  arduboy.fillTriangle(frontX, frontY, rearLeftX, rearLeftY, rearRightX, rearRightY, WHITE);
}

void drawEnemiesOnMinimap() {
  for (uint8_t i = 0; i < currentMission.numEnemies; ++i) {
    if (enemies[i].state == EnemyState::Active) {
      arduboy.drawRect((uint8_t)(enemies[i].x - 1), (uint8_t)(enemies[i].y - 1), 3, 3, WHITE);
    }
  }
}

// Define the Bullet struct
struct Bullet : public Object {
  SQ7x8 speed = 0.2;
  uint8_t angle;
  bool active = false;
  uint8_t scaledSize = 0;  // Scaled size based on distance
  SQ7x8 lifetime = 0;
};

const uint8_t maxBullets = 5;  // Max number of bullets
Bullet bullets[maxBullets];    // Bullet array
const uint8_t maxBulletDistance = 10;

void fireBullet() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].x = player.x;
      bullets[i].y = player.y;
      bullets[i].angle = player.angle;
      break;
    }
  }
}

void updateBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bullets[i].active) {
      // Update bullet position
      bullets[i].x += bullets[i].speed * Cos(bullets[i].angle);
      bullets[i].y += bullets[i].speed * Sin(bullets[i].angle);
      bullets[i].x = wrapCoordinate(bullets[i].x, worldWidth);
      bullets[i].y = wrapCoordinate(bullets[i].y, worldHeight);

      // Increse bullet lifetime and check for expiration
      bullets[i].lifetime += bullets[i].speed;

      //5 is max bullet distance
      if (bullets[i].lifetime >= maxBulletDistance) {
        bullets[i].active = false;
        bullets[i].lifetime = 0;
      }
    }
  }
}

int8_t currentBulletIndex = -1;  // Global index for the currently rendered bullet

void drawBullet(uint8_t x, uint8_t y, SQ7x8 distance) {
  const uint8_t minBulletSize = 1;
  const uint8_t maxBulletSize = 5;
  const SQ7x8 minBulletDistance = 0.5;
  const SQ7x8 maxBulletDistance = 16;

  // Storing the scaled size for collision detection purposes
  bullets[currentBulletIndex].scaledSize = calculateSize(distance, minBulletSize, maxBulletSize, minBulletDistance, maxBulletDistance);
  arduboy.fillCircle(x, y, bullets[currentBulletIndex].scaledSize, WHITE);
}

void renderBullets() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bullets[i].active) {
      currentBulletIndex = i;
      // Prepare an empty DrawParameters object
      DrawParameters drawParameters;
      // Determine whether the object should be drawn
      if (to3DView(bullets[i], drawParameters))
        // If the object is onscreen, draw it
        drawBullet(drawParameters.x, drawParameters.y, drawParameters.distance);
    }
  }
  currentBulletIndex = -1;  // Reset index after rendering bullets
}

void checkBulletEnemyCollisions() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bullets[i].active) {
      for (uint8_t j = 0; j < maxEnemies; ++j) {
        if (enemies[j].state != EnemyState::Inactive) {
          SQ7x8 dx = bullets[i].x - enemies[j].x;
          SQ7x8 dy = bullets[i].y - enemies[j].y;

          // Calculate squared distance
          SQ15x16 distanceSquared = SQ15x16(dx) * dx + SQ15x16(dy) * dy;

          // Use a fraction of the scaled size for more accurate collision detection (larger denominator means narrower detection)
          SQ7x8 collisionDistance = (enemies[j].scaledSize / 8) + bullets[i].scaledSize;
          SQ15x16 collisionThreshold = SQ15x16(collisionDistance) * collisionDistance;

          // Check for collision
          if (distanceSquared <= collisionThreshold) {
            // Handle collision
            bullets[i].active = false;
            hitEnemy(j);
            // Additional collision handling code here
          }
        }
      }
    }
  }
}

// Maybe change to function that returns bool (so logic can be continued outside of function)
// and separate enemy health decrement and state change to be handled elsewhere?
// So there can also be visual confirmation of hits by inverting colors in rendering logic?
void hitEnemy(uint8_t bulletEnemyIndex) {
  if (enemies[bulletEnemyIndex].health <= 0 && enemies[bulletEnemyIndex].state != EnemyState::Exploding) {
    enemies[bulletEnemyIndex].state = EnemyState::Exploding;
  }
  enemies[bulletEnemyIndex].health -= player.bulletDamage;  // Move below conditional?
}

void handleInput() {
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
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();

  FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);  // // initialise FX chip
  FX::loadGameState(saveData);

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
      if (currentMission.activeEnemies == 0){
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
      updateAndRenderEnemies();

      // Bullet handling:
      updateBullets();               // Update bullet positions
      checkBulletEnemyCollisions();  // Check for bullet-enemy collisions
      renderBullets();               // Render bullets

      FX::drawBitmap(0, 0, battleCatCockpit128x64, 0, dbmMasked);
      //FX::drawBitmap(0, 0, thorHammerCockpit128x64, 0, dbmMasked);

      if (minimapEnabled) {
        drawMinimap();
        drawEnemiesOnMinimap();
      }

      break;
  }

  FX::display(CLEAR_BUFFER);
}