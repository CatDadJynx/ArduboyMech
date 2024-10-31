#pragma once

/**** FX data header generated by fxdata-build.py tool version 1.15 ****/

using uint24_t = __uint24;

// Initialize FX hardware using  FX::begin(FX_DATA_PAGE); in the setup() function.

constexpr uint16_t FX_DATA_PAGE  = 0xfdb1;
constexpr uint24_t FX_DATA_BYTES = 147134;

constexpr uint16_t FX_SAVE_PAGE  = 0xfff0;
constexpr uint24_t FX_SAVE_BYTES = 2;

constexpr uint24_t playerMothraStats = 0x000000;
constexpr uint24_t playerBattleCatStats = 0x000006;
constexpr uint24_t playerThorHammerStats = 0x00000C;
//constexpr uint24_t availableWeapons = 0x000012;
constexpr uint24_t mainMenuItems = 0x000030;
constexpr uint24_t saveLoadMenuItems = 0x00003C;
constexpr uint24_t missionMenuItems = 0x000044;
constexpr uint24_t hangerMenuItems = 0x000050;
constexpr uint24_t customizationMenuItems = 0x000068;
constexpr uint24_t noSaveDataStr = 0x000078;
constexpr uint16_t noSaveDataStrWidth  = 50;
constexpr uint16_t noSaveDataStrHeight = 6;

constexpr uint24_t dayStr = 0x0000AE;
constexpr uint16_t dayStrWidth  = 20;
constexpr uint16_t dayStrHeight = 6;

constexpr uint24_t enemiesStr = 0x0000C6;
constexpr uint16_t enemiesStrWidth  = 40;
constexpr uint16_t enemiesStrHeight = 6;

constexpr uint24_t costStr = 0x0000F2;
constexpr uint16_t costStrWidth  = 25;
constexpr uint16_t costStrHeight = 6;

constexpr uint24_t dmgStr = 0x00010F;
constexpr uint16_t dmgStrWidth  = 20;
constexpr uint16_t dmgStrHeight = 6;

constexpr uint24_t pressAStr = 0x000127;
constexpr uint16_t pressAStrWidth  = 30;
constexpr uint16_t pressAStrHeight = 6;

constexpr uint24_t notEnoughMoneyStr = 0x000149;
constexpr uint16_t notEnoughMoneyStrWidth  = 75;
constexpr uint16_t notEnoughMoneyStrHeight = 6;

constexpr uint24_t AorBStr = 0x000198;
constexpr uint16_t AorBStrWidth  = 45;
constexpr uint16_t AorBStrHeight = 6;

constexpr uint24_t AyesBnoStr = 0x0001C9;
constexpr uint16_t AyesBnoStrWidth  = 45;
constexpr uint16_t AyesBnoStrHeight = 6;

constexpr uint24_t sellThisWeaponStr = 0x0001FA;
constexpr uint16_t sellThisWeaponStrWidth  = 75;
constexpr uint16_t sellThisWeaponStrHeight = 6;

constexpr uint24_t mothraStr = 0x000249;
constexpr uint16_t mothraStrWidth  = 30;
constexpr uint16_t mothraStrHeight = 6;

constexpr uint24_t battleCatStr = 0x00026B;
constexpr uint16_t battleCatStrWidth  = 45;
constexpr uint16_t battleCatStrHeight = 6;

constexpr uint24_t thorHammerStr = 0x00029C;
constexpr uint16_t thorHammerStrWidth  = 50;
constexpr uint16_t thorHammerStrHeight = 6;

constexpr uint24_t armorStr = 0x0002D2;
constexpr uint16_t armorStrWidth  = 25;
constexpr uint16_t armorStrHeight = 6;

constexpr uint24_t weightStr = 0x0002EF;
constexpr uint16_t weightStrWidth  = 30;
constexpr uint16_t weightStrHeight = 6;

constexpr uint24_t heatSinkStr = 0x000311;
constexpr uint16_t heatSinkStrWidth  = 40;
constexpr uint16_t heatSinkStrHeight = 6;

constexpr uint24_t heatStr = 0x00033D;
constexpr uint16_t heatStrWidth  = 25;
constexpr uint16_t heatStrHeight = 6;

constexpr uint24_t moveSpeedStr = 0x00035A;
constexpr uint16_t moveSpeedStrWidth  = 45;
constexpr uint16_t moveSpeedStrHeight = 6;

constexpr uint24_t creditsStr = 0x00038B;
constexpr uint16_t creditsStrWidth  = 35;
constexpr uint16_t creditsStrHeight = 6;

constexpr uint24_t overheatStr = 0x0003B2;
constexpr uint16_t overheatStrWidth  = 40;
constexpr uint16_t overheatStrHeight = 6;

constexpr uint24_t gameOverStr = 0x0003DE;
constexpr uint16_t gameOverStrWidth  = 40;
constexpr uint16_t gameOverStrHeight = 6;

constexpr uint24_t weaponSlotsStr = 0x00040A;
constexpr uint16_t weaponSlotsStrWidth  = 60;
constexpr uint16_t weaponSlotsStrHeight = 6;

constexpr uint24_t availableWeaponsStr = 0x00044A;
constexpr uint16_t availableWeaponsStrWidth  = 85;
constexpr uint16_t availableWeaponsStrHeight = 6;

constexpr uint24_t weaponsListStr = 0x0004A3;
constexpr uint16_t weaponsListStrWidth  = 55;
constexpr uint16_t weaponsListStrHeight = 6;

constexpr uint24_t lightMGStr = 0x0004DE;
constexpr uint16_t lightMGStrWidth  = 30;
constexpr uint16_t lightMGStrHeight = 6;

constexpr uint24_t heavyMGStr = 0x000500;
constexpr uint16_t heavyMGStrWidth  = 35;
constexpr uint16_t heavyMGStrHeight = 6;

constexpr uint24_t rocketStr = 0x000527;
constexpr uint16_t rocketStrWidth  = 30;
constexpr uint16_t rocketStrHeight = 6;

constexpr uint24_t medRocketStr = 0x000549;
constexpr uint16_t medRocketStrWidth  = 45;
constexpr uint16_t medRocketStrHeight = 6;

constexpr uint24_t laserStr = 0x00057A;
constexpr uint16_t laserStrWidth  = 25;
constexpr uint16_t laserStrHeight = 6;

constexpr uint24_t emptyStr = 0x000597;
constexpr uint16_t emptyStrWidth  = 25;
constexpr uint16_t emptyStrHeight = 6;

constexpr uint24_t font_4x6 = 0x0005B4;
constexpr uint16_t font_4x6_width  = 128;
constexpr uint16_t font_4x6_height = 64;

constexpr uint24_t font_3x5 = 0x0009B8;
constexpr uint16_t font_3x5_width  = 128;
constexpr uint16_t font_3x5_height = 64;

constexpr uint24_t availableWeaponsMenu90x60 = 0x000DBC;
constexpr uint16_t availableWeaponsMenu90x60Width  = 90;
constexpr uint16_t availableWeaponsMenu90x60Height = 60;

constexpr uint24_t weaponsStatsMenu50x40 = 0x001360;
constexpr uint16_t weaponsStatsMenu50x40Width  = 50;
constexpr uint16_t weaponsStatsMenu50x40Height = 40;

constexpr uint24_t notEnoughMoney86x20 = 0x00145E;
constexpr uint16_t notEnoughMoney86x20Width  = 86;
constexpr uint16_t notEnoughMoney86x20Height = 20;

constexpr uint24_t titleScreen128x64 = 0x001564;
constexpr uint16_t titleScreen128x64Width  = 128;
constexpr uint16_t titleScreen128x64Height = 64;

constexpr uint24_t mainMenu128x64 = 0x001D68;
constexpr uint16_t mainMenu128x64Width  = 128;
constexpr uint16_t mainMenu128x64Height = 64;

constexpr uint24_t saveLoadMenu128x64 = 0x00256C;
constexpr uint16_t saveLoadMenu128x64Width  = 128;
constexpr uint16_t saveLoadMenu128x64Height = 64;

constexpr uint24_t missionMenu128x64 = 0x002D70;
constexpr uint16_t missionMenu128x64Width  = 128;
constexpr uint16_t missionMenu128x64Height = 64;

constexpr uint24_t hangerMenu128x64 = 0x003574;
constexpr uint16_t hangerMenu128x64Width  = 128;
constexpr uint16_t hangerMenu128x64Height = 64;

constexpr uint24_t customizationMenu128x64 = 0x003D78;
constexpr uint16_t customizationMenu128x64Width  = 128;
constexpr uint16_t customizationMenu128x64Height = 64;

constexpr uint24_t leftArrowSmall = 0x00457C;
constexpr uint16_t leftArrowSmallWidth  = 7;
constexpr uint16_t leftArrowSmallHeight = 13;

constexpr uint24_t rightArrowSmall = 0x00459C;
constexpr uint16_t rightArrowSmallWidth  = 7;
constexpr uint16_t rightArrowSmallHeight = 13;

constexpr uint24_t mothraCockpit128x64 = 0x0045BC;
constexpr uint16_t mothraCockpit128x64Width  = 128;
constexpr uint16_t mothraCockpit128x64Height = 64;

constexpr uint24_t battleCatCockpit128x64 = 0x004DC0;
constexpr uint16_t battleCatCockpit128x64Width  = 128;
constexpr uint16_t battleCatCockpit128x64Height = 64;

constexpr uint24_t thorHammerCockpit128x64 = 0x0055C4;
constexpr uint16_t thorHammerCockpit128x64Width  = 128;
constexpr uint16_t thorHammerCockpit128x64Height = 64;

constexpr uint24_t enemy2x3 = 0x005DC8;
constexpr uint16_t enemy2x3Width  = 2;
constexpr uint16_t enemy2x3Height = 3;

constexpr uint24_t mech6x8 = 0x005DD0;
constexpr uint16_t mech6x8Width  = 6;
constexpr uint16_t mech6x8Height = 8;

constexpr uint24_t Mothra8x11 = 0x005DE0;
constexpr uint16_t Mothra8x11Width  = 8;
constexpr uint16_t Mothra8x11Height = 11;
constexpr uint8_t  Mothra8x11Frames = 8;

constexpr uint24_t Mothra16x21 = 0x005EE4;
constexpr uint16_t Mothra16x21Width  = 16;
constexpr uint16_t Mothra16x21Height = 21;
constexpr uint8_t  Mothra16x21Frames = 8;

constexpr uint24_t Mothra30x40 = 0x0061E8;
constexpr uint16_t Mothra30x40Width  = 30;
constexpr uint16_t Mothra30x40Height = 42;
constexpr uint8_t  Mothra30x40Frames = 8;

constexpr uint24_t Mothra50x67 = 0x006D2C;
constexpr uint16_t Mothra50x67Width  = 50;
constexpr uint16_t Mothra50x67Height = 70;
constexpr uint8_t  Mothra50x67Frames = 8;

constexpr uint24_t Mothra70x94 = 0x008950;
constexpr uint16_t Mothra70x94Width  = 70;
constexpr uint16_t Mothra70x94Height = 98;

constexpr uint24_t Mothra90x121 = 0x009070;
constexpr uint16_t Mothra90x121Width  = 90;
constexpr uint16_t Mothra90x121Height = 126;

constexpr uint24_t Mothra110x148 = 0x009BB4;
constexpr uint16_t Mothra110x148Width  = 110;
constexpr uint16_t Mothra110x148Height = 154;

constexpr uint24_t Battle_Cat8x11 = 0x00ACE8;
constexpr uint16_t Battle_Cat8x11_width  = 8;
constexpr uint16_t Battle_Cat8x11_height = 11;

constexpr uint24_t Battle_Cat16x21 = 0x00AD0C;
constexpr uint16_t Battle_Cat16x21_width  = 16;
constexpr uint16_t Battle_Cat16x21_height = 21;

constexpr uint24_t Battle_Cat30x40 = 0x00AD70;
constexpr uint16_t Battle_Cat30x40_width  = 30;
constexpr uint16_t Battle_Cat30x40_height = 40;
constexpr uint8_t  Battle_Cat30x40_frames = 8;

constexpr uint24_t Battle_Cat50x67 = 0x00B6D4;
constexpr uint16_t Battle_Cat50x67_width  = 50;
constexpr uint16_t Battle_Cat50x67_height = 67;
constexpr uint8_t  Battle_Cat50x67_frames = 8;

constexpr uint24_t Battle_Cat70x94 = 0x00D2F8;
constexpr uint16_t Battle_Cat70x94_width  = 70;
constexpr uint16_t Battle_Cat70x94_height = 94;
constexpr uint8_t  Battle_Cat70x94_frames = 8;

constexpr uint24_t Battle_Cat90x121 = 0x01077C;
constexpr uint16_t Battle_Cat90x121_width  = 90;
constexpr uint16_t Battle_Cat90x121_height = 121;

constexpr uint24_t Battle_Cat110x148 = 0x0112C0;
constexpr uint16_t Battle_Cat110x148_width  = 110;
constexpr uint16_t Battle_Cat110x148_height = 148;

constexpr uint24_t Thor_Hammer8x11 = 0x012318;
constexpr uint16_t Thor_Hammer8x11_width  = 8;
constexpr uint16_t Thor_Hammer8x11_height = 11;
constexpr uint8_t  Thor_Hammer8x11_frames = 8;

constexpr uint24_t Thor_Hammer16x21 = 0x01241C;
constexpr uint16_t Thor_Hammer16x21_width  = 16;
constexpr uint16_t Thor_Hammer16x21_height = 21;
constexpr uint8_t  Thor_Hammer16x21_frames = 8;

constexpr uint24_t Thor_Hammer30x40 = 0x012720;
constexpr uint16_t Thor_Hammer30x40_width  = 30;
constexpr uint16_t Thor_Hammer30x40_height = 39;
constexpr uint8_t  Thor_Hammer30x40_frames = 8;

constexpr uint24_t Thor_Hammer50x67 = 0x013084;
constexpr uint16_t Thor_Hammer50x67_width  = 50;
constexpr uint16_t Thor_Hammer50x67_height = 65;
constexpr uint8_t  Thor_Hammer50x67_frames = 8;

constexpr uint24_t Thor_Hammer70x94 = 0x014CA8;
constexpr uint16_t Thor_Hammer70x94_width  = 70;
constexpr uint16_t Thor_Hammer70x94_height = 90;
constexpr uint8_t  Thor_Hammer70x94_frames = 8;

constexpr uint24_t Thor_Hammer90x121 = 0x01812C;
constexpr uint16_t Thor_Hammer90x121_width  = 90;
constexpr uint16_t Thor_Hammer90x121_height = 116;

constexpr uint24_t Thor_Hammer110x148 = 0x018BBC;
constexpr uint16_t Thor_Hammer110x148_width  = 110;
constexpr uint16_t Thor_Hammer110x148_height = 142;

constexpr uint24_t explosion23x23 = 0x019B38;
constexpr uint16_t explosion23x23Width  = 23;
constexpr uint16_t explosion23x23Height = 23;
constexpr uint8_t  explosion23x23Frames = 7;

constexpr uint24_t explosion46x46 = 0x019F02;
constexpr uint16_t explosion46x46Width  = 46;
constexpr uint16_t explosion46x46Height = 46;
constexpr uint8_t  explosion46x46Frames = 7;

constexpr uint24_t explosion92x92 = 0x01AE1E;
constexpr uint16_t explosion92x92Width  = 92;
constexpr uint16_t explosion92x92Height = 92;
constexpr uint8_t  explosion92x92Frames = 7;

constexpr uint24_t explosion110x110 = 0x01EA82;
constexpr uint16_t explosion110x110Width  = 110;
constexpr uint16_t explosion110x110Height = 110;
constexpr uint8_t  explosion110x110Frames = 7;

