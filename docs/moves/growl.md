# Effect: ATTACK_DOWN (Growl)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_ATTACK_DOWN` (pokeemerald: 18)
**Introduced:** Generation I
**Example Moves:** Growl, Tail Whip (for Defense)
**Category:** Stat Modification (no damage)

---

## Overview

This effect lowers the target's Attack stat by 1 stage. This is our first **stat stage modification** move.

---

## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Lower target's Attack stat stage by 1
3. No damage is dealt

This introduces the **stat stage system** - a fundamental Pokemon mechanic where stats can be temporarily modified during battle.

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance = move.accuracy * accuracy_stage / evasion_stage
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. ModifyStatStage(STAT_ATK, -1)
   ├── Guard: if move_failed, return
   ├── Check current stat stage for Attack
   ├── If already at -6 (minimum) → show "won't go lower" message, return
   ├── Decrease stat stage by 1
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Attack fell!"

3. (No damage or faint - status move)
```

### Stat Stage System

Stat stages range from **-6 to +6** (13 possible values, with 0 being neutral).

**Stage Multipliers:**

| Stage | Multiplier | Fraction |
|-------|-----------|----------|
| -6    | 2/8 = 0.25x | 25% |
| -5    | 2/7 ≈ 0.29x | 29% |
| -4    | 2/6 ≈ 0.33x | 33% |
| -3    | 2/5 = 0.40x | 40% |
| -2    | 2/4 = 0.50x | 50% |
| -1    | 2/3 ≈ 0.67x | 67% |
| 0     | 2/2 = 1.00x | 100% (neutral) |
| +1    | 3/2 = 1.50x | 150% |
| +2    | 4/2 = 2.00x | 200% |
| +3    | 5/2 = 2.50x | 250% |
| +4    | 6/2 = 3.00x | 300% |
| +5    | 7/2 = 3.50x | 350% |
| +6    | 8/2 = 4.00x | 400% |

**Formula:**
- If stage >= 0: multiplier = (2 + stage) / 2
- If stage < 0: multiplier = 2 / (2 - stage)

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `defender.stat_stages[STAT_ATK]` | Decreased by 1 | Move hits, not at -6 |

### Edge Cases

- **Already at -6:** Cannot go lower, move fails with "won't go lower" message
- **Substitute:** Blocked by substitute (unlike burn which applies through sub)
- **No damage:** This move deals 0 damage, so no HP changes
- **Clear Body ability:** Prevents stat drops (future implementation)
- **Mist field effect:** Prevents stat drops (future implementation)

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:516-554`:
```asm
BattleScript_EffectAttackDown::
	setstatchanger STAT_ATK, 1, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectStatDown::
	attackcanceler
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_FailedFromAtkString
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
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

Key observations:
- `setstatchanger STAT_ATK, 1, TRUE` - sets which stat to change, by how much, and direction (TRUE = down)
- Substitute blocks stat drops
- Accuracy check happens before stat change
- Message changes if stat won't go lower

### Move Data

From `src/data/battle_moves.h:588-598`:
```c
[MOVE_GROWL] =
{
    .effect = EFFECT_ATTACK_DOWN,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 100,
    .pp = 40,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_BOTH,
    .priority = 0,
    .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
    .split = SPLIT_STATUS,
}
```

### Stat Constants

From `include/constants/pokemon.h:74-84`:
```c
#define STAT_HP      0
#define STAT_ATK     1
#define STAT_DEF     2
#define STAT_SPEED   3
#define STAT_SPATK   4
#define STAT_SPDEF   5
#define NUM_STATS    6

#define STAT_ACC     6 // Only in battles
#define STAT_EVASION 7 // Only in battles
```

---

## Command Sequence

```cpp
void Effect_AttackDown(BattleContext& ctx) {
    AccuracyCheck(ctx);
    ModifyStatStage(ctx, STAT_ATK, -1);  // Lower Attack by 1 stage
    // No damage, no faint check
}
```

---

## ModifyStatStage Implementation

```cpp
enum Stat : uint8_t {
    STAT_HP = 0,
    STAT_ATK = 1,
    STAT_DEF = 2,
    STAT_SPEED = 3,
    STAT_SPATK = 4,
    STAT_SPDEF = 5,
    STAT_ACC = 6,      // Battle-only
    STAT_EVASION = 7,  // Battle-only
};

void ModifyStatStage(BattleContext& ctx, Stat stat, int8_t change) {
    // Guard: skip if move already failed
    if (ctx.move_failed) return;

    // Get current stage for this stat
    int8_t current_stage = ctx.defender->stat_stages[stat];

    // Calculate new stage (clamped to -6..+6)
    int8_t new_stage = current_stage + change;
    if (new_stage < -6) new_stage = -6;
    if (new_stage > 6) new_stage = 6;

    // Check if change actually happened
    if (new_stage == current_stage) {
        // Stat won't go lower/higher
        // TODO: Set message flag
        return;
    }

    // Apply the stat stage change
    ctx.defender->stat_stages[stat] = new_stage;

    // TODO: Set message for stat change
    // "[Pokemon]'s [Stat] fell!" or "[Stat] rose!"
}
```

---

## Applying Stat Stages to Damage

When calculating damage, stat stages must be applied:

```cpp
int GetModifiedStat(const Pokemon& p, Stat stat) {
    int base_stat = 0;
    switch (stat) {
        case STAT_ATK: base_stat = p.attack; break;
        case STAT_DEF: base_stat = p.defense; break;
        case STAT_SPATK: base_stat = p.sp_attack; break;
        case STAT_SPDEF: base_stat = p.sp_defense; break;
        case STAT_SPEED: base_stat = p.speed; break;
        default: return base_stat;
    }

    int8_t stage = p.stat_stages[stat];

    // Apply stage multiplier
    if (stage >= 0) {
        return (base_stat * (2 + stage)) / 2;
    } else {
        return (base_stat * 2) / (2 - stage);
    }
}

// In CalculateDamage:
int attack = GetModifiedStat(*ctx.attacker, STAT_ATK);
int defense = GetModifiedStat(*ctx.defender, STAT_DEF);
```

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_AttackDown, LowersAttackStage) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1);
}

TEST(Effect_AttackDown, DoesNotDealDamage) {
    auto [attacker, defender] = SetupBattle();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_AttackDown, CanStackMultipleTimes) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);

    // Use Growl 3 times
    Effect_AttackDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_AttackDown(ctx);
    ctx.move_failed = false;
    Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -3);
}
```

### Edge Cases

```cpp
TEST(Effect_AttackDown, MinimumStageMinus6) {
    auto [attacker, defender] = SetupBattle();
    defender.stat_stages[STAT_ATK] = -6;  // Already at minimum

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -6);  // Stays at -6
    // TODO: Check for "won't go lower" message
}

TEST(Effect_AttackDown, CanLowerFromPositiveStages) {
    auto [attacker, defender] = SetupBattle();
    defender.stat_stages[STAT_ATK] = 2;  // Boosted by Swords Dance

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], 1);  // Now at +1
}

TEST(Effect_AttackDown, DoesNotModifyAttacker) {
    auto [attacker, defender] = SetupBattle();

    int8_t original_stage = attacker.stat_stages[STAT_ATK];
    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], original_stage);
}

TEST(Effect_AttackDown, DoesNotAffectOtherStats) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::Growl);
    Effect_AttackDown(ctx);

    // Only Attack should be lowered
    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1);
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], 0);
}
```

### Integration with Damage

```cpp
TEST(StatStages, AttackStageAffectsDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto move = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &move);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with -1 Attack
    attacker.stat_stages[STAT_ATK] = -1;
    auto ctx2 = SetupContext(&attacker, &defender2, &move);
    Effect_Hit(ctx2);
    uint16_t reduced_damage = ctx2.damage_dealt;

    // With -1 Attack, damage should be reduced (multiplied by 2/3)
    EXPECT_LT(reduced_damage, normal_damage);
    EXPECT_NEAR(reduced_damage, normal_damage * 2 / 3, 2);  // Allow 2 damage rounding
}
```

---

## Related Effects

- **EFFECT_DEFENSE_DOWN** - Lowers Defense (Tail Whip, Leer)
- **EFFECT_SPEED_DOWN** - Lowers Speed (String Shot)
- **EFFECT_ATTACK_UP** - Raises Attack (Meditate)
- **EFFECT_ATTACK_UP_2** - Raises Attack by 2 stages (Swords Dance)
- **EFFECT_ATTACK_DOWN_HIT** - Damage + lower Attack (Aurora Beam, etc.)

---

## Moves Using This Effect

| Move | Type | Accuracy | Notes |
|------|------|----------|-------|
| Growl | Normal | 100 | Lowers Attack -1 |
| Charm | Normal | 100 | Lowers Attack -2 (different effect) |
| Feather Dance | Flying | 100 | Lowers Attack -2 |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [ ] Stat stage system designed
- [ ] Command sequence defined
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:516-554`, `src/data/battle_moves.h:588`
**Validation Notes:** Growl lowers Attack by 1 stage. Stat stages range from -6 to +6 with specific multipliers applied during damage calculation.
