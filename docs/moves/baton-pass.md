# Effect: BATON_PASS (Baton Pass)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_BATON_PASS` (pokeemerald: 127)
**Introduced:** Generation II
**Example Moves:** Baton Pass
**Category:** Status (Transfers stat changes to switched-in Pokemon)

---

## Overview

Baton Pass is a **status move** that allows the user to switch out while **transferring stat stage changes** to the incoming Pokemon. This is the signature mechanic of Baton Pass chains in competitive Pokemon.

**Key Mechanics:**
- User switches out (like a normal switch)
- Stat stages (ATK, DEF, SPEED, SPATK, SPDEF, ACC, EVASION) are **transferred** to the switched-in Pokemon
- Volatile status effects like substitute, Aqua Ring, Ingrain are also transferred (not implemented in this version)

**Simplified Implementation:**
Since we don't have a full party/switching system yet, this implementation focuses on the **core mechanic**: copying stat stages from one Pokemon to another. This establishes the pattern for when switching is fully implemented.

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_BATON_PASS] = {
    .effect = EFFECT_BATON_PASS,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 0,  // Never misses (self-targeting)
    .pp = 40,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,
    .priority = 0,
    .flags = 0,
}
```

### Battle Script Flow (pokeemerald)

**Full Implementation (with switching):**
1. Check if can switch (not in Arena, not trapped)
2. Open party screen for player to select Pokemon
3. Switch out current Pokemon (run switch-out abilities)
4. **Transfer stat stages to incoming Pokemon**
5. Switch in new Pokemon (run switch-in abilities/effects)

**Our Simplified Implementation:**
1. Copy all stat stages from attacker to defender
2. This simulates the "passing" of stats that would occur during a switch
3. Establishes the core mechanic for future full implementation

---

## Implementation Requirements

### Simplified Implementation

For this vertical slice, Baton Pass **copies stat stages** from the user (attacker) to a target Pokemon (defender in tests, or incoming Pokemon in full implementation).

**Effect_BatonPass(BattleContext& ctx):**

```cpp
void Effect_BatonPass(BattleContext& ctx) {
    // Copy all stat stages from attacker to defender
    // In full implementation, defender would be the incoming Pokemon
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        ctx.defender->stat_stages[i] = ctx.attacker->stat_stages[i];
    }

    ctx.move_failed = false;  // Always succeeds
}
```

### Future Full Implementation

When switching is implemented:

```cpp
void Effect_BatonPass(BattleContext& ctx) {
    // 1. Validate can switch
    if (!CanSwitch(ctx.attacker)) {
        ctx.move_failed = true;
        return;
    }

    // 2. Save stat stages to transfer
    int8_t stat_stages[NUM_BATTLE_STATS];
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        stat_stages[i] = ctx.attacker->stat_stages[i];
    }

    // 3. Trigger switch (player selects Pokemon)
    Pokemon* incoming = SelectSwitchTarget(ctx);

    // 4. Transfer stat stages to incoming Pokemon
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        incoming->stat_stages[i] = stat_stages[i];
    }

    // 5. Complete switch
    SwitchPokemon(ctx, incoming);
}
```

---

## Test Cases

### Stat Transfer Mechanics

1. **Transfers all stat stages**
   - Attacker has +2 ATK, +1 DEF, -1 SPEED
   - After Baton Pass, defender has +2 ATK, +1 DEF, -1 SPEED
   - All 7 stats transferred correctly

2. **Transfers positive stages**
   - Attacker has +6 ATK (max boost)
   - Defender receives +6 ATK

3. **Transfers negative stages**
   - Attacker has -3 SPEED
   - Defender receives -3 SPEED

4. **Overwrites defender's existing stages**
   - Defender starts with +2 ATK
   - Attacker has -1 ATK
   - After Baton Pass, defender has -1 ATK (overwritten)

5. **Transfers all seven stats**
   - ATK, DEF, SPEED, SPATK, SPDEF, ACC, EVASION all transferred

6. **Always succeeds**
   - move_failed = false (no failure conditions in simplified version)

### Edge Cases

7. **Neutral stages transfer**
   - Attacker has all stats at 0 (neutral)
   - Defender receives all stats at 0

8. **Mixed stages transfer**
   - Attacker has +6 ATK, +4 DEF, -6 SPEED, +2 SPATK, -3 SPDEF, +1 ACC, 0 EVASION
   - All transferred correctly

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:131` - EFFECT_BATON_PASS definition
- `data/battle_scripts_1.s:1694-1714` - BattleScript_EffectBatonPass
- `src/data/battle_moves.h:2941-2953` - Baton Pass move data

**Key Mechanics (Full Game):**
- Transfers stat stages: ATK, DEF, SPEED, SPATK, SPDEF, ACC, EVASION
- Transfers substitute (if active)
- Transfers Aqua Ring, Ingrain (if active)
- Transfers confusion (volatile status)
- Transfers Focus Energy (crit boost)
- Does NOT transfer: primary status (burn/paralysis), semi-invulnerable state

**Switching Flow:**
1. `jumpifcantswitch` - Check if switch is allowed
2. `openpartyscreen` - Player selects Pokemon
3. `switchoutabilities` - Run switch-out abilities (Intimidate, etc.)
4. `switchindataupdate` - Load incoming Pokemon data + **transfer stats**
5. `switchineffects` - Run switch-in abilities/effects

---

## Notes

- This implementation is **simplified** to work without a full party/switching system
- Focuses on the **core mechanic**: stat stage transfer
- Establishes the pattern for future full implementation
- In real battles, Baton Pass is used to "pass" stat boosts from a setup Pokemon to a sweeper
- Common Baton Pass chains: Ninjask (+Speed) → Sweeper
- Does NOT transfer primary status conditions (burn, paralysis, etc.)
- Does NOT transfer semi-invulnerable state (Fly, Dig)

---

## Implementation Checklist

- [x] Write specification document
- [x] Implement Effect_BatonPass with stat transfer logic
- [x] Write comprehensive tests for all stat stages
- [x] Add BatonPass to Move enum and MOVE_DATABASE
- [x] Add Effect_BatonPass to EFFECT_DISPATCH table
- [x] All tests passing
- [x] Mark specification as implemented

**Implementation Status:** ✅ Complete (2025-11-21)
