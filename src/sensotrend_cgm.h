#pragma once

#include "pebble.h"

static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []) {
    { -4, 0 },
    { 4, 0 },
    { 4, -38 },
    { 0, -39 },
    { -4, -38 }
  }
};

static const GPathInfo MINUTE_HAND_POINTS = {
  5, (GPoint []) {
    { -3, 0 },
    { 3, 0 },
    { 3, -60 },
    { 0, -61 },
    { -3, -60 }
  }
};

