/**
 * @file battle/random.cpp
 * @brief PCG32 random number generator implementation
 *
 * Reference implementation from https://www.pcg-random.org/
 * PCG32 (Permuted Congruential Generator) - excellent statistical quality
 * with minimal code size (~100 bytes) and state (16 bytes).
 */

#include "random.hpp"

// Platform-specific entropy source
#ifdef _EZ80
#include <sys/rtc.h>
// TI-84 CE: Use RTC for entropy
#define GET_ENTROPY_SEED() rtc_Time()
#else
#include <chrono>
// Host: Use std::chrono for entropy
#define GET_ENTROPY_SEED() \
    static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count())
#endif

namespace battle {
namespace random {

// PCG32 state (64-bit state + 64-bit increment)
// Reference defaults from PCG32_INITIALIZER
static uint64_t g_state = 0x853c49e6748fea9bULL;
static uint64_t g_inc = 0xda3e39cb94b95bdbULL;

/**
 * @brief PCG32 algorithm (internal)
 * @return Random uint32_t
 *
 * PCG XSH RR 64/32 variant:
 * - 64-bit LCG state
 * - XOR shift + rotate output transformation
 * - Period: 2^64
 */
static uint32_t PCG32_Next() {
    uint64_t oldstate = g_state;
    // LCG step: state = state * multiplier + increment
    g_state = oldstate * 6364136223846793005ULL + g_inc;

    // Output permutation (XSH RR):
    // - XOR high and low bits, shift right
    // - Rotate by top bits for final mixing
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void Initialize(uint32_t seed) {
    // Default to platform-specific entropy if seed is 0
    if (seed == 0) {
        seed = GET_ENTROPY_SEED();
    }

    // Seeding algorithm from pcg32_srandom_r()
    // Uses two-step initialization for proper state mixing
    g_state = 0U;
    g_inc = ((uint64_t)seed << 1u) | 1u;  // Ensure increment is odd
    PCG32_Next();                         // First iteration
    g_state += seed;                      // Mix in seed
    PCG32_Next();                         // Second iteration for avalanche
}

uint16_t Random(uint16_t max) {
    if (max == 0)
        return 0;

    // Simple modulo (could be replaced with bounded rand for perfect uniformity)
    // Bias ≈ (2^32 mod bound) / 2^32   : Random(100) = 96/4294967296 ≈ 0.0000022%
    //                                  : Random(2^N) = 0
    // --> should be orders of magnitidue smaller than EZ80 hardware measurement error
    return PCG32_Next() % max;
}

}  // namespace random
}  // namespace battle
