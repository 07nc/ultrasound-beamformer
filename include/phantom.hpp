#pragma once

#include "common.hpp"

namespace us {
struct Scatterer {
  float x;
  float z;
  float amplitude;
};

void generate_phantom(RFData &rf);
} // namespace us
