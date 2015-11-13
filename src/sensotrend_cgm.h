#pragma once

#include "pebble.h"

static const GPathInfo MINUTE_HAND_POINTS = {
  5, (GPoint []) {
    { -3, 3 },
    { 3, 3 },
    { 2, -75 },
    { 0, -80 },
    { -2, -75 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []) {
    { -3, 3 },
    { 3, 3 },
    { 3, -35 },
    { 0, -45 },
    { -3, -35 }
  }
};
