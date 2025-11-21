# Effect: SOLAR_BEAM (Solar Beam)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SOLAR_BEAM` (pokeemerald: 151)
**Introduced:** Generation I
**Example Moves:** Solar Beam, Skull Bash, Sky Attack
**Category:** Damage (Two-turn move)

---

## Overview

Solar Beam is a **two-turn move** that requires a charging turn before dealing damage. On the first turn, the user charges energy (consuming PP). On the second turn, the move executes with high power. This is the first implementation of the two-turn move mechanic.

**Special Case:** In sunny weather (Sunny Day active), Solar Beam skips the charging turn and executes immediately.

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_SOLAR_BEAM] = {
    .effect = EFFECT_SOLAR_BEAM,
    .power = 120,
    .type = TYPE_GRASS,
    .accuracy = 100,
    .pp = 10,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_SELECTED,
    .priority = 0,
}
```

### Battle Script Flow

**Turn 1 (Charging):**
1. Check if STATUS2_MULTIPLETURNS is set → if yes, go to Turn 2
2. If not charging yet:
   - Set STATUS2_MULTIPLETURNS flag
   - Set HITMARKER_CHARGING
   - Display message: "[Pokémon] absorbed light!"
   - Consume PP
   - End turn (no damage dealt)

**Turn 2 (Attack):**
1. STATUS2_MULTIPLETURNS is already set → proceed to attack
2. Clear STATUS2_MULTIPLETURNS flag
3. Execute normal damage calculation (AccuracyCheck → CalculateDamage → ApplyDamage)
4. PP is NOT consumed (HITMARKER_NO_PPDEDUCT set)

**Sunny Weather Shortcut:**
- If weather is B_WEATHER_SUN (and no Cloud Nine/Air Lock ability):
  - Skip charging turn entirely
  - Set HITMARKER_CHARGING marker
  - Consume PP
  - Execute attack immediately (same turn)

---

## State Tracking

Solar Beam uses the **STATUS2_MULTIPLETURNS** volatile status flag:

```c
#define STATUS2_MULTIPLETURNS (1 << 12)  // pokeemerald: include/constants/battle.h
```

This flag:
- Is set when the move begins charging (Turn 1)
- Persists across turns (volatile state, not saved)
- Is cleared when the move executes (Turn 2)
- Is shared by all two-turn moves (Razor Wind, Sky Attack, Skull Bash, Fly, Dig, etc.)

---

## Implementation Requirements

### New Pokemon State Fields

Add to `battle::state::Pokemon`:

```cpp
// Two-turn move state
bool is_charging;          // Volatile flag: currently charging a two-turn move
domain::Move charging_move; // Which move is being charged (for move validation)
```

### Effect Implementation

**Effect_SolarBeam(BattleContext& ctx):**

```
Turn 1 Logic:
1. Check if attacker->is_charging is false
   - If false: Begin charging
     - Set attacker->is_charging = true
     - Set attacker->charging_move = Move::SolarBeam
     - Set ctx.move_failed = false (move succeeded in starting)
     - Return (end turn, no damage)

Turn 2 Logic:
2. Check if attacker->is_charging is true
   - If true: Execute attack
     - Clear attacker->is_charging = false
     - Call AccuracyCheck
     - Call CalculateDamage
     - Call ApplyDamage
     - Call CheckFaint
```

**Sunny Weather Optimization (Future):**
- Skip charging if weather == WEATHER_SUN
- Execute attack immediately
- Note: Weather system not implemented yet, so defer this

---

## Test Cases

### Basic Two-Turn Behavior

1. **Turn 1: Charging**
   - Solar Beam starts charging
   - No damage dealt to defender
   - ctx.move_failed = false (move succeeded)
   - attacker->is_charging = true

2. **Turn 2: Attack**
   - Solar Beam executes after charging
   - Damage dealt to defender
   - attacker->is_charging = false (charging cleared)
   - ctx.damage_dealt > 0

3. **No Damage on Turn 1**
   - Defender HP unchanged after Turn 1
   - Defender HP decreased after Turn 2

### Edge Cases

4. **Interrupted Charging**
   - If attacker faints during Turn 1, charging is lost
   - If attacker switches out, charging is reset

5. **Accuracy Check Only on Turn 2**
   - Accuracy is checked when attack executes (Turn 2)
   - Not checked during charging turn (Turn 1)

6. **Miss After Charging**
   - If Solar Beam misses on Turn 2, charging is still consumed
   - No damage dealt, is_charging cleared

7. **Protection During Turn 2**
   - If defender uses Protect on Turn 2, Solar Beam is blocked
   - Charging is consumed, no damage dealt

8. **Cannot Change Move**
   - Once charging begins, the Pokemon is locked into Solar Beam
   - (This may require Engine-level logic, defer for now)

### Stat Interactions

9. **Critical Hits**
   - Solar Beam can crit on Turn 2 (normal damage calculation)

10. **Type Effectiveness**
    - Grass-type move, double damage vs Water/Ground/Rock
    - Half damage vs Fire/Grass/Dragon/Bug

11. **Stat Stages Apply on Turn 2**
    - Sp. Attack stages affect damage calculation on Turn 2

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:155` - EFFECT_SOLAR_BEAM definition
- `data/battle_scripts_1.s:2314` - BattleScript_EffectSolarBeam
- `data/battle_scripts_1.s:785-803` - Charging turn and attack turn logic
- `src/data/battle_moves.h:991-999` - Solar Beam move data

**Key Mechanics:**
- STATUS2_MULTIPLETURNS flag (bit 12 of status2)
- HITMARKER_CHARGING flag
- HITMARKER_NO_PPDEDUCT on Turn 2
- B_MSG_TURN1_SOLAR_BEAM message ID
- Sunny weather check: B_WEATHER_SUN
- Cloud Nine/Air Lock ability bypass

---

## Notes

- This is the **first two-turn move** implementation
- Establishes pattern for Razor Wind, Sky Attack, Fly, Dig, Bounce, etc.
- Sunny weather interaction deferred until weather system implemented
- Move-locking (can't switch moves mid-charge) may require Engine changes
- PP consumption: Turn 1 only (Turn 2 uses HITMARKER_NO_PPDEDUCT)

---

## Implementation Checklist

- [x] Add is_charging and charging_move fields to Pokemon struct
- [x] Write failing tests for all test cases
- [x] Implement Effect_SolarBeam with two-turn logic
- [x] Initialize new fields in test helpers
- [x] Verify integration with existing commands (AccuracyCheck, CalculateDamage, etc.)
- [x] Add Solar Beam to Move enum and MOVE_DATABASE
- [x] Add Effect_SolarBeam to EFFECT_DISPATCH table
- [x] All tests passing (376/376)
- [x] Mark specification as implemented

**Implementation Status:** ✅ Complete (2025-11-21)
