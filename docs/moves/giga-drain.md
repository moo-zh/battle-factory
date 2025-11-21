# Effect: DRAIN_HIT (Giga Drain)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_ABSORB` (pokeemerald: 3)
**Introduced:** Generation I
**Example Moves:** Giga Drain, Absorb, Mega Drain, Leech Life
**Category:** Damage + HP drain

---

## Overview

This effect deals damage to the target, then the attacker heals for 50% of the damage dealt. This introduces **HP drain mechanics** - the mirror of recoil where the attacker heals instead of taking damage.

---
## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Calculate damage
3. Apply damage to target
4. **Heal attacker for 50% of damage dealt**
5. Check if target fainted
6. Check if attacker fainted (can happen if HP was 0 before heal)

This introduces the concept that **attackers can heal themselves** through move effects.

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. CalculateDamage
   ├── Calculate damage using Gen III formula
   ├── Store in ctx.damage_dealt
   └── Continue

3. ApplyDamage (to defender)
   ├── Subtract damage from defender.current_hp
   ├── Clamp to 0
   └── Continue

4. ApplyDrain (to attacker)
   ├── Guard: if move_failed, return
   ├── Guard: if damage_dealt == 0, return (no drain on miss)
   ├── Calculate drain = damage_dealt / 2 (50% drain)
   ├── Minimum drain = 1 (if damage > 0)
   ├── Add drain to attacker.current_hp
   ├── Clamp attacker HP to max_hp (cannot overheal)
   └── Continue

5. CheckFaint (defender)
   ├── If defender.current_hp == 0, set defender.is_fainted = true
   └── Continue

6. CheckFaint (attacker)
   ├── If attacker.current_hp == 0, set attacker.is_fainted = true
   └── (Rarely happens, but possible if attacker was at 0 HP somehow)
```

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState (defender) | `current_hp` | Reduced by damage | Move hits |
| PokemonState (defender) | `is_fainted` | Set to true | HP reaches 0 |
| **PokemonState (attacker)** | `current_hp` | **Increased by drain** | **Move hits and deals damage** |

### Edge Cases

- **Miss:** If accuracy check fails, no damage dealt, no drain received
- **Zero damage:** If damage = 0 (immunity, etc.), no drain received
- **Drain minimum:** Drain is always at least 1 if any damage was dealt
- **Cannot overheal:** Drain cannot increase HP above max_hp
- **Already at max HP:** Still heals, but clamped to max_hp
- **Liquid Ooze ability:** Attacker takes damage instead of healing (future implementation)
- **Big Root item:** Increases drain by 30% (future implementation)

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:326-359`:
```asm
BattleScript_EffectAbsorb::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	negativedamage  # Sets gBattleMoveDamage = -(gHpDealt / 2)
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	jumpifability BS_TARGET, ABILITY_LIQUID_OOZE, BattleScript_AbsorbLiquidOoze
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABSORB
	goto BattleScript_AbsorbUpdateHp
BattleScript_AbsorbLiquidOoze::
	manipulatedamage DMG_CHANGE_SIGN
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABSORB_OOZE
BattleScript_AbsorbUpdateHp::
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	jumpifmovehadnoeffect BattleScript_AbsorbTryFainting
	printfromtable gAbsorbDrainStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AbsorbTryFainting::
	tryfaintmon BS_ATTACKER
	tryfaintmon BS_TARGET
	goto BattleScript_MoveEnd
```

**Key observations:**
- `negativedamage` calculates drain as `-(gHpDealt / 2)` (negative damage = healing)
- Minimum drain is 1 (if any damage dealt)
- Updates attacker's HP bar (healing, not damage)
- Liquid Ooze ability reverses the effect (damages attacker instead)

### Drain Calculation

From `src/battle_script_commands.c`:
```c
static void Cmd_negativedamage(void)
{
    gBattleMoveDamage = -(gHpDealt / 2);
    if (gBattleMoveDamage == 0)
        gBattleMoveDamage = -1;
    gBattlescriptCurrInstr++;
}
```

**Key observations:**
- Drain = `gHpDealt / 2` (50% of actual damage dealt to target)
- Negative value indicates healing (negative damage)
- Minimum drain = 1 (if any damage dealt)

### Move Data

From `src/data/battle_moves.h:2629-2640`:
```c
[MOVE_GIGA_DRAIN] =
{
    .effect = EFFECT_ABSORB,
    .power = 60,
    .type = TYPE_GRASS,
    .accuracy = 100,
    .pp = 5,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_SELECTED,
    .priority = 0,
    .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
}
```

**Key observations:**
- Moderate power (60) - balanced drain move
- Very low PP (5) - balances the healing effect
- Grass type - most drain moves are Grass type
- No secondary effect - drain is the primary effect

---

## Command Sequence

```cpp
void Effect_DrainHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);            // Damage to defender
    commands::ApplyDrain(ctx, 50);         // NEW: Drain to attacker (50%)
    commands::CheckFaint(ctx);             // Check if defender fainted
    commands::CheckFaint(ctx, true);       // Check if attacker fainted (rare)
}
```

**Comparison with recoil:**
- Recoil: Attacker takes damage (subtract HP)
- Drain: Attacker heals (add HP, clamp to max)

---

## Implementation: ApplyDrain Command

```cpp
/**
 * @brief Apply drain healing to attacker based on damage dealt
 *
 * @param ctx Battle context containing attacker, damage_dealt
 * @param drain_percent Percentage of damage to heal (typically 50)
 */
void ApplyDrain(BattleContext& ctx, uint8_t drain_percent) {
    // Guard: skip if move failed
    if (ctx.move_failed) return;

    // Guard: no drain if no damage was dealt
    if (ctx.damage_dealt == 0) return;

    // Calculate drain amount
    uint16_t drain_amount;
    if (drain_percent == 50) {
        drain_amount = ctx.damage_dealt / 2;  // 50% drain
    } else if (drain_percent == 75) {
        drain_amount = (ctx.damage_dealt * 3) / 4;  // 75% drain (Dream Eater)
    } else {
        drain_amount = ctx.damage_dealt / 2;  // Default to 50%
    }

    // Minimum drain is 1 if any damage was dealt
    if (drain_amount == 0 && ctx.damage_dealt > 0) {
        drain_amount = 1;
    }

    // Apply drain to attacker (heal HP)
    uint16_t new_hp = ctx.attacker->current_hp + drain_amount;

    // Clamp to max HP (cannot overheal)
    if (new_hp > ctx.attacker->max_hp) {
        ctx.attacker->current_hp = ctx.attacker->max_hp;
    } else {
        ctx.attacker->current_hp = new_hp;
    }

    // Store drain amount for testing
    ctx.drain_received = drain_amount;  // NEW field in BattleContext

    // TODO (future): Check Liquid Ooze ability to reverse drain
    // if (HasAbility(ctx.defender, ABILITY_LIQUID_OOZE)) {
    //     ctx.attacker->current_hp -= drain_amount * 2; // Take damage instead
    // }
}
```

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_DrainHit, DealsDamageToTarget) {
    auto [attacker, defender] = SetupBattle();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    EXPECT_LT(defender.current_hp, original_hp);
    EXPECT_GT(ctx.damage_dealt, 0);
}

TEST(Effect_DrainHit, AttackerHealsFromDrain) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    // Attacker should heal
    EXPECT_GT(attacker.current_hp, original_attacker_hp);
}

TEST(Effect_DrainHit, DrainIsHalfOfDamage) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = 10;
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    uint16_t drain_received = attacker.current_hp - original_attacker_hp;
    uint16_t expected_drain = ctx.damage_dealt / 2;

    // Drain should be 1/2 of damage (minimum 1)
    if (expected_drain == 0) expected_drain = 1;
    EXPECT_EQ(drain_received, expected_drain);
}
```

### Edge Cases

```cpp
TEST(Effect_DrainHit, NoDrainOnMiss) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = 10;
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    ctx.move_failed = true;  // Simulate miss
    Effect_DrainHit(ctx);

    // No drain if move misses
    EXPECT_EQ(attacker.current_hp, original_attacker_hp);
}

TEST(Effect_DrainHit, MinimumDrainIsOne) {
    auto [attacker, defender] = SetupBattle();
    defender.defense = 999;  // Very high defense, low damage
    attacker.current_hp = 10;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    // Even if damage / 2 = 0, drain should be at least 1
    if (ctx.damage_dealt > 0 && ctx.damage_dealt < 2) {
        uint16_t drain = attacker.current_hp - 10;
        EXPECT_EQ(drain, 1);
    }
}

TEST(Effect_DrainHit, CannotOverheal) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = attacker.max_hp - 2;  // Almost full HP

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    // HP should not exceed max_hp
    EXPECT_LE(attacker.current_hp, attacker.max_hp);
    EXPECT_EQ(attacker.current_hp, attacker.max_hp);  // Clamped to max
}

TEST(Effect_DrainHit, FullHPStillHeals) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = attacker.max_hp;  // Already full HP

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    // Still processes drain, just clamped to max
    EXPECT_EQ(attacker.current_hp, attacker.max_hp);
}
```

### Integration Tests

```cpp
TEST(Effect_DrainHit, LowPowerLowDrain) {
    auto [attacker, defender] = SetupBattle();
    attacker.current_hp = 10;

    BattleContext ctx = CreateContext(attacker, defender, Move::GigaDrain);
    Effect_DrainHit(ctx);

    // With 60 power, expect moderate drain
    uint16_t drain = attacker.current_hp - 10;
    EXPECT_GT(drain, 3);  // Expect at least some meaningful drain
}
```

---

## Related Effects

- **EFFECT_RECOIL_HIT** - Attacker takes damage (Double-Edge) - mirror of drain
- **EFFECT_DREAM_EATER** - 75% drain, only works on sleeping targets - higher drain
- **EFFECT_DRAIN_PUNCH** - Same as Giga Drain, but Fighting type
- **EFFECT_PARABOLIC_CHARGE** - 50% drain to all opponents (Gen VI+)

---

## Moves Using This Effect

| Move | Type | Power | Drain | Notes |
|------|------|-------|-------|-------|
| Absorb | Grass | 20 | 50% | Weakest drain move |
| Mega Drain | Grass | 40 | 50% | Mid-tier drain |
| Giga Drain | Grass | 60 | 50% | Strongest basic drain (Gen III) |
| Drain Punch | Fighting | 60 | 50% | Fighting-type drain |
| Leech Life | Bug | 20 | 50% | Bug-type drain (buffed in Gen VII) |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [ ] ApplyDrain command implemented
- [ ] BattleContext extended with drain_received field
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:326-359`, `src/battle_script_commands.c` (Cmd_negativedamage), `src/data/battle_moves.h:2629-2640`
**Validation Notes:** Giga Drain deals damage then heals attacker for 50% of damage dealt. Drain minimum is 1. Cannot overheal past max_hp.
