# Effect: SPEED_UP_2 (Agility)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SPEED_UP_2` (pokeemerald: 52)
**Introduced:** Generation I
**Example Moves:** Agility, Rock Polish
**Category:** Self-targeting stat modification (no damage)

---

## Overview

This effect raises the **user's** Speed stat by 2 stages. This is the **Speed counterpart to Swords Dance** (Attack +2) and **completes the Speed stat validation** (alongside String Shot for Speed -1).

---

## Behavior Specification

### What This Effect Does

1. Raise user's Speed stat stage by 2
2. No accuracy check (self-targeting moves cannot miss)
3. No damage dealt

This is the **self-targeting Speed setup move**, mirroring the pattern established by Swords Dance (Attack +2) and Iron Defense (Defense +2). It validates that the stat stage system works for Speed in both directions (down with String Shot, up with Agility).

### Execution Steps

```
1. ModifyStatStage(STAT_SPEED, +2, affects_user=TRUE)
   ├── Check current stat stage for user's Speed
   ├── If already at +6 (maximum) → show "won't go higher" message, return
   ├── Increase stat stage by 2 (or to max +6)
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Speed rose sharply!"

2. (No accuracy check - self-targeting)
3. (No damage, no faint check)
```

### Stat Stage System

Stat stages range from **-6 to +6** (13 possible values, with 0 being neutral).

**Speed Stage Multipliers:**

| Stage | Multiplier | Fraction | Effect |
|-------|-----------|----------|--------|
| 0     | 2/2 = 1.00x | 100% | Baseline |
| +1    | 3/2 = 1.50x | 150% | 50% faster |
| +2    | 4/2 = 2.00x | 200% | **Doubles speed** |
| +3    | 5/2 = 2.50x | 250% | 2.5x faster |
| +4    | 6/2 = 3.00x | 300% | 3x faster |
| +5    | 7/2 = 3.50x | 350% | 3.5x faster |
| +6    | 8/2 = 4.00x | 400% | **4x faster** (maximum) |

**Formula:**
- If stage >= 0: multiplier = (2 + stage) / 2
- If stage < 0: multiplier = 2 / (2 - stage)

**Key Observation:** Speed +2 doubles the Pokemon's effective Speed, making it extremely likely to act first in battle. This is one of the most powerful setup moves in Pokemon.

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `attacker.stat_stages[STAT_SPEED]` | Increased by 2 | Not at +6 |

**Pattern Consistency:**
- Same pattern as Swords Dance (Attack +2) and Iron Defense (Defense +2)
- Modifies `attacker.stat_stages` (self-targeting)
- Uses `affects_user=true` parameter
- No accuracy check (accuracy = 0 in move data)

### Edge Cases

- **Already at +6:** Cannot go higher, move fails with "won't go higher" message
- **No accuracy check:** Self-targeting moves cannot miss
- **Can stack:** Multiple uses raise Speed further (to +6 maximum)
- **Can be lowered:** Opponent can use String Shot to reduce Speed stages
- **No damage:** This move deals 0 damage, so no HP changes
- **Turn order:** Speed affects turn order (higher Speed = acts first in turn)

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:624`:
```asm
BattleScript_EffectSpeedUp2::
	setstatchanger STAT_SPEED, 2, FALSE
	goto BattleScript_EffectStatUp
```

Key observations:
- `setstatchanger STAT_SPEED, 2, FALSE` - sets which stat (Speed), by how much (2), direction (FALSE = up)
- Goes to common `BattleScript_EffectStatUp` routine (same as Swords Dance/Iron Defense)
- Note: FALSE = up, TRUE = down (counterintuitive naming)

From `data/battle_scripts_1.s:787-803` (BattleScript_EffectStatUp):
```asm
BattleScript_EffectStatUp::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_SPEED, MAX_STAT_STAGE, BattleScript_StatUpDoAnim
	jumpifstat BS_ATTACKER, CMP_NOT_EQUAL, 0, STAT_SPEED, BattleScript_StatUpEnd
BattleScript_StatUpDoAnim::
	attackanimation
	waitanimation
	setgraphicalstatchangevalues
	playanimation BS_ATTACKER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_StatUpEnd::
	goto BattleScript_MoveEnd
```

**Key line:** `BS_ATTACKER` - modifies user, not target (MOVE_EFFECT_AFFECTS_USER flag)

### Move Data

From `src/data/battle_moves.h:1264-1275`:
```c
[MOVE_AGILITY] =
{
    .effect = EFFECT_SPEED_UP_2,
    .power = 0,
    .type = TYPE_PSYCHIC,
    .accuracy = 0,          // Self-targeting moves don't need accuracy
    .pp = 30,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,  // Targets self
    .priority = 0,
    .flags = FLAG_SNATCH_AFFECTED,
}
```

**Key observations:**
- `accuracy = 0` (self-targeting moves cannot miss)
- `target = MOVE_TARGET_USER` (affects user, not opponent)
- Psychic type (thematically fitting - mental focus for speed)
- High PP (30) - can be used many times per battle

---

## Command Sequence

```cpp
void Effect_SpeedUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss
    commands::ModifyStatStage(ctx, STAT_SPEED, +2, /* affects_user= */ true);
    // No damage, no faint check
}
```

**Design Note:** Reuses existing ModifyStatStage command with `affects_user=true`. Zero new commands needed - pure effect composition!

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_SpeedUp2, RaisesSpeedStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +2);
}

TEST(Effect_SpeedUp2, DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_SpeedUp2, CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Agility 2 times: +2, +2 = +4
    Effect_SpeedUp2(ctx);
    ctx.move_failed = false;
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +4);
}
```

### Edge Cases

```cpp
TEST(Effect_SpeedUp2, MaximumStagePlus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    attacker.stat_stages[STAT_SPEED] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +6);
}

TEST(Effect_SpeedUp2, CanRaiseFromNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    attacker.stat_stages[STAT_SPEED] = -2;  // Lowered by String Shot

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], 0);
}

TEST(Effect_SpeedUp2, DoesNotModifyDefender) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    int8_t original_stage = defender.stat_stages[STAT_SPEED];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], original_stage);
}

TEST(Effect_SpeedUp2, DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    // Only Speed should be raised
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +2);
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], 0);
}

TEST(Effect_SpeedUp2, DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    EXPECT_FALSE(attacker.is_fainted);
    EXPECT_GT(attacker.current_hp, 0);
}

TEST(Effect_SpeedUp2, NoAccuracyCheck) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    move.accuracy = 0;  // Self-targeting moves have accuracy = 0

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    // Move should never miss
    EXPECT_FALSE(ctx.move_failed);
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +2);
}
```

---

## Related Effects

- **EFFECT_SPEED_DOWN** - Lowers Speed by 1 (String Shot) - opponent-targeting counterpart
- **EFFECT_SPEED_DOWN_2** - Lowers Speed by 2 (Scary Face, Cotton Spore)
- **EFFECT_ATTACK_UP_2** - Raises Attack by 2 (Swords Dance) - same pattern, different stat
- **EFFECT_DEFENSE_UP_2** - Raises Defense by 2 (Iron Defense) - same pattern, different stat
- **EFFECT_SPEED_UP_HIT** - Damage + Speed up chance (future)

---

## Moves Using This Effect

| Move | Type | Power | Accuracy | PP | Notes |
|------|------|-------|----------|----|----- |
| Agility | Psychic | 0 | 0 | 30 | Speed +2, self-targeting |
| Rock Polish | Rock | 0 | 0 | 20 | Speed +2, self-targeting |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [x] Tests written
- [x] Implementation complete
- [x] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:624, 787-803`, `src/data/battle_moves.h:1264-1275`
**Validation Notes:** Agility raises user's Speed by 2 stages (doubles effective Speed). Self-targeting, no accuracy check. Completes Speed stat validation (String Shot -1, Agility +2). Reuses ModifyStatStage with affects_user=true.
