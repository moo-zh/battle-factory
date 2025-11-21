# Effect: DEFENSE_UP_2 (Iron Defense)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_DEFENSE_UP_2` (pokeemerald: 51)
**Introduced:** Generation III
**Example Moves:** Iron Defense, Barrier, Acid Armor
**Category:** Self-targeting stat modification (no damage)

---

## Overview

This effect raises the **user's** Defense stat by 2 stages. This mirrors Swords Dance but for Defense instead of Attack, validating that the stat stage system works correctly for defensive stats.

---
## Behavior Specification

### What This Effect Does

1. Raise user's Defense stat stage by 2
2. No accuracy check (self-targeting moves cannot miss)
3. No damage dealt

This is the **defensive counterpart to Swords Dance** - the fundamental defensive setup move pattern.

### Execution Steps

```
1. ModifyStatStage(STAT_DEF, +2, affects_user=TRUE)
   ├── Check current stat stage for user's Defense
   ├── If already at +6 (maximum) → show "won't go higher" message, return
   ├── If at +5, cap at +6 (only +1 effective)
   ├── Increase stat stage by 2 (or to max +6)
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Defense rose sharply!" (+2 = "sharply")

2. (No damage, no accuracy check, no faint check)
```

### Stat Stage System

Stat stages range from **-6 to +6** (13 possible values, with 0 being neutral).

**Stage Multipliers for Defense:**

| Stage | Multiplier | Fraction | Damage Reduction |
|-------|-----------|----------|------------------|
| 0     | 2/2 = 1.00x | 100% | Baseline |
| +1    | 3/2 = 1.50x | 150% | ~33% less damage taken |
| +2    | 4/2 = 2.00x | 200% | ~50% less damage taken (2x Defense) |
| +3    | 5/2 = 2.50x | 250% | ~60% less damage taken |
| +4    | 6/2 = 3.00x | 300% | ~67% less damage taken (3x Defense) |
| +5    | 7/2 = 3.50x | 350% | ~71% less damage taken |
| +6    | 8/2 = 4.00x | 400% | ~75% less damage taken (4x Defense) |

**Formula:**
- If stage >= 0: multiplier = (2 + stage) / 2
- If stage < 0: multiplier = 2 / (2 - stage)

**Key Observation:** Iron Defense (+2 Defense) **doubles effective Defense**, reducing physical damage taken by approximately 50%.

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `attacker.stat_stages[STAT_DEF]` | Increased by 2 | Not at +6 |

**Pattern Consistency:**
- Same pattern as Swords Dance, just different stat
- Modifies `attacker.stat_stages` (self-targeting)
- Uses same `affects_user=true` parameter

### Edge Cases

- **Already at +6:** Cannot go higher, move fails with "won't go higher" message
- **At +5:** Only increases by 1 (to +6 cap), still shows message
- **No target required:** Self-targeting moves skip target selection
- **Can't miss:** No accuracy check for self-targeting moves (accuracy = 0 in move data)
- **Taunt:** Blocked by Taunt (future implementation - status moves only)
- **No damage:** This move deals 0 damage, so no HP changes

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:619`:
```asm
BattleScript_EffectDefenseUp2::
	setstatchanger STAT_DEF, 2, FALSE
	goto BattleScript_EffectStatUp
```

Key observations:
- `setstatchanger STAT_DEF, 2, FALSE` - sets which stat (Defense), by how much (+2), direction (FALSE = up)
- Goes to common `BattleScript_EffectStatUp` routine (same as Swords Dance)
- Note: FALSE = up, TRUE = down (counterintuitive naming)

From `data/battle_scripts_1.s:787-803` (BattleScript_EffectStatUp):
```asm
BattleScript_EffectStatUp::
	attackcanceler
BattleScript_EffectStatUpAfterAtkCanceler::
	attackstring
	ppreduce
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_StatUpEnd
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_StatUpAttackAnim
	pause B_WAIT_TIME_SHORT
	goto BattleScript_StatUpPrintString
BattleScript_StatUpAttackAnim::
	attackanimation
	waitanimation
BattleScript_StatUpDoAnim::
	setgraphicalstatchangevalues
	playanimation BS_ATTACKER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
BattleScript_StatUpPrintString::
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_StatUpEnd::
	goto BattleScript_MoveEnd
```

**Critical line:** `statbuffchange MOVE_EFFECT_AFFECTS_USER | ...`
- This flag tells the engine to modify the **attacker's** stats, not the defender's
- Without this flag, stat changes apply to the target (like Tail Whip)

### Move Data

From `src/data/battle_moves.h:4345-4355`:
```c
[MOVE_IRON_DEFENSE] =
{
    .effect = EFFECT_DEFENSE_UP_2,
    .power = 0,
    .type = TYPE_STEEL,
    .accuracy = 0,         // 0 = always hits (self-targeting, no accuracy check)
    .pp = 15,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,  // Self-targeting
    .priority = 0,
    .flags = FLAG_SNATCH_AFFECTED,  // Can be stolen by Snatch
}
```

**Key differences from Tail Whip:**
- `accuracy = 0` (not 100) - self-targeting moves can't miss
- `target = MOVE_TARGET_USER` (not MOVE_TARGET_BOTH)
- `flags = FLAG_SNATCH_AFFECTED` (not protect/reflect flags)
- Steel type (thematically fitting for "Iron" Defense)

---

## Command Sequence

```cpp
void Effect_DefenseUp2(BattleContext& ctx) {
    ModifyStatStage(ctx, STAT_DEF, +2, /* affects_user= */ true);
    // No accuracy check (self-targeting can't miss)
    // No damage, no faint check
}
```

**Design Note:** Reuses existing `ModifyStatStage` command with `affects_user=true` parameter. Zero new commands needed - pure effect composition.

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_DefenseUp2, RaisesDefenseStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +2);  // Attacker boosted, not defender
}

TEST(Effect_DefenseUp2, DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_DefenseUp2, CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Iron Defense 3 times: +2, +2, +2 = +6
    Effect_DefenseUp2(ctx);
    ctx.move_failed = false;
    Effect_DefenseUp2(ctx);
    ctx.move_failed = false;
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +6);  // Capped at +6
}
```

### Edge Cases

```cpp
TEST(Effect_DefenseUp2, MaximumStagePlus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +6);  // Stays at +6
}

TEST(Effect_DefenseUp2, CapsAtPlus6FromPlus5) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +6);  // Only +1 effective
}

TEST(Effect_DefenseUp2, CanRaiseFromNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = -3;  // Lowered by Tail Whip 3x

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], -1);  // -3 + 2 = -1
}

TEST(Effect_DefenseUp2, DoesNotModifyDefender) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    int8_t original_stage = defender.stat_stages[STAT_DEF];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], original_stage);  // Defender unchanged
}

TEST(Effect_DefenseUp2, DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    // Only Defense should be raised
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +2);
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], 0);
}

TEST(Effect_DefenseUp2, DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    EXPECT_FALSE(defender.is_fainted);
    EXPECT_GT(defender.current_hp, 0);
}
```

### Integration with Damage

```cpp
TEST(StatStages, DefenseUp2ReducesDamageTaken) {
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto attacker = CreateCharmander();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with +2 Defense
    defender2.stat_stages[STAT_DEF] = +2;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t reduced_damage = ctx2.damage_dealt;

    // With +2 Defense, damage should be roughly halved (2x Defense = ~0.5x damage)
    EXPECT_LT(reduced_damage, normal_damage);

    // Expect reduced damage to be approximately 40-60% of normal (allowing for rounding)
    uint16_t min_expected = (normal_damage * 40) / 100;
    uint16_t max_expected = (normal_damage * 60) / 100;
    EXPECT_GE(reduced_damage, min_expected);
    EXPECT_LE(reduced_damage, max_expected);
}
```

---

## Related Effects

- **EFFECT_ATTACK_UP_2** - Raises Attack by 2 (Swords Dance) - offensive counterpart
- **EFFECT_DEFENSE_DOWN** - Lowers Defense by 1 (Tail Whip) - targets opponent
- **EFFECT_DEFENSE_UP** - Raises Defense by 1 (Withdraw) - self-targeting +1
- **EFFECT_DEFENSE_DOWN_2** - Lowers Defense by 2 (Screech) - targets opponent
- **EFFECT_COSMIC_POWER** - Raises Defense+Sp. Def by 1 each - multi-stat self boost

---

## Moves Using This Effect

| Move | Type | PP | Accuracy | Notes |
|------|------|----|---------  |-------|
| Iron Defense | Steel | 15 | 0 (always hits) | Defense +2 |
| Barrier | Psychic | 20 | 0 | Defense +2 |
| Acid Armor | Poison | 20 | 0 | Defense +2 |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:619, 787-803`, `src/data/battle_moves.h:4345-4355`
**Validation Notes:** Iron Defense raises user's Defense by 2 stages (+2 = 2x Defense, ~50% damage reduction). Defensive counterpart to Swords Dance. No accuracy check for self-targeting moves. Reuses ModifyStatStage with affects_user=true.
