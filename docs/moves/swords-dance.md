# Effect: ATTACK_UP_2 (Swords Dance)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_ATTACK_UP_2` (pokeemerald: 50)
**Introduced:** Generation I
**Example Moves:** Swords Dance, Howl
**Category:** Self-targeting stat modification (no damage)

---

## Overview

This effect raises the **user's** Attack stat by 2 stages. This is the first **self-targeting stat move**, introducing the concept that stat changes can affect the attacker instead of the defender.

---
## Behavior Specification

### What This Effect Does

1. Raise user's Attack stat stage by 2
2. No accuracy check (self-targeting moves cannot miss)
3. No damage dealt

This introduces **self-stat boosting** - the fundamental setup move pattern in Pokemon.

### Execution Steps

```
1. ModifyStatStage(STAT_ATK, +2, affects_user=TRUE)
   ├── Check current stat stage for user's Attack
   ├── If already at +6 (maximum) → show "won't go higher" message, return
   ├── If at +5, cap at +6 (only +1 effective)
   ├── Increase stat stage by 2 (or to max +6)
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Attack rose sharply!" (+2 = "sharply")

2. (No damage, no accuracy check, no faint check)
```

### Stat Stage System

Stat stages range from **-6 to +6** (13 possible values, with 0 being neutral).

**Stage Multipliers:**

| Stage | Multiplier | Fraction | Damage Boost |
|-------|-----------|----------|--------------|
| 0     | 2/2 = 1.00x | 100% | Baseline |
| +1    | 3/2 = 1.50x | 150% | +50% |
| +2    | 4/2 = 2.00x | 200% | +100% (2x) |
| +3    | 5/2 = 2.50x | 250% | +150% |
| +4    | 6/2 = 3.00x | 300% | +200% (3x) |
| +5    | 7/2 = 3.50x | 350% | +250% |
| +6    | 8/2 = 4.00x | 400% | +300% (4x) |

**Formula:**
- If stage >= 0: multiplier = (2 + stage) / 2
- If stage < 0: multiplier = 2 / (2 - stage)

**Key Observation:** Swords Dance (+2 Attack) **doubles damage output**. This is why it's such a powerful setup move.

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `attacker.stat_stages[STAT_ATK]` | Increased by 2 | Not at +6 |

**Critical difference from Growl/Tail Whip:**
- Previous moves: Modified `defender.stat_stages`
- This move: Modifies `attacker.stat_stages` (self-targeting)

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

From `data/battle_scripts_1.s:614`:
```asm
BattleScript_EffectAttackUp2::
	setstatchanger STAT_ATK, 2, FALSE
	goto BattleScript_EffectStatUp
```

Key observations:
- `setstatchanger STAT_ATK, 2, FALSE` - sets which stat (Attack), by how much (+2), direction (FALSE = up)
- Goes to common `BattleScript_EffectStatUp` routine
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
- Without this flag, stat changes apply to the target (like Growl)

### Move Data

From `src/data/battle_moves.h:185-195`:
```c
[MOVE_SWORDS_DANCE] =
{
    .effect = EFFECT_ATTACK_UP_2,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 0,         // 0 = always hits (self-targeting, no accuracy check)
    .pp = 30,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,  // Self-targeting
    .priority = 0,
    .flags = FLAG_SNATCH_AFFECTED,  // Can be stolen by Snatch
    .split = SPLIT_STATUS,
}
```

**Key differences from Growl:**
- `accuracy = 0` (not 100) - self-targeting moves can't miss
- `target = MOVE_TARGET_USER` (not MOVE_TARGET_BOTH)
- `flags = FLAG_SNATCH_AFFECTED` (not protect/reflect flags)

---

## Command Sequence

```cpp
void Effect_AttackUp2(BattleContext& ctx) {
    ModifyStatStage(ctx, STAT_ATK, +2, /* affects_user= */ true);
    // No accuracy check (self-targeting can't miss)
    // No damage, no faint check
}
```

**Design Decision:** Need to extend `ModifyStatStage` to support targeting the attacker.

**Options:**
1. Add `bool affects_user` parameter to ModifyStatStage
2. Create separate `ModifyUserStatStage` command
3. Add `target` parameter (Pokemon* target) to ModifyStatStage

**Recommendation:** Option 1 (bool parameter) - keeps command unified, matches pokeemerald's flag approach.

---

## Implementation: Extending ModifyStatStage

### Current Signature
```cpp
void ModifyStatStage(BattleContext& ctx, Stat stat, int8_t change);
// Hardcoded to modify ctx.defender
```

### New Signature
```cpp
void ModifyStatStage(BattleContext& ctx, Stat stat, int8_t change, bool affects_user = false);
// Default: affects_user=false (modifies defender, backward compatible)
// When true: modifies attacker instead
```

### Implementation
```cpp
inline void ModifyStatStage(BattleContext& ctx, domain::Stat stat, int8_t change,
                            bool affects_user = false) {
    if (ctx.move_failed) return;

    // Select target based on affects_user flag
    state::Pokemon* target = affects_user ? ctx.attacker : ctx.defender;

    // Get current stage
    int8_t current_stage = target->stat_stages[stat];

    // Calculate new stage (clamped to -6..+6)
    int16_t new_stage_unclamped = (int16_t)current_stage + (int16_t)change;
    int8_t new_stage = (int8_t)new_stage_unclamped;

    if (new_stage < -6) new_stage = -6;
    if (new_stage > 6) new_stage = 6;

    // Check if change occurred
    if (new_stage == current_stage) {
        // Stat won't go lower/higher
        return;
    }

    // Apply the change
    target->stat_stages[stat] = new_stage;
}
```

**Backward Compatibility:** Default parameter `affects_user=false` means existing calls work unchanged:
```cpp
// Existing calls (Growl, Tail Whip) work as before
ModifyStatStage(ctx, STAT_ATK, -1);  // affects_user defaults to false

// New calls for self-targeting
ModifyStatStage(ctx, STAT_ATK, +2, true);  // affects_user = true
```

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_AttackUp2, RaisesAttackStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +2);  // Attacker boosted, not defender
}

TEST(Effect_AttackUp2, DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_AttackUp2, CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Swords Dance 3 times: +2, +2, +2 = +6
    Effect_AttackUp2(ctx);
    ctx.move_failed = false;
    Effect_AttackUp2(ctx);
    ctx.move_failed = false;
    Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6);  // Capped at +6
}
```

### Edge Cases

```cpp
TEST(Effect_AttackUp2, MaximumStagePlus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6);  // Stays at +6
}

TEST(Effect_AttackUp2, CapsAtPlus6FromPlus5) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6);  // Only +1 effective
}

TEST(Effect_AttackUp2, CanRaiseFromNegativeStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = -3;  // Lowered by Growl 3x

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], -1);  // -3 + 2 = -1
}

TEST(Effect_AttackUp2, DoesNotModifyDefender) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    int8_t original_stage = defender.stat_stages[STAT_ATK];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], original_stage);  // Defender unchanged
}

TEST(Effect_AttackUp2, DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    // Only Attack should be raised
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +2);
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], 0);
}
```

### Integration with Damage

```cpp
TEST(StatStages, AttackUp2DoublesDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with +2 Attack
    attacker.stat_stages[STAT_ATK] = +2;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t boosted_damage = ctx2.damage_dealt;

    // With +2 Attack, damage should be doubled (2x multiplier)
    EXPECT_GT(boosted_damage, normal_damage);

    // Allow 2 damage rounding error
    uint16_t expected_damage = normal_damage * 2;
    int16_t diff = (int16_t)boosted_damage - (int16_t)expected_damage;
    if (diff < 0) diff = -diff;
    EXPECT_LE(diff, 2);  // Within 2 damage of exactly 2x
}
```

---

## Related Effects

- **EFFECT_ATTACK_DOWN** - Lowers Attack by 1 (Growl) - targets opponent
- **EFFECT_DEFENSE_DOWN** - Lowers Defense by 1 (Tail Whip) - targets opponent
- **EFFECT_DEFENSE_UP_2** - Raises Defense by 2 (Iron Defense) - self-targeting
- **EFFECT_ATTACK_UP** - Raises Attack by 1 (Meditate) - self-targeting
- **EFFECT_DRAGON_DANCE** - Raises Attack+Speed by 1 each - multi-stat self boost

---

## Moves Using This Effect

| Move | Type | Accuracy | Notes |
|------|------|----------|-------|
| Swords Dance | Normal | 0 (always hits) | Attack +2 |
| Nasty Plot | Dark | 0 | Sp. Attack +2 (different effect in code) |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [x] ModifyStatStage extended to support self-targeting (backward compatible)
- [x] Tests written (10 tests)
- [x] Implementation complete
- [x] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:614, 787-803`, `src/data/battle_moves.h:185-195`
**Validation Notes:** Swords Dance raises user's Attack by 2 stages (+2 = 2x damage multiplier). First self-targeting stat move. No accuracy check for self-targeting moves.
