# Effect: SPEED_DOWN (String Shot)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SPEED_DOWN` (pokeemerald: 20)
**Introduced:** Generation I
**Example Moves:** String Shot, Growl (for Attack)
**Category:** Opponent-targeting stat modification (no damage)

---

## Overview

This effect lowers the **target's** Speed stat by 1 stage. This introduces **Speed stat modification**, extending the stat stage system to a third stat type (Attack, Defense, now Speed).

---
## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Lower target's Speed stat stage by 1
3. No damage dealt

This is the **Speed counterpart to Growl and Tail Whip** - validating that the stat system works for Speed just as it does for Attack and Defense.

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance based on move accuracy (95 for String Shot)
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. ModifyStatStage(STAT_SPEED, -1, affects_user=FALSE)
   ├── Check current stat stage for target's Speed
   ├── If already at -6 (minimum) → show "won't go lower" message, return
   ├── Decrease stat stage by 1 (or to min -6)
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Speed fell!"

3. (No damage, no faint check)
```

### Stat Stage System

Stat stages range from **-6 to +6** (13 possible values, with 0 being neutral).

**Speed Stage Multipliers:**

| Stage | Multiplier | Fraction | Effect |
|-------|-----------|----------|--------|
| 0     | 2/2 = 1.00x | 100% | Baseline |
| -1    | 2/3 = 0.67x | 67% | ~33% slower |
| -2    | 2/4 = 0.50x | 50% | Half speed (2x slower) |
| -3    | 2/5 = 0.40x | 40% | 60% slower |
| -4    | 2/6 = 0.33x | 33% | 67% slower |
| -5    | 2/7 = 0.29x | 29% | 71% slower |
| -6    | 2/8 = 0.25x | 25% | 75% slower (4x slower) |

**Formula:**
- If stage >= 0: multiplier = (2 + stage) / 2
- If stage < 0: multiplier = 2 / (2 - stage)

**Key Observation:** Speed stages affect **turn order** in battle. Lower Speed = acts later in the turn. Unlike Attack/Defense which affect damage, Speed affects action sequencing.

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `defender.stat_stages[STAT_SPEED]` | Decreased by 1 | Move hits, not at -6 |

**Pattern Consistency:**
- Same pattern as Growl (Attack -1) and Tail Whip (Defense -1)
- Modifies `defender.stat_stages` (opponent-targeting)
- Uses default `affects_user=false` parameter

### Edge Cases

- **Already at -6:** Cannot go lower, move fails with "won't go lower" message
- **Miss:** If accuracy check fails, no stat change occurs
- **Can stack:** Multiple uses lower Speed further (to -6 minimum)
- **No damage:** This move deals 0 damage, so no HP changes
- **Turn order:** In current implementation, Speed affects turn order (future feature)

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:533`:
```asm
BattleScript_EffectSpeedDown::
	setstatchanger STAT_SPEED, 1, TRUE
	goto BattleScript_EffectStatDown
```

Key observations:
- `setstatchanger STAT_SPEED, 1, TRUE` - sets which stat (Speed), by how much (1), direction (TRUE = down)
- Goes to common `BattleScript_EffectStatDown` routine (same as Growl/Tail Whip)
- Note: TRUE = down, FALSE = up (counterintuitive naming)

From `data/battle_scripts_1.s:555-583` (BattleScript_EffectStatDown):
```asm
BattleScript_EffectStatDown::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	jumpifsubstituteblocks BattleScript_ButItFailed
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_StatDownEnd
	jumpifbyte CMP_LESS_THAN, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_StatDownDoAnim
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_FELL_EMPTY, BattleScript_StatDownEnd
	pause B_WAIT_TIME_SHORT
	goto BattleScript_StatDownPrintString
BattleScript_StatDownDoAnim::
	attackanimation
	waitanimation
	setgraphicalstatchangevalues
	playanimation BS_TARGET, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
BattleScript_StatDownPrintString::
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_StatDownEnd::
	goto BattleScript_MoveEnd
```

**Key line:** No `MOVE_EFFECT_AFFECTS_USER` flag - modifies target, not user

### Move Data

From `src/data/battle_moves.h:1056-1066`:
```c
[MOVE_STRING_SHOT] =
{
    .effect = EFFECT_SPEED_DOWN,
    .power = 0,
    .type = TYPE_BUG,
    .accuracy = 95,        // Can miss (unlike self-targeting moves)
    .pp = 40,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_BOTH,  // Targets foes
    .priority = 0,
    .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
}
```

**Key observations:**
- `accuracy = 95` (can miss, unlike self-targeting moves which have 0)
- `target = MOVE_TARGET_BOTH` (targets opponents)
- Bug type (thematically fitting - bugs shoot webbing)
- High PP (40) - balanced by lower accuracy

---

## Command Sequence

```cpp
void Effect_SpeedDown(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::ModifyStatStage(ctx, STAT_SPEED, -1);  // Default: affects_user=false
    // No damage, no faint check
}
```

**Design Note:** Reuses existing AccuracyCheck and ModifyStatStage commands. Zero new commands needed - pure effect composition!

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_SpeedDown, LowersSpeedStage) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -1);
}

TEST(Effect_SpeedDown, DoesNotDealDamage) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_SpeedDown, CanStackMultipleTimes) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use String Shot 3 times: -1, -1, -1 = -3
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;
    Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -3);
}
```

### Edge Cases

```cpp
TEST(Effect_SpeedDown, MinimumStageMinus6) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -6);
}

TEST(Effect_SpeedDown, CanLowerFromPositiveStages) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = +2;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], +1);
}

TEST(Effect_SpeedDown, DoesNotModifyAttacker) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();

    int8_t original_stage = attacker.stat_stages[STAT_SPEED];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], original_stage);
}

TEST(Effect_SpeedDown, DoesNotAffectOtherStats) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    // Only Speed should be lowered
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -1);
    EXPECT_EQ(defender.stat_stages[STAT_ATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], 0);
}

TEST(Effect_SpeedDown, DoesNotCauseFaint) {
    auto attacker = CreateCaterpie();
    auto defender = CreatePikachu();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    EXPECT_FALSE(defender.is_fainted);
    EXPECT_GT(defender.current_hp, 0);
}
```

---

## Related Effects

- **EFFECT_ATTACK_DOWN** - Lowers Attack by 1 (Growl) - same pattern, different stat
- **EFFECT_DEFENSE_DOWN** - Lowers Defense by 1 (Tail Whip) - same pattern, different stat
- **EFFECT_SPEED_UP_2** - Raises Speed by 2 (Agility) - self-targeting counterpart
- **EFFECT_SPEED_DOWN_2** - Lowers Speed by 2 (Scary Face, Cotton Spore)
- **EFFECT_SPEED_DOWN_HIT** - Damage + Speed down chance (Icy Wind, Rock Tomb)

---

## Moves Using This Effect

| Move | Type | Power | Accuracy | PP | Notes |
|------|------|-------|----------|----|----- |
| String Shot | Bug | 0 | 95 | 40 | Speed -1 |
| Cotton Spore | Grass | 0 | 100 | 40 | Speed -2 (different effect) |
| Scary Face | Normal | 0 | 100 | 10 | Speed -2 (different effect) |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:533, 555-583`, `src/data/battle_moves.h:1056-1066`
**Validation Notes:** String Shot lowers target's Speed by 1 stage. Extends stat system to Speed (3rd stat validated). Accuracy 95%, can miss. Reuses ModifyStatStage with default affects_user=false.
