/*TODO:
Mask cockpit sprite
Shooting mechanics
Sprites::draw buildings/mountains sprites at horizon
Player health
Player armor
Player overheat
Player weapons/ammo
Player move speed (legs)
Player rotation speed (chassis)
Mech weight
Heatsink
View bobbing? Jump jets?
Player money
Enemy health
Enemy pathfinding
Main menu
Mech customization menu
Mission menu
Save/load player data
Upgrades for things like enemy nav points/lock ons? Flares/shaff?

weapon slots w/ ammo or energy weapon recharge tradeoff
Energy weapons burn off energy shields w/ straight long shots, MG damages armor/health once shields are gone, missiles longer range damage armor/health?
*/

#include <Arduboy2.h>
#include "GameSprites.h"

Arduboy2 arduboy;

constexpr uint8_t gridX = 10;
constexpr uint8_t gridY = 10;
constexpr uint8_t gridSpacing = 4;
constexpr uint8_t worldXLimit = gridX * gridSpacing;
constexpr uint8_t worldYLimit = gridY * gridSpacing;
constexpr uint8_t screenCenterX = WIDTH / 2;
constexpr uint8_t screenCenterY = HEIGHT / 2;
constexpr int8_t horizonOffset = -20;

constexpr float triangleAngleOffset = 2.35f;

bool minimapEnabled = false;  // New variable to track minimap state

struct Player {
  float x = 0.0f;
  float y = 0.0f;
  float angle = 0.0f;
  float playerViewX = 0.0f;
  float viewAngle = 0.0f;

  float rotationSpeed = 0.1f;
  float moveSpeed = 0.5f;
  uint8_t cockpitFrame = 0;

  void wrapPosition() {
    x = fmod(x + worldXLimit, worldXLimit);
    y = fmod(y + worldYLimit, worldYLimit);
    playerViewX = fmod(playerViewX + worldXLimit, worldXLimit);
  }

  void normalizeAngle() {
    angle = fmod(angle + TWO_PI, TWO_PI);
  }

  void normalizeViewAngle() {
    viewAngle = fmod(viewAngle + TWO_PI, TWO_PI);
  }

  void drawPlayerSprite(){
    Sprites::drawSelfMasked(0, 0, Cockpit, cockpitFrame);
  }
} player;

struct Enemy {
  float x;
  float y;
};

constexpr uint8_t enemyCount = 3;
Enemy enemies[enemyCount] = {
  {10, 10},
  {20, 20},
  {30, 30}
};

void drawEnemiesOnMinimap() {
  for (uint8_t i = 0; i < enemyCount; ++i) {
    arduboy.drawRect(enemies[i].x - 1, enemies[i].y - 1, 3, 3, WHITE);
  }
}

void drawMinimap() {
  arduboy.fillRect(0, 0, gridX * gridSpacing, gridY * gridSpacing, BLACK);
  for (uint8_t x = 0; x < gridX; ++x) {
    for (uint8_t y = 0; y < gridY; ++y) {
      arduboy.drawPixel(x * gridSpacing, y * gridSpacing, WHITE);
    }
  }

  constexpr uint8_t triangleSize = 3;
  arduboy.fillTriangle(
    player.x - triangleSize * sin(player.angle),
    player.y + triangleSize * cos(player.angle),
    player.x - triangleSize * sin(player.angle - triangleAngleOffset),
    player.y + triangleSize * cos(player.angle - triangleAngleOffset),
    player.x - triangleSize * sin(player.angle + triangleAngleOffset),
    player.y + triangleSize * cos(player.angle + triangleAngleOffset),
    WHITE);
}

void handleInput(float playerViewCos, float playerViewSin, float playerCos, float playerSin) {
  if (arduboy.justPressed(A_BUTTON)) {  // Check for A button press
    minimapEnabled = !minimapEnabled;  // Toggle minimap
    if (player.cockpitFrame == 0)
    {
      player.cockpitFrame = 1;
    }
    else player.cockpitFrame = 0;
  }
  if (arduboy.pressed(B_BUTTON)) {
    debugDisplayPlayerInfo();  
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += player.rotationSpeed;
    player.viewAngle -= player.rotationSpeed;
  }
  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= player.rotationSpeed;
    player.viewAngle += player.rotationSpeed;
  }
  if (arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) {
    float direction = (arduboy.pressed(UP_BUTTON) ? 1.0f : -1.0f);
    player.x -= direction * player.moveSpeed * playerSin;
    player.y += direction * player.moveSpeed * playerCos;
    player.playerViewX -= direction * player.moveSpeed * playerViewSin;
  }
  player.normalizeAngle();
  player.normalizeViewAngle();
  player.wrapPosition();
}

float wrapCoordinate(float val, float limit) {
  while (val < -limit / 2) val += limit;
  while (val >= limit / 2) val -= limit;
  return val;
}

//viewX = worldWidth - player.x
void draw3DView(float playerViewCos, float playerViewSin) {
  constexpr int8_t vanishingPointXLeft = screenCenterX - 1;
  constexpr int8_t vanishingPointY = screenCenterY + horizonOffset;
  constexpr int8_t depth = 120;

  for (int x = -gridX; x <= gridX; ++x) {
    for (int y = -gridY; y <= gridY; ++y) {
      float relX = (x * gridSpacing) - player.playerViewX;
      float relY = (y * gridSpacing) - player.y;

      float wrappedX = wrapCoordinate(relX, worldXLimit);
      float wrappedY = wrapCoordinate(relY, worldYLimit);

      float rotatedX = wrappedX * playerViewCos + wrappedY * playerViewSin;
      float rotatedY = wrappedY * playerViewCos - wrappedX * playerViewSin;

      if (rotatedY <= 0) continue;

      int16_t screenX = screenCenterX + (int16_t)(rotatedX / rotatedY * depth);
      int16_t screenY = vanishingPointY + (int16_t)(100 / rotatedY);

      if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
        arduboy.drawPixel(screenX, screenY, WHITE);
      }
    }
  }
}

void drawEnemiesIn3DView(float playerViewCos, float playerViewSin) {
  constexpr int8_t vanishingPointXLeft = screenCenterX - 1;
  constexpr int8_t vanishingPointY = screenCenterY + horizonOffset;
  constexpr int8_t depth = 120;

  for (uint8_t i = 0; i < enemyCount; ++i) {
    float relX = worldXLimit - enemies[i].x - player.playerViewX;
    float relY = enemies[i].y - player.y;

    float wrappedX = wrapCoordinate(relX, worldXLimit);
    float wrappedY = wrapCoordinate(relY, worldYLimit);

    float rotatedX = wrappedX * playerViewCos + wrappedY * playerViewSin;
    float rotatedY = wrappedY * playerViewCos - wrappedX * playerViewSin;

    if (rotatedY <= 0) continue;

    int16_t screenX = screenCenterX + (int16_t)(rotatedX / rotatedY * depth);
    int16_t screenY = vanishingPointY + (int16_t)(100 / rotatedY);

    // Scale the square size based on distance
    int8_t squareSize = 4 / (rotatedY / 40.0f);

    if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
      arduboy.drawRect(screenX - squareSize / 2, screenY - squareSize / 2, squareSize, squareSize, WHITE);
    }
  }
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();
  arduboy.clear();
  
  float playerCos = cos(player.angle);
  float playerSin = sin(player.angle);
  float playerViewCos = cos(player.viewAngle);
  float playerViewSin = sin(player.viewAngle);

  draw3DView(playerViewCos, playerViewSin);
  drawEnemiesIn3DView(playerViewCos, playerViewSin);  // Draw enemies in 3D grid
  player.drawPlayerSprite();
  if (minimapEnabled) {  // Draw the minimap only if enabled
    drawMinimap();
    drawEnemiesOnMinimap();  // Draw enemies on minimap
  }
  handleInput(playerViewCos, playerViewSin, playerCos, playerSin);

  arduboy.display();
}

void debugDisplayPlayerInfo() {
  arduboy.setCursor(0, 0);
  arduboy.print(F("Player Info:"));
  arduboy.setCursor(0, 8);
  arduboy.print(F("X: "));
  arduboy.print(player.x, 2);
  arduboy.setCursor(0, 16);
  arduboy.print(F("Y: "));
  arduboy.print(player.y, 2);
  arduboy.setCursor(0, 24);
  arduboy.print(F("Angle: "));
  arduboy.print(player.angle, 2);

  arduboy.setCursor(0, 32);
  arduboy.print(F("Player Info:"));
  arduboy.setCursor(0, 40);
  arduboy.print(F("ViewX: "));
  arduboy.print(player.playerViewX, 2);
  arduboy.setCursor(0, 48);
  arduboy.print(F("ViewAngle: "));
  arduboy.print(player.viewAngle, 2);
}