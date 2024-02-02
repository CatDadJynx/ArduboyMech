#include <Arduboy2.h>
#include <ArduboyFX.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "Trig.h"    // Assuming Trig.h is in the same directory
#include "fxdata.h"  // this file contains all the references to FX data

Arduboy2 arduboy;

enum MechType : uint8_t {
  LIGHT,
  MEDIUM,
  HEAVY
};

enum MechStatus : uint8_t {
  NORMAL,
  OVERHEAT,
  LEG_DAMAGE,
  ARM_DAMAGE
};

enum EnemyState : uint8_t {
  Active,
  Inactive,
  Exploding
};

enum GameState : uint8_t {
    MAIN_MENU,
    INIT_GAME,
    //NEW_GAME,
    //LOAD_GAME,
    GAME_PLAY,
    GAME_OVER
};

GameState currentState = MAIN_MENU;

// Define the Object struct
struct Object {
  SQ7x8 x;
  SQ7x8 y;
};

struct Player : public Object {
  uint8_t angle = 0;
  uint8_t viewAngle = 0;
  SQ7x8 moveSpeed = 0;
  uint8_t rotationSpeed = 0;
  uint8_t health = 0;
  uint8_t bulletDamage = 0;
  // Eventually add enum for 'MechType' (Light, Medium, Heavy) and use this to populate mech stats
  MechType mechType;
  MechStatus status;
  const uint24_t* cockpitSprite;
} player;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  
  FX::begin(FX_DATA_PAGE);  // // initialise FX chip
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }

  arduboy.clear();
  arduboy.pollButtons();

  switch (currentState) {
    case MAIN_MENU:
      FX::drawBitmap(0, 0, MainMenu128x64, 0, dbmMasked);
      if (arduboy.justPressed(A_BUTTON)) {
        //currentState = NEW_GAME;
        player.mechType = LIGHT;
        currentState = INIT_GAME;
      }
      if (arduboy.justPressed(B_BUTTON)) {
        //currentState = LOAD_GAME;
        player.mechType = MEDIUM;
        currentState = INIT_GAME;
      }
      if (arduboy.justPressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
        player.mechType = HEAVY;
        currentState = INIT_GAME;
      }
      break;
    case INIT_GAME:
      if (player.mechType == LIGHT) {
        player.moveSpeed = 0.3;
        player.rotationSpeed = 3;
        player.health = 100;
        player.status = NORMAL;
        player.cockpitSprite = mothraCockpit128x64;
      }
      if (player.mechType == MEDIUM) {
        player.moveSpeed = 0.2;
        player.rotationSpeed = 2;
        player.health = 150;
        player.status = NORMAL;
        player.cockpitSprite = battleCatCockpit128x64;
      }
      if (player.mechType == HEAVY) {
        player.moveSpeed = 0.1;
        player.rotationSpeed = 1;
        player.health = 200;
        player.status = NORMAL;
        player.cockpitSprite = thorHammerCockpit128x64;
      }
      currentState = GAME_PLAY;
      break;
    case GAME_PLAY:
      break;
    //case LOAD_GAME:
      // Case for loading game
    //  break;
    case GAME_OVER:
      // Code for game over
      break;
  }

  FX::display(CLEAR_BUFFER);
}
