#pragma once

/**** FX data header generated by fxdata-build.py tool version 1.15 ****/

using uint24_t = __uint24;

// Initialize FX hardware using  FX::begin(FX_DATA_PAGE); in the setup() function.

constexpr uint16_t FX_DATA_PAGE  = 0xfec0;
constexpr uint24_t FX_DATA_BYTES = 0;

constexpr uint16_t FX_SAVE_PAGE  = 0xfec0;
constexpr uint24_t FX_SAVE_BYTES = 81057;

constexpr uint24_t mothraStr = 0x000002;
constexpr uint24_t battleCatStr = 0x000009;
constexpr uint24_t enemiesStr = 0x000013;
constexpr uint24_t titleScreen128x64 = 0x00001B;
constexpr uint16_t titleScreen128x64Width  = 128;
constexpr uint16_t titleScreen128x64Height = 64;

constexpr uint24_t mainMenu128x64 = 0x00081F;
constexpr uint16_t mainMenu128x64Width  = 128;
constexpr uint16_t mainMenu128x64Height = 64;

constexpr uint24_t saveLoadMenu128x64 = 0x001023;
constexpr uint16_t saveLoadMenu128x64Width  = 128;
constexpr uint16_t saveLoadMenu128x64Height = 64;

constexpr uint24_t missionMenu128x64 = 0x001827;
constexpr uint16_t missionMenu128x64Width  = 128;
constexpr uint16_t missionMenu128x64Height = 64;

constexpr uint24_t hangerMenu128x64 = 0x00202B;
constexpr uint16_t hangerMenu128x64Width  = 128;
constexpr uint16_t hangerMenu128x64Height = 64;

constexpr uint24_t customizationMenu128x64 = 0x00282F;
constexpr uint16_t customizationMenu128x64Width  = 128;
constexpr uint16_t customizationMenu128x64Height = 64;

constexpr uint24_t leftArrowSmall = 0x003033;
constexpr uint16_t leftArrowSmallWidth  = 7;
constexpr uint16_t leftArrowSmallHeight = 13;

constexpr uint24_t rightArrowSmall = 0x003053;
constexpr uint16_t rightArrowSmallWidth  = 7;
constexpr uint16_t rightArrowSmallHeight = 13;

constexpr uint24_t mothraCockpit128x64 = 0x003073;
constexpr uint16_t mothraCockpit128x64Width  = 128;
constexpr uint16_t mothraCockpit128x64Height = 64;

constexpr uint24_t battleCatCockpit128x64 = 0x003877;
constexpr uint16_t battleCatCockpit128x64Width  = 128;
constexpr uint16_t battleCatCockpit128x64Height = 64;

constexpr uint24_t thorHammerCockpit128x64 = 0x00407B;
constexpr uint16_t thorHammerCockpit128x64Width  = 128;
constexpr uint16_t thorHammerCockpit128x64Height = 64;

constexpr uint24_t enemy2x3 = 0x00487F;
constexpr uint16_t enemy2x3Width  = 2;
constexpr uint16_t enemy2x3Height = 3;

constexpr uint24_t mech6x8 = 0x004887;
constexpr uint16_t mech6x8Width  = 6;
constexpr uint16_t mech6x8Height = 8;

constexpr uint24_t Mothra8x11 = 0x004897;
constexpr uint16_t Mothra8x11Width  = 8;
constexpr uint16_t Mothra8x11Height = 11;

constexpr uint24_t Mothra16x21 = 0x0048BB;
constexpr uint16_t Mothra16x21Width  = 16;
constexpr uint16_t Mothra16x21Height = 21;

constexpr uint24_t Mothra30x40 = 0x00491F;
constexpr uint16_t Mothra30x40Width  = 30;
constexpr uint16_t Mothra30x40Height = 42;

constexpr uint24_t Mothra50x67 = 0x004A8B;
constexpr uint16_t Mothra50x67Width  = 50;
constexpr uint16_t Mothra50x67Height = 70;

constexpr uint24_t Mothra70x94 = 0x004E13;
constexpr uint16_t Mothra70x94Width  = 70;
constexpr uint16_t Mothra70x94Height = 98;

constexpr uint24_t Mothra90x121 = 0x005533;
constexpr uint16_t Mothra90x121Width  = 90;
constexpr uint16_t Mothra90x121Height = 126;

constexpr uint24_t Mothra110x148 = 0x006077;
constexpr uint16_t Mothra110x148Width  = 110;
constexpr uint16_t Mothra110x148Height = 154;

constexpr uint24_t Battle_Cat8x11 = 0x0071AB;
constexpr uint16_t Battle_Cat8x11_width  = 8;
constexpr uint16_t Battle_Cat8x11_height = 11;

constexpr uint24_t Battle_Cat16x21 = 0x0071CF;
constexpr uint16_t Battle_Cat16x21_width  = 16;
constexpr uint16_t Battle_Cat16x21_height = 21;

constexpr uint24_t Battle_Cat30x40 = 0x007233;
constexpr uint16_t Battle_Cat30x40_width  = 30;
constexpr uint16_t Battle_Cat30x40_height = 40;

constexpr uint24_t Battle_Cat50x67 = 0x007363;
constexpr uint16_t Battle_Cat50x67_width  = 50;
constexpr uint16_t Battle_Cat50x67_height = 67;

constexpr uint24_t Battle_Cat70x94 = 0x0076EB;
constexpr uint16_t Battle_Cat70x94_width  = 70;
constexpr uint16_t Battle_Cat70x94_height = 94;

constexpr uint24_t Battle_Cat90x121 = 0x007D7F;
constexpr uint16_t Battle_Cat90x121_width  = 90;
constexpr uint16_t Battle_Cat90x121_height = 121;

constexpr uint24_t Battle_Cat110x148 = 0x0088C3;
constexpr uint16_t Battle_Cat110x148_width  = 110;
constexpr uint16_t Battle_Cat110x148_height = 148;

constexpr uint24_t explosion23x23 = 0x00991B;
constexpr uint16_t explosion23x23Width  = 23;
constexpr uint16_t explosion23x23Height = 23;
constexpr uint8_t  explosion23x23Frames = 7;

constexpr uint24_t explosion46x46 = 0x009CE5;
constexpr uint16_t explosion46x46Width  = 46;
constexpr uint16_t explosion46x46Height = 46;
constexpr uint8_t  explosion46x46Frames = 7;

constexpr uint24_t explosion92x92 = 0x00AC01;
constexpr uint16_t explosion92x92Width  = 92;
constexpr uint16_t explosion92x92Height = 92;
constexpr uint8_t  explosion92x92Frames = 7;

constexpr uint24_t explosion110x110 = 0x00E865;
constexpr uint16_t explosion110x110Width  = 110;
constexpr uint16_t explosion110x110Height = 110;
constexpr uint8_t  explosion110x110Frames = 7;

