# Effect: PARALYZE (Thunder Wave)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_PARALYZE` (pokeemerald: 67)
**Introduced:** Generation I
**Example Moves:** Thunder Wave, Stun Spore, Glare
**Category:** Status-only (no damage)

---

## Overview

This effect inflicts Paralysis status on the target. Unlike Ember which damages + burns, this is a pure status move with no damage component.

---
## Behavior Specification

### What This Effect Does

1. Check if move hits (accuracy check)
2. Attempt to apply Paralysis status
3. No damage is dealt

This is the first **status-only** move we're implementing - it tests the non-damage path through the battle system.

### Execution Steps

```
1. AccuracyCheck
   ├── Calculate hit chance = move.accuracy * accuracy_stage / evasion_stage
   ├── Roll random 0-99
   ├── If roll >= hit chance → move_failed = true, exit
   └── Continue

2. TryApplyParalysis(100)
   ├── Guard: if move_failed, return
   ├── Check Ground type immunity → return if Ground type
   ├── Check Limber ability → return if has ability
   ├── Check already statused → return if status1 != 0
   ├── Roll random 0-99 (with 100% chance, this always succeeds)
   ├── Set defender.status1 = PARALYSIS
   └── Return

3. (No CheckFaint - can't faint from status-only move)
```

### State Changes

| Domain | Field | Change | Condition |
|--------|-------|--------|-----------|
| PokemonState | `defender.status1` | Set to PARALYSIS | Move hits, not immune |

### Edge Cases

- **Ground type defender:** Immune to paralysis from Electric-type moves (Thunder Wave)
- **Limber ability:** Immune to paralysis
- **Already statused:** Cannot be paralyzed if already has any status
- **No damage:** This move deals 0 damage, so no HP changes
- **Accuracy:** Thunder Wave has 100 accuracy in Gen III (90 in later gens)

**Important Gen III mechanic:**
Thunder Wave is Electric-type, so Ground types are immune via type immunity (not just paralysis immunity). This is different from Stun Spore (Grass-type) which can paralyze Ground types.

---

## Paralysis Immunity Rules

```
Can this Pokemon be paralyzed?
├── Already has status (Sleep, Burn, Freeze, etc.)? → No
├── Move is Electric-type and target is Ground type? → No
├── Has Limber ability? → No
├── Safeguard active on defender's side? → No
└── Yes, can be paralyzed
```

---

## Pokeemerald Reference

### Battle Script

From `data/battle_scripts_1.s:684`:
```asm
BattleScript_EffectParalyze::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_LIMBER, BattleScript_LimberProtected
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	typecalc
	jumpifmovehadnoeffect BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_PARALYSIS, BattleScript_AlreadyParalyzed
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_PARALYSIS
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd
```

Key observations:
- Check order: Limber → Substitute → Type calc → Already paralyzed → Already statused → Accuracy → Safeguard
- `typecalc` + `jumpifmovehadnoeffect` handles Ground-type immunity
- Substitute blocks paralysis (unlike burn in Ember which can apply through sub - Gen III quirk)
- No damage is dealt

### Status Application

From `src/battle_script_commands.c`:
```c
case MOVE_EFFECT_PARALYSIS:
    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
        && gBattleMons[gEffectBattler].hp != 0
        && !gProtectStructs[gEffectBattler].confusionSelfDmg)
    {
        statusChanged = SetMoveEffect(FALSE, 0);
        if (statusChanged & STATUS_CHANGE_WORKED)
            gBattleMons[gEffectBattler].status1 = STATUS1_PARALYSIS;
    }
    break;
```

---

## Command Sequence

```cpp
void Effect_Paralyze(BattleContext& ctx) {
    AccuracyCheck(ctx);
    TryApplyParalysis(ctx, 100);  // 100% chance if it hits
    // No CheckFaint - status-only moves don't deal damage
}
```

**Key difference from Effect_BurnHit:**
- No `CalculateDamage()`
- No `ApplyDamage()`
- No `CheckFaint()`

This is a pure status application effect.

---

## TryApplyParalysis Implementation

```cpp
void TryApplyParalysis(BattleContext& ctx, uint8_t chance) {
    // Guard: skip if move already failed
    if (ctx.move_failed) return;

    // Ground type is immune to paralysis from Electric-type moves
    // TODO (Phase 1): We'll check Ground immunity when we add type effectiveness
    // For now, assume non-Electric paralysis moves (like Stun Spore)

    // Check immunities
    if (ctx.defender->HasAbility(Ability::Limber)) return;  // TODO: Phase 2
    if (ctx.defender->status1 != 0) return;  // Already statused

    // Check Safeguard
    // TODO: Phase 2

    // Roll for paralysis
    if (Random(100) < chance) {
        ctx.defender->status1 = STATUS1_PARALYSIS;
        // Message: "[Pokemon] was paralyzed!"
    }
}
```

---

## Test Cases

### Basic Functionality

```cpp
TEST(Effect_Paralyze, AppliesParalysis) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(defender.status1, STATUS1_PARALYSIS);
}

TEST(Effect_Paralyze, DoesNotDealDamage) {
    auto [attacker, defender] = SetupBattle();
    defender.current_hp = 100;

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(defender.current_hp, 100);  // No damage dealt
    EXPECT_EQ(ctx.damage_dealt, 0);
}

TEST(Effect_Paralyze, DoesNotSetFaintFlag) {
    auto [attacker, defender] = SetupBattle();

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_FALSE(defender.is_fainted);  // Status moves don't faint
}
```

### Immunities

```cpp
TEST(Effect_Paralyze, AlreadyStatusedCantParalyze) {
    auto [attacker, defender] = SetupBattle();
    defender.status1 = STATUS1_BURN;  // Already burned

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(defender.status1, STATUS1_BURN);  // Status unchanged
}

TEST(Effect_Paralyze, LimberImmune) {
    // TODO: Enable when abilities are implemented
    auto [attacker, defender] = SetupBattle();
    defender.ability = Ability::Limber;

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(defender.status1, 0);  // Not paralyzed
}

TEST(Effect_Paralyze, GroundTypeImmune) {
    // TODO: Enable when type effectiveness is implemented
    // Ground types are immune to Electric-type paralysis moves
    auto [attacker, defender] = SetupBattle();
    defender.type1 = Type::Ground;

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(defender.status1, 0);  // Not paralyzed
}
```

### Edge Cases

```cpp
TEST(Effect_Paralyze, MissDoesNotParalyze) {
    // TODO: Enable when accuracy formula is fully implemented
    auto [attacker, defender] = SetupBattle();
    SetRNG(99);  // Force miss

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_TRUE(ctx.move_failed);
    EXPECT_EQ(defender.status1, 0);  // Not paralyzed
}

TEST(Effect_Paralyze, DoesNotModifyAttacker) {
    auto [attacker, defender] = SetupBattle();

    uint16_t original_hp = attacker.current_hp;
    uint8_t original_status = attacker.status1;

    BattleContext ctx = CreateContext(attacker, defender, Move::ThunderWave);
    Effect_Paralyze(ctx);

    EXPECT_EQ(attacker.current_hp, original_hp);
    EXPECT_EQ(attacker.status1, original_status);
}
```

---

## Related Effects

- **EFFECT_BURN_HIT** - Damage + secondary burn (we just implemented this)
- **EFFECT_POISON** - Status-only poison (similar structure)
- **EFFECT_SLEEP** - Status-only sleep
- **EFFECT_PARALYZE_HIT** - Damage + secondary paralysis (e.g., Body Slam, Lick)

---

## Moves Using This Effect

| Move | Type | Accuracy | Power | Notes |
|------|------|----------|-------|-------|
| Thunder Wave | Electric | 100 | 0 | Ground types immune |
| Stun Spore | Grass | 75 | 0 | Can paralyze Ground types |
| Glare | Normal | 75 | 0 | Can paralyze Ground types |

---

## Implementation Status

- [x] Specification complete
- [x] Command sequence defined
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested
- [x] Validated against pokeemerald

---

**Last Updated:** 2025-11-20
**Pokeemerald Reference:** `data/battle_scripts_1.s`, `src/battle_script_commands.c`
**Validation Notes:** Thunder Wave is 100 accuracy in Gen III. Ground type immunity is via type chart, not paralysis-specific immunity.
