# Archived TI-84 CE Test Harness

This directory contains the original test framework that validated the battle engine concept ladder on TI-84 CE hardware via CEmu emulator.

## Historical Significance

**Purpose:** Proved architecture stability through vertical slice development methodology

**Test Coverage:**
- **Foundation tests (440 tests):** Concept ladder moves that validated core architecture
- **Status tests (46 tests):** Comprehensive paralysis mechanics

**Total:** 486 tests validating battle engine on actual hardware constraints

## Status

**Archived:** 2024-11-21
**Commit:** d7390ff (feat: implement PCG32 random number generator)

These tests are **no longer maintained**. The test suite has been migrated to GTest for:
- Unlimited test capacity (no RAM constraints)
- Cross-platform testing (host + TI-84 CE validation)
- CI/CD integration
- Industry-standard tooling

## Test Organization

```
archived/
├── framework.hpp       # Custom lightweight test framework for TI-84 CE
├── framework.cpp       # Framework implementation
├── main.cpp            # Test runner
└── ti84ce/
    ├── foundation/     # 440 tests (concept ladder)
    │   ├── test_tackle.cpp
    │   ├── test_ember.cpp
    │   ├── test_stat_modify.cpp
    │   ├── test_multi_hit.cpp
    │   ├── test_protect.cpp
    │   ├── test_solar_beam.cpp
    │   ├── test_fly.cpp
    │   ├── test_substitute.cpp
    │   └── test_baton_pass.cpp
    └── status/         # 46 tests (paralysis mechanics)
        ├── test_paralysis_speed.cpp
        ├── test_paralysis_turn_skip.cpp
        ├── test_paralysis_immunity.cpp
        └── test_paralysis_integration.cpp
```

## Build Instructions

**These tests cannot be built** due to changed file paths after archiving.

The tests are preserved as **source code reference only**, not for execution. They use relative include paths (`../../../src/`) that were valid from the original `test/unit/` location but are broken after moving to `test/archived/ti84ce/`.

To understand the test patterns and convert them to GTest, read the source files directly.

## Lessons Learned

1. **RAM constraints force good architecture** - Binary splits at 136KB created natural domain boundaries
2. **Hardware testing is essential** - Caught platform-specific issues that host tests would miss
3. **Vertical slices work** - Concept ladder approach proved architecture flexibility
4. **Test-first development** - All 486 tests passed on hardware before declaring features complete

## Migration to GTest

Current test suite: `test/unit/` (GTest-based)

Key differences:
- **Host execution:** Tests run natively on development machine
- **Unlimited capacity:** No RAM constraints
- **Better assertions:** `EXPECT_EQ`, `EXPECT_NEAR`, matchers
- **CI integration:** GitHub Actions, pre-commit hooks

## Reference

For understanding the development philosophy that led to these tests, see:
- `.claude/docs/development/development-philosophy.md`
- `.claude/docs/architecture/design-decisions.md` (ADR-001: Walking Skeleton)
- `.claude/docs/development/test-organization.md`
