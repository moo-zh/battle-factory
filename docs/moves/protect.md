# Effect: PROTECT (Protect)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_PROTECT` (pokeemerald: 111)
**Introduced:** Generation II
**Example Moves:** Protect, Detect, Endure
**Category:** Status (Self-targeting protection)

---

## Overview

Protect is a **protection move** that blocks most incoming attacks with degrading success rate on consecutive uses. When successfully used, the user enters a "protected" state for that turn, causing most moves targeting them to fail.

**Key Concept:** Protection mechanics with success rate degradation

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_PROTECT] = {
    .effect = EFFECT_PROTECT,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 0,  // Self-targeting, cannot miss
    .pp = 10,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,
    .priority = 4,  // +4 priority
}
```

### Success Rate Formula

The success rate **degrades with consecutive successful uses**:

```
Success Rate = 100% / (2^n)
```

where `n` is the number of consecutive successful Protects.

| Consecutive Uses | Success Rate | Calculation |
|-----------------|--------------|-------------|
| First use       | 100%         | 100 / 2^0 = 100/1 |
| Second use      | 50%          | 100 / 2^1 = 100/2 |
| Third use       | 25%          | 100 / 2^2 = 100/4 |
| Fourth use      | 12.5%        | 100 / 2^3 = 100/8 |
| Fifth+ use      | ~6.25%       | 100 / 2^4 = 100/16 |

### Execution Steps

```
1. Calculate success_rate = 100 / (2^protect_count)
2. Roll RNG: random(100)
3. If roll < success_rate:
   - Set is_protected = true
   - Increment protect_count
   - Move succeeds
4. Else:
   - Reset protect_count = 0
   - Set move_failed = true
```

---

## State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon (user) | `is_protected` | Set to true (if successful) |
| Pokemon (user) | `protect_count` | Incremented (if successful) or reset (if failed) |

---

## Protection Behavior

### What Protect Blocks:
- All damaging moves (Tackle, Ember, Solar Beam, etc.)
- Most status moves targeting the opponent (Thunder Wave, Growl, Fake Tears, etc.)

### What Protect Does NOT Block:
- Self-targeting moves (Swords Dance, Agility) - don't target the protected Pokemon
- Status conditions already applied (burn damage, poison ticking)
- Field effects (weather, entry hazards)

---

## Counter Reset Conditions

The `protect_count` resets to 0 when:
- The user successfully uses any move OTHER than Protect/Detect/Endure
- Protect fails due to low success rate
- The user switches out (volatile state cleared)

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:115` - EFFECT_PROTECT definition (effect 111)
- `data/battle_scripts_1.s:1011` - BattleScript_EffectProtect implementation
- `src/battle_script_commands.c` - Cmd_protectaffects (success rate calculation)

**Key Mechanics:**
- Uses `gProtectStructs[battler].protected` flag (volatile per-turn state)
- Success rate formula: `100 / (1 << protect_count)`
- +4 priority ensures Protect usually goes first
- Self-targeting (MOVE_TARGET_USER)

---

## Implementation Notes

### State Tracking (Pokemon struct):
```cpp
bool is_protected;      // Volatile flag: protected this turn
uint8_t protect_count;  // Consecutive successful Protect uses
```

### Command Integration:
- `AccuracyCheck`: After normal accuracy check, if defender is protected → fail move
- `ModifyStatStage`: If targeting opponent and they're protected → fail move
- Self-targeting moves bypass protection check (affects_user = true)

### Turn Management:
- `is_protected` is **volatile** - cleared at start of each turn
- `protect_count` persists across turns until reset
- Each Pokemon tracks protection state independently

---

## Example Scenarios

### Scenario 1: Basic Protection
```
Turn 1:
  Charmander uses Protect (protect_count: 0)
    → Success rate: 100%, rolls 45 → succeeds
    → is_protected = true, protect_count = 1

  Bulbasaur uses Tackle on Charmander
    → AccuracyCheck sees is_protected = true
    → Move fails, no damage dealt
```

### Scenario 2: Consecutive Use Degradation
```
Turn 1: Protect (count: 0 → 1, rate: 100%) → succeeds
Turn 2: Protect (count: 1, rate: 50%) → rolls 23 → succeeds (count: 2)
Turn 3: Protect (count: 2, rate: 25%) → rolls 67 → fails (count: 0)
```

### Scenario 3: Counter Reset
```
Turn 1: Protect (count: 0 → 1)
Turn 2: Tackle (count: 1 → 0, reset on other move)
Turn 3: Protect (count: 0 → 1, back to 100%)
```

---

## Related Effects

- `EFFECT_DETECT` - Same as Protect (Fighting type)
- `EFFECT_ENDURE` - Similar but allows user to survive with 1 HP instead of blocking
- `EFFECT_KINGS_SHIELD` - Gen VI+, Protect variant that lowers Attack

---

## Test Coverage

- ✅ First use always succeeds (100%)
- ✅ Second use has ~50% success rate (statistical test)
- ✅ Third use has ~25% success rate (statistical test)
- ✅ Blocks damaging moves
- ✅ Blocks status moves
- ✅ Blocks stat modification moves
- ✅ Does NOT block self-targeting moves
- ✅ Counter resets on other move use
- ✅ Counter resets on failure
- ✅ Protection clears each turn
- ✅ Independent per Pokemon
