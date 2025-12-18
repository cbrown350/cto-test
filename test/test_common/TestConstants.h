#ifndef TEST_CONSTANTS_H
#define TEST_CONSTANTS_H

#include <cstdint>

namespace TestConstants {

// Timeouts / durations used across tests. Keep these small to ensure test suite completes quickly.
static const uint32_t kShortDurationSeconds = 5;
static const uint32_t kMediumDurationSeconds = 60;

// Common numeric tolerances
static const double kTimeToleranceMinutes = 15.0;
static const float kFloatEpsilon = 1e-3f;

// Locations for sunrise/sunset tests
static const double kNYCLat = 40.7128;
static const double kNYCLon = -74.0060;

static const double kLondonLat = 51.5074;
static const double kLondonLon = -0.1278;

} // namespace TestConstants

#endif // TEST_CONSTANTS_H
