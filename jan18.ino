#include <Arduboy2.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "Trig.h"  // Assuming Trig.h is in the same directory
#include "GameSprites.h"

Arduboy2 arduboy;

const uint8_t worldWidth = 32;
const uint8_t worldHeight = 32;

const uint8_t cellSize = 4;

const uint8_t gridX = worldWidth / cellSize;   // Number of cells along the X-axis
const uint8_t gridY = worldHeight / cellSize;  // Number of cells along the Y-axis

bool debugEnabled = false;

bool minimapEnabled = true;  // New variable to track minimap state

// Define the Object struct
struct Object {
  SQ7x8 x;
  SQ7x8 y;
};

struct Player : public Object {
  uint8_t angle = 0;
  uint8_t viewAngle = 0;
  SQ7x8 moveSpeed = 0.1;
  uint8_t rotationSpeed = 1;
} player;

struct Enemy : public Object {
  SQ7x8 moveSpeed = 0.1;
  bool active = false;
  uint8_t scaledSize = 0;  // Add this field
};

// Initialize objects
Object gridCells[gridX * gridY];
const uint8_t enemyCount = 3;  // Example enemy count
Enemy enemies[enemyCount];     // Example enemy array

void handleInput() {
  if (arduboy.justPressed(A_BUTTON)) {
    debugEnabled = !debugEnabled;
    minimapEnabled = !minimapEnabled;
  }
  if (arduboy.pressed(B_BUTTON)) {
    //if (arduboy.justPressed(B_BUTTON)) {
    fireBullet();
  }
  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= player.rotationSpeed;      // Rotate left
    player.viewAngle += player.rotationSpeed;  // Rotate right
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += player.rotationSpeed;      // Rotate right
    player.viewAngle -= player.rotationSpeed;  // Rotate left
  }
  if (arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) {
    int8_t direction = (arduboy.pressed(UP_BUTTON) ? 1.0f : -1.0f);

    player.x += direction * player.moveSpeed * Cos(player.angle);  // For 2D minimap
    player.y += direction * player.moveSpeed * Sin(player.angle);  // For 2D minimap

    player.x = wrapCoordinate(player.x, worldWidth);
    player.y = wrapCoordinate(player.y, worldHeight);
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

void to3DView(Object object, void (*drawFunction)(uint8_t, uint8_t, SQ7x8)) {
  constexpr int8_t screenCenterX = WIDTH / 2;
  constexpr int8_t horizon = 10;
  constexpr int8_t depth = 120;
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

  if (distance <= minDistance || distance > maxDistance) return;

  if (abs(rotatedX / rotatedY) > maxAngleRatio) return;

  uint8_t screenX = screenCenterX + (uint8_t)(rotatedX / rotatedY * depth);
  uint8_t screenY = horizon + (uint8_t)(depth / rotatedY);

  if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
    (*drawFunction)(screenX, screenY, distance);
  }
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

int currentEnemyIndex = -1;  // Global index for the currently rendered enemy

void drawEnemy(uint8_t x, uint8_t y, SQ7x8 distance) {
  const uint8_t minEnemySize = 2;
  const uint8_t maxEnemySize = 120;
  const SQ7x8 minEnemyDistance = 0.5;
  const SQ7x8 maxEnemyDistance = 16;

  if (currentEnemyIndex >= 0 && currentEnemyIndex < enemyCount) {
    enemies[currentEnemyIndex].scaledSize = calculateSize(distance, minEnemySize, maxEnemySize, minEnemyDistance, maxEnemyDistance);
    ;
  }

  // Draw the enemy with the calculated size
  arduboy.drawRect(x - enemies[currentEnemyIndex].scaledSize / 2, y - enemies[currentEnemyIndex].scaledSize / 2, enemies[currentEnemyIndex].scaledSize, enemies[currentEnemyIndex].scaledSize, WHITE);

  // Choose a sprite based on the square size and draw it centered within the square
  const uint8_t* sprite;
  uint8_t spriteWidth, spriteHeight;
  if (enemies[currentEnemyIndex].scaledSize < 8) {
    sprite = enemy2x3;
    spriteWidth = 2;
    spriteHeight = 3;
  } else if (enemies[currentEnemyIndex].scaledSize < 15) {
    sprite = mech6x8;
    spriteWidth = 6;
    spriteHeight = 8;
  } else if (enemies[currentEnemyIndex].scaledSize < 20) {
    sprite = Battle_Cat8x11;
    spriteWidth = 8;
    spriteHeight = 11;
    //Add in between size here
  } else if (enemies[currentEnemyIndex].scaledSize < 30) {
    sprite = Battle_Cat16x21;
    spriteWidth = 16;
    spriteHeight = 21;
  } else if (enemies[currentEnemyIndex].scaledSize < 40) {
    sprite = Battle_Cat30x40;
    spriteWidth = 30;
    spriteHeight = 40;
  } else if (enemies[currentEnemyIndex].scaledSize < 60) {
    sprite = Battle_Cat50x67;
    spriteWidth = 50;
    spriteHeight = 67;
  } else if (enemies[currentEnemyIndex].scaledSize < 80) {
    sprite = Battle_Cat70x94;
    spriteWidth = 70;
    spriteHeight = 94;
  } else if (enemies[currentEnemyIndex].scaledSize < 100) {
    sprite = Battle_Cat90x121;
    spriteWidth = 90;
    spriteHeight = 121;
  } else {
    sprite = Battle_Cat110x148;
    spriteWidth = 110;
    spriteHeight = 148;
  }

  // int8_t spriteX = screenX - spriteWidth / 2;
  // int8_t spriteY = screenY - spriteHeight / 2;

  Sprites::drawSelfMasked(x - spriteWidth / 2, y - spriteWidth / 2, sprite, 0);
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
  int frontX = static_cast<int8_t>(player.x + Cos(player.angle) * triangleSize);
  int frontY = static_cast<int8_t>(player.y + Sin(player.angle) * triangleSize);
  int rearLeftX = static_cast<int8_t>(player.x + Cos(player.angle - triangleAngleOffset) * triangleSize);
  int rearLeftY = static_cast<int8_t>(player.y + Sin(player.angle - triangleAngleOffset) * triangleSize);
  int rearRightX = static_cast<int8_t>(player.x + Cos(player.angle + triangleAngleOffset) * triangleSize);
  int rearRightY = static_cast<int8_t>(player.y + Sin(player.angle + triangleAngleOffset) * triangleSize);

  arduboy.fillTriangle(frontX, frontY, rearLeftX, rearLeftY, rearRightX, rearRightY, WHITE);
}

void drawEnemiesOnMinimap() {
  for (uint8_t i = 0; i < enemyCount; ++i) {
    arduboy.drawRect((uint8_t)(enemies[i].x - 1), (uint8_t)(enemies[i].y - 1), 3, 3, WHITE);
  }
}

// Define the Bullet struct
struct Bullet : public Object {
  SQ7x8 speed = 0.05;
  uint8_t angle;
  bool active = false;
  uint8_t scaledSize = 0;  // Scaled size based on distance
  SQ7x8 lifetime = 0;
};

const uint8_t maxBullets = 1;  // Max number of bullets
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

int currentBulletIndex = -1;  // Global index for the currently rendered bullet

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
      to3DView(bullets[i], drawBullet);
    }
  }
  currentBulletIndex = -1;  // Reset index after rendering bullets
}

void checkBulletEnemyCollisions() {
  for (uint8_t i = 0; i < maxBullets; ++i) {
    if (bullets[i].active) {
      for (uint8_t j = 0; j < enemyCount; ++j) {
        if (enemies[j].active) {
          // Calculate squared distance between bullet and enemy
          SQ7x8 dx = bullets[i].x - enemies[j].x;
          SQ7x8 dy = bullets[i].y - enemies[j].y;
          SQ15x16 distanceSq = SQ15x16(dx * dx + dy * dy);

          // Calculate the collision radius based on scaled sizes
          uint8_t collisionRadius = bullets[i].scaledSize / 2 + enemies[j].scaledSize / 2;

          // Debug output
          if (debugEnabled) {
            arduboy.setCursor(0, 0);
            arduboy.print(F("Bullet: "));
            arduboy.print(bullets[i].scaledSize);
            arduboy.setCursor(0, 10);
            arduboy.print(F("Enemy: "));
            arduboy.print(enemies[j].scaledSize);
            arduboy.setCursor(0, 20);
            arduboy.print(F("DistSq: "));
            arduboy.print(static_cast<float>(distanceSq));
            arduboy.setCursor(0, 40);
            arduboy.print(F("Radius: "));
            arduboy.print(collisionRadius);
          }

          // Check for collision (using squared values)
          if (distanceSq < SQ15x16(collisionRadius * collisionRadius)) {
            // Handle collision (e.g., deactivate bullet and enemy)
            //bullets[i].active = false;
            //enemies[j].active = false;
            break;  // Exit inner loop if collision occurs
          }
        }
      }
    }
  }
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  // Initialize grid cells
  for (uint8_t x = 0; x < gridX; ++x) {
    for (uint8_t y = 0; y < gridY; ++y) {
      gridCells[x * gridY + y].x = x * cellSize;
      gridCells[x * gridY + y].y = y * cellSize;
    }
  }

  // Initialize enemies with example positions
  for (uint8_t i = 0; i < enemyCount; ++i) {
    enemies[i].active = true;
    enemies[i].x = random(worldWidth);   // Random X position
    enemies[i].y = random(worldHeight);  // Random Y position
  }
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }

  arduboy.clear();
  arduboy.pollButtons();
  handleInput();

  // Render each grid cell
  for (uint8_t i = 0; i < gridX * gridY; ++i) {
    to3DView(gridCells[i], drawGridCell);
  }

  // Render each enemy and update their rendered size
  for (uint8_t i = 0; i < enemyCount; ++i) {
    if (enemies[i].active) {
      currentEnemyIndex = i;
      to3DView(enemies[i], drawEnemy);
    }
  }
  currentEnemyIndex = -1;  // Reset index after rendering enemies

  updateBullets();               // Update bullet positions
  checkBulletEnemyCollisions();  // Check for bullet-enemy collisions
  renderBullets();               // Render bullets

  if (minimapEnabled) {
    drawMinimap();
    drawEnemiesOnMinimap();
  }

  arduboy.display();
}