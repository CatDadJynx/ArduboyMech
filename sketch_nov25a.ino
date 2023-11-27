#include <Arduboy2.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "Trig.h"  // Assuming Trig.h is in the same directory

Arduboy2 arduboy;
const SQ7x8 playerSpeed = 0.5;
const uint8_t rotationSpeed = 1;

const uint8_t worldWidth = 32;
const uint8_t worldHeight = 32;

const uint8_t cellSize = 4;

const uint8_t gridX = worldWidth / cellSize;       // Number of cells along the X-axis
const uint8_t gridY = worldHeight / cellSize;      // Number of cells along the Y-axis

bool debugEnabled = false;

struct Player {
  SQ7x8 x, y;     // Position
  uint8_t angle;  // Direction the player is facing (in binary radians)
} player;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  //Serial.begin(9600);

  player.x = (worldWidth - cellSize) / 2;
  player.y = (worldHeight - cellSize) / 2;
  player.angle = 0;  // Facing right initially
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();
  arduboy.clear();

  handleInput();
  drawMinimap();
  draw3DGrid();
  debugInfo();
  arduboy.display();
}

void drawMinimap() {
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

void handleInput() {
  if (arduboy.justPressed(A_BUTTON)) {
    debugEnabled = !debugEnabled;
  }

  if (arduboy.pressed(LEFT_BUTTON)) {
    player.angle -= rotationSpeed;  // Rotate left
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    player.angle += rotationSpeed;  // Rotate right
  }
  /*if (arduboy.pressed(UP_BUTTON)) {
    player.x += Cos(player.angle) * playerSpeed;
    player.y += Sin(player.angle) * playerSpeed;

    player.x = wrapCoordinate(player.x, worldWidth);
    player.y = wrapCoordinate(player.y, worldHeight);
  }*/
}

void debugInfo() {
  if (debugEnabled) {
    debugDisplayPlayerInfo();
  }
}

void debugDisplayPlayerInfo() {
  arduboy.setCursor(0, 0);
  arduboy.print(F("Player Info:"));
  arduboy.setCursor(0, 8);
  arduboy.print(F("X: "));
  arduboy.print(static_cast<float>(player.x));
  arduboy.setCursor(0, 16);
  arduboy.print(F("Y: "));
  arduboy.print(static_cast<float>(player.y));
  arduboy.setCursor(0, 24);
  arduboy.print(F("Angle: "));
  arduboy.print(static_cast<float>(player.angle));
}

SQ7x8 wrapCoordinate(SQ7x8 coordinate, SQ7x8 max) {
  if (coordinate < 0) {
    return max + coordinate;
  } else if (coordinate >= max) {
    return coordinate - max;
  }
  return coordinate;
}

void draw3DGrid() {
  constexpr int8_t horizonOffset = -20;
  constexpr int8_t screenCenterX = WIDTH / 2;
  constexpr int8_t screenCenterY = HEIGHT / 2;
  constexpr int8_t vanishingPointY = screenCenterY + horizonOffset;
  constexpr int8_t depth = 120;

  for (int x = 0; x < gridX; ++x) {
    for (int y = 0; y < gridY; ++y) {
      SQ7x8 worldX = x * cellSize;
      SQ7x8 worldY = y * cellSize;

      SQ7x8 relX = worldX - player.x;
      SQ7x8 relY = worldY - player.y;

      SQ7x8 rotatedX = relX * Cos(player.angle) + relY * Sin(player.angle);
      SQ7x8 rotatedY = relY * Cos(player.angle) - relX * Sin(player.angle);

      if (rotatedY <= 0) continue; // Don't draw points behind the player

      int screenX = screenCenterX + (int)(rotatedX / rotatedY * depth);
      int screenY = vanishingPointY + (int)(depth / rotatedY);

      if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
        arduboy.drawPixel(screenX, screenY, WHITE);
      }
    }
  }
}