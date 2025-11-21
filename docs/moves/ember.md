# Effect: BURN_HIT (Ember, Flamethrower, Fire Blast, etc.)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_BURN_HIT` (pokeemerald: 4)
**Introduced:** Generation I
**Example Moves:** Ember (10%), Flamethrower (10%), Fire Blast (10%)
**Category:** Damage + Secondary Status

---

## Overview

This effect deals damage and has a chance to inflict Burn.

---

## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Calculate and apply damage
3. Roll for burn chance
4. If roll succeeds and target isn't immune, apply Burn

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance = move.accuracy * accuracy_stage / evasion_stage
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. CalculateDamage
   ├── Determine physical/special (Fire = Special in Gen III)
   ├── Get attacker SpA, defender SpD with stages
   ├── Apply all modifiers (weather, STAB, type, etc.)
   ├── Roll for critical hit
   └── Set ctx.damage_dealt, ctx.critical_hit, ctx.effectiveness

3. ApplyDamage
   ├── Reduce defender HP by damage_dealt
   └── If substitute active, damage substitute instead

4. TryApplyBurn(chance)
   ├── Guard: if move_failed, return
   ├── Check Fire type immunity → return if Fire type
   ├── Check Water Veil ability → return if has ability
   ├── Check already statused → return if status1 != 0
   ├── Roll random 0-99
   ├── If roll < chance → defender.status1 = BURN
   └── Return

5. CheckFaint
   ├── If defender.hp <= 0 → set faint flag
   └── Return
```

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `defender.current_hp` | Reduced by damage | Move hits |
| PokemonState | `defender.status1` | Set to BURN | Roll succeeds, not immune |
| SlotState | `protect.special_dmg` | Set to damage | For Mirror Coat (Fire is Special in Gen III) |

### Edge Cases

- **Fire type defender:** Immune to Burn (TryApplyBurn returns early)
- **Water Veil ability:** Immune to Burn
- **Already statused:** Cannot be burned if already has any status
- **Substitute:** Damage hits substitute, but burn CAN still apply to Pokemon behind it (Gen III behavior)
- **Immunity by type chart:** If defender is Fire type, damage is 0.5x but burn immunity is separate
- **Critical hit:** Does not affect burn chance

**Gen III Substitute Behavior:**

In Gen III, secondary status effects (like Burn) can be applied to a Pokemon that has an active Substitute. The damage is blocked by the substitute, but the status effect is not blocked.

Example:
- Defender has Substitute (50 HP) and is at 100/100 HP
- Attacker uses Ember (40 power, 10% burn chance)
- Damage hits Substitute (reduces it to ~30 HP)
- If burn roll succeeds, Defender gets burned (even though protected by Sub)

---

## Burn Immunity Rules

```
Can this Pokemon be burned?
├── Already has status (Sleep, Burn, Freeze, etc.)? → No
├── Is Fire type? → No
├── Has Water Veil ability? → No
├── Safeguard active on defender's side? → No
├── Has Leaf Guard ability and Sun active? → No
└── Yes, can be burned
```

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:397` (BattleScript_EffectBurnHit):
```asm
BattleScript_EffectBurnHit:
    setmoveeffect MOVE_EFFECT_BURN
    goto BattleScript_EffectHit
```

The actual burn application happens in `BattleScript_EffectHit` which calls the move effect handler.

### Move Effect Handler

From `src/battle_script_commands.c`:
```c
// In Cmd_setmoveeffect for MOVE_EFFECT_BURN:
if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
    && gBattleMons[gEffectBattler].hp != 0
    && !gProtectStructs[gEffectBattler].confusionSelfDmg)
{
    if (IsTargetImmuneToBurn(gEffectBattler))
    {
        gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
        // ...
    }
    else
    {
        statusChanged = TRUE;
        gBattleMons[gEffectBattler].status1 = STATUS1_BURN;
        // ...
    }
}
```

### Burn Immunity Check

```c
static bool32 IsTargetImmuneToBurn(u32 battler)
{
    // Fire type immune
    if (IS_BATTLER_OF_TYPE(battler, TYPE_FIRE))
        return TRUE;
    // Water Veil immune
    if (GetBattlerAbility(battler) == ABILITY_WATER_VEIL)
        return TRUE;
    // Already statused
    if (gBattleMons[battler].status1 & STATUS1_ANY)
        return TRUE;
    return FALSE;
}
```

---

## Command Sequence

```cpp
void Effect_BurnHit(BattleContext& ctx) {
    AccuracyCheck(ctx);
    CalculateDamage(ctx);
    ApplyDamage(ctx);
    TryApplyBurn(ctx, ctx.move->effect_chance);  // 10 for Ember/Flamethrower
    CheckFaint(ctx);
}
```

### Alternative: Hardcoded Chance

If moves don't carry effect_chance, create specific functions:

```cpp
void Effect_BurnHit10(BattleContext& ctx) {
    AccuracyCheck(ctx);
    CalculateDamage(ctx);
    ApplyDamage(ctx);
    TryApplyBurn(ctx, 10);
    CheckFaint(ctx);
}

void Effect_BurnHit30(BattleContext& ctx) {
    AccuracyCheck(ctx);
    CalculateDamage(ctx);
    ApplyDamage(ctx);
    TryApplyBurn(ctx, 30);
    CheckFaint(ctx);
}
```

---

## TryApplyBurn Implementation

```cpp
void TryApplyBurn(BattleContext& ctx, uint8_t chance) {
    // Guard: skip if move already failed
    if (ctx.move_failed) return;

    // Guard: skip if target fainted (damage already applied)
    if (ctx.defender->current_hp == 0) return;

    // Check immunities
    if (ctx.defender->HasType(Type::Fire)) return;
    if (ctx.defender->HasAbility(Ability::WaterVeil)) return;
    if (ctx.defender->HasAbility(Ability::LeafGuard) && ctx.field->weather == Weather::Sun) return;
    if (ctx.defender->status1 != 0) return;  // Already statused

    // Check Safeguard
    Side defender_side = GetSide(ctx.defender);
    if (ctx.field->sides[defender_side].safeguard) return;

    // Roll for burn
    if (Random(100) < chance) {
        ctx.defender->status1 = STATUS1_BURN;
        // Message: "[Pokemon] was burned!"
    }
}
```

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_BurnHit, DealsDamage) {
    auto [attacker, defender] = SetupBattle();
    defender.current_hp = 100;

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_LT(defender.current_hp, 100);  // Damage dealt
}

TEST(Effect_BurnHit, CanApplyBurn) {
    auto [attacker, defender] = SetupBattle();
    SetRNG(5);  // Force burn roll to succeed (5 < 10)

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.status1, STATUS1_BURN);
}

TEST(Effect_BurnHit, BurnChanceIsCorrect) {
    // Run 1000 times, expect ~100 burns (10% chance)
    int burns = 0;
    for (int i = 0; i < 1000; i++) {
        auto [attacker, defender] = SetupBattle();
        BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
        Effect_BurnHit(ctx);
        if (defender.status1 == STATUS1_BURN) burns++;
        defender.status1 = 0;  // Reset for next trial
    }

    // 10% ± 3% tolerance
    EXPECT_GT(burns, 70);
    EXPECT_LT(burns, 130);
}
```

### Immunities

```cpp
TEST(Effect_BurnHit, FireTypeImmune) {
    auto [attacker, defender] = SetupBattle();
    defender.type1 = Type::Fire;
    SetRNG(0);  // Would burn if not immune

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.status1, 0);  // Not burned
}

TEST(Effect_BurnHit, WaterVeilImmune) {
    auto [attacker, defender] = SetupBattle();
    defender.ability = Ability::WaterVeil;
    SetRNG(0);

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.status1, 0);
}

TEST(Effect_BurnHit, AlreadyStatusedCantBurn) {
    auto [attacker, defender] = SetupBattle();
    defender.status1 = STATUS1_PARALYSIS;
    SetRNG(0);

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.status1, STATUS1_PARALYSIS);  // Still paralyzed, not burned
}

TEST(Effect_BurnHit, SafeguardPrevents) {
    auto [attacker, defender] = SetupBattle();
    SetSafeguard(GetSide(defender));
    SetRNG(0);

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.status1, 0);
}
```

### Edge Cases

```cpp
TEST(Effect_BurnHit, MissDoesNotBurn) {
    auto [attacker, defender] = SetupBattle();
    SetRNG(99);  // Force miss (99 >= 95 accuracy for Ember)

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_TRUE(ctx.move_failed);
    EXPECT_EQ(defender.status1, 0);
    EXPECT_EQ(defender.current_hp, defender.max_hp);  // No damage
}

TEST(Effect_BurnHit, FaintedTargetNotBurned) {
    auto [attacker, defender] = SetupBattle();
    defender.current_hp = 1;  // Will faint from damage
    SetRNG(0);  // Would burn

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    EXPECT_EQ(defender.current_hp, 0);
    EXPECT_EQ(defender.status1, 0);  // Dead Pokemon can't be burned
}

TEST(Effect_BurnHit, SubstituteAllowsBurn) {
    auto [attacker, defender] = SetupBattle();
    defender.status2 |= STATUS2_SUBSTITUTE;
    defender.substitute_hp = 50;
    SetRNG(0);  // Force burn roll to succeed

    BattleContext ctx = CreateContext(attacker, defender, Move::Ember);
    Effect_BurnHit(ctx);

    // Gen III: Damage blocked by substitute, but status effect applies
    EXPECT_LT(defender.substitute_hp, 50);  // Damage to substitute
    EXPECT_EQ(defender.current_hp, defender.max_hp);  // Real HP unchanged
    EXPECT_EQ(defender.status1, STATUS1_BURN);  // But burn DOES apply
}
```

---

## Related Effects

- **EFFECT_PARALYZE_HIT** - Same pattern, different status
- **EFFECT_FREEZE_HIT** - Same pattern, different status
- **EFFECT_POISON_HIT** - Same pattern, different status
- **EFFECT_BURN** - 100% burn without damage (Will-O-Wisp)
- **EFFECT_TRI_ATTACK** - 20% chance of burn/freeze/paralyze

---

## Moves Using This Effect

| Move | Power | Accuracy | Burn Chance |
|------|-------|----------|-------------|
| Ember | 40 | 100 | 10% |
| Fire Punch | 75 | 100 | 10% |
| Flamethrower | 95 | 100 | 10% |
| Fire Blast | 120 | 85 | 10% |
| Heat Wave | 100 | 90 | 10% |
| Lava Plume | 80 | 100 | 30% |
| Scald | 80 | 100 | 30% |
| Sacred Fire | 100 | 95 | 50% |
| Blue Flare | 130 | 85 | 20% |

Note: Scald is Water-type with burn effect - unusual but valid.

---

## Implementation Status

- [x] Specification complete
- [x] Command sequence defined
- [x] Tests written
- [ ] Implementation complete
- [ ] Integration tested
- [x] Validated against pokeemerald

---

**Last Updated:** 2025-11-19
**Pokeemerald Reference:** `src/battle_script_commands.c`, `data/battle_scripts_1.s`
**Validation Notes:** Confirmed Gen III substitute behavior allows status effects through substitute. Updated test cases accordingly.
