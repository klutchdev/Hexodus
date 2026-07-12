/**********************************************************************************************
*
*   Hexodus Utils - small random helpers
*
*   NOTE: these use C rand() (seeded in main); raylib's GetRandomValue is a separate,
*   separately-seeded RNG
*
*   Built by Kyle aka Klutch @ quicks games
*
**********************************************************************************************/
#include "utils.hpp"

namespace UTILS
{
  Color getRandColor(const std::vector<Color> &colors)
  {
    int idx = rand() % colors.size();

    return colors[idx];
  }

  float getRandomFloat(float min, float max)
  {
    return min + (rand() / (float)RAND_MAX) * (max - min);
  }
}