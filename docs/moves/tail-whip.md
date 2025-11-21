# Effect: DEFENSE_DOWN (Tail Whip)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_DEFENSE_DOWN` (pokeemerald: 19)
**Introduced:** Generation I
**Example Moves:** Tail Whip, Leer
**Category:** Stat Modification (no damage)

---

## Overview

This effect lowers the target's Defense stat by 1 stage. This mirrors Effect_AttackDown but targets Defense instead.

---
## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Lower target's Defense stat stage by 1
3. No damage is dealt

This reuses the **stat stage system** introduced with Growl, demonstrating that the system works for different stats.

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance = move.accuracy * accuracy_stage / evasion_stage
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. ModifyStatStage(STAT_DEF, -1)
   ├── Guard: if move_failed, return
   ├── Check current stat stage for Defense
   ├── If already at -6 (minimum) → show "won't go lower" message, return
   ├── Decrease stat stage by 1
   ├── Store new stage value
   └── Show message: "[Pokemon]'s Defense fell!"

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
| PokemonState | `defender.stat_stages[STAT_DEF]` | Decreased by 1 | Move hits, not at -6 |

### Edge Cases

- **Already at -6:** Cannot go lower, move fails with "won't go lower" message
- **Substitute:** Blocked by substitute (unlike burn which applies through sub)
- **No damage:** This move deals 0 damage, so no HP changes
- **Clear Body ability:** Prevents stat drops (future implementation)
- **Mist field effect:** Prevents stat drops (future implementation)

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:527`:
```asm
BattleScript_EffectDefenseDown::
	setstatchanger STAT_DEF, 1, TRUE
	goto BattleScript_EffectStatDown
```

This shows that Defense Down is identical to Attack Down, just targeting STAT_DEF instead of STAT_ATK. Both go through the common `BattleScript_EffectStatDown` routine.

Key observations:
- `setstatchanger STAT_DEF, 1, TRUE` - sets which stat to change (Defense), by how much (1), and direction (TRUE = down)
- Shares the same battle script flow as Attack Down
- Substitute blocks stat drops
- Accuracy check happens before stat change
- Message changes if stat won't go lower

### Move Data

From `src/data/battle_moves.h:510-520`:
```c
[MOVE_TAIL_WHIP] =
{
    .effect = EFFECT_DEFENSE_DOWN,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 100,
    .pp = 30,
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
void Effect_DefenseDown(BattleContext& ctx) {
    AccuracyCheck(ctx);
    ModifyStatStage(ctx, STAT_DEF, -1);  // Lower Defense by 1 stage
    // No damage, no faint check
}
```

This is nearly identical to Effect_AttackDown, just changing which stat is modified.

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_DefenseDown, LowersDefenseStage) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], -1);
}

TEST(Effect_DefenseDown, DoesNotDealDamage) {
    auto [attacker, defender] = SetupBattle();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.current_hp, original_hp);
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_DefenseDown, CanStackMultipleTimes) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);

    // Use Tail Whip 3 times
    Effect_DefenseDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_DefenseDown(ctx);
    ctx.move_failed = false;
    Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], -3);
}
```

### Edge Cases

```cpp
TEST(Effect_DefenseDown, MinimumStageMinus6) {
    auto [attacker, defender] = SetupBattle();
    defender.stat_stages[STAT_DEF] = -6;  // Already at minimum

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], -6);  // Stays at -6
    // TODO: Check for "won't go lower" message
}

TEST(Effect_DefenseDown, CanLowerFromPositiveStages) {
    auto [attacker, defender] = SetupBattle();
    defender.stat_stages[STAT_DEF] = 2;  // Boosted by Iron Defense

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], 1);  // Now at +1
}

TEST(Effect_DefenseDown, DoesNotModifyAttacker) {
    auto [attacker, defender] = SetupBattle();

    int8_t original_stage = attacker.stat_stages[STAT_DEF];
    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], original_stage);
}

TEST(Effect_DefenseDown, DoesNotAffectOtherStats) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::TailWhip);
    Effect_DefenseDown(ctx);

    // Only Defense should be lowered
    EXPECT_EQ(defender.stat_stages[STAT_DEF], -1);
    EXPECT_EQ(defender.stat_stages[STAT_ATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], 0);
}
```

### Integration with Damage

```cpp
TEST(StatStages, DefenseStageAffectsDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto move = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &move);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with -1 Defense
    defender2.stat_stages[STAT_DEF] = -1;
    auto ctx2 = SetupContext(&attacker, &defender2, &move);
    Effect_Hit(ctx2);
    uint16_t increased_damage = ctx2.damage_dealt;

    // With -1 Defense, damage should be increased (defender has 2/3 effective defense)
    EXPECT_GT(increased_damage, normal_damage);
    // Damage = Attack / Defense, so if Defense is 2/3, damage is 3/2 (1.5x)
    EXPECT_NEAR(increased_damage, normal_damage * 3 / 2, 2);  // Allow 2 damage rounding
}
```

---

## Related Effects

- **EFFECT_ATTACK_DOWN** - Lowers Attack (Growl)
- **EFFECT_SPEED_DOWN** - Lowers Speed (String Shot)
- **EFFECT_DEFENSE_UP** - Raises Defense (Withdraw, Defense Curl)
- **EFFECT_DEFENSE_UP_2** - Raises Defense by 2 stages (Iron Defense)
- **EFFECT_DEFENSE_DOWN_HIT** - Damage + lower Defense (Crush Claw, etc.)

---

## Moves Using This Effect

| Move | Type | Accuracy | Notes |
|------|------|----------|-------|
| Tail Whip | Normal | 100 | Lowers Defense -1 |
| Leer | Normal | 100 | Lowers Defense -1 |

---

## Implementation Status

- [x] Specification complete
- [x] Cross-referenced with pokeemerald
- [x] Tests written (9 tests)
- [x] Implementation complete
- [x] Integration tested

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s:527`, `src/data/battle_moves.h:510-520`
**Validation Notes:** Tail Whip lowers Defense by 1 stage. Functionally identical to Growl but targets STAT_DEF instead of STAT_ATK.
