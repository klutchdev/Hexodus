/**********************************************************************************************
*
*   Hexodus Utils - small random helpers - Declarations
*
*   Built by Kyle aka Klutch @ quicks games
*
**********************************************************************************************/
#pragma once
#include "raylib.h"
#include <ctime>
#include <vector>
#include <cstdlib>

namespace UTILS
{
  Color getRandColor(const std::vector<Color> &colors);

  float getRandomFloat(float min, float max);
  
}