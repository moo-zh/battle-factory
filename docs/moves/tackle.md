# Effect: HIT (Tackle)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_HIT` (pokeemerald: 0)
**Introduced:** Generation I
**Example Moves:** Tackle, Pound, Scratch, Vine Whip, Water Gun
**Category:** Damage

---

## Overview

The simplest damaging move effect. Performs an accuracy check, calculates damage using the standard Gen III formula, applies it to the target, and checks for faint. No secondary effects.

This is the **foundation effect** - almost all other damaging effects build on this pattern by adding status, stat changes, or other side effects.

---

## Behavior Specification

### What This Effect Does

Executes a basic damaging attack with no secondary effects.

### Execution Steps

```
1. Check accuracy
   - Roll against move's accuracy value
   - Account for accuracy/evasion stat stages
   - If miss, move fails and ends
2. Calculate damage
   - Use Gen III damage formula with:
     * Attacker's Attack or Special Attack (based on move type)
     * Defender's Defense or Special Defense
     * Move power
     * STAB, type effectiveness, weather, items, abilities
     * Critical hit chance
     * Random variance (85-100%)
3. Apply damage to target
   - Reduce target's current HP
   - Clamp to 0 (no negative HP)
4. Check if target fainted
   - Set faint flag if HP = 0
   - Actual faint processing happens after move ends
```

### State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon (defender) | `current_hp` | Reduced by calculated damage |
| Pokemon (defender) | `faint_flag` | Set to true if HP reaches 0 |
| BattleContext | `damage_dealt` | Set to calculated damage value |
| BattleContext | `critical_hit` | Set to true if crit occurred |
| BattleContext | `effectiveness` | Set to type effectiveness multiplier |
| BattleContext | `move_failed` | Set to true if accuracy check fails |

### Edge Cases

- **Miss:** If accuracy check fails, no damage is calculated or applied
- **Type immunity:** Effectiveness = 0, damage = 0, but move still "hits" (triggers contact effects)
- **Substitute:** Damage goes to substitute instead of Pokemon (handled in ApplyDamage)
- **Already fainted:** Move shouldn't execute (pre-action check prevents this)

## Pokeemerald Reference

From `data/battle_scripts_1.s:170` (BattleScript_EffectHit):

```asm
BattleScript_EffectHit::
	jumpifnotmove MOVE_SURF, BattleScript_HitFromAtkCanceler
	jumpifnostatus3 BS_TARGET, STATUS3_UNDERWATER, BattleScript_HitFromAtkCanceler
	orword gHitMarker, HITMARKER_IGNORE_UNDERWATER
	setbyte sDMG_MULTIPLIER, 2
BattleScript_HitFromAtkCanceler::
	attackcanceler                           # Pre-move checks (flinch, etc.)
BattleScript_HitFromAccCheck::
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE  # Accuracy roll
BattleScript_HitFromAtkString::
	attackstring                             # Display "[Pokemon] used [Move]!"
	ppreduce                                 # Decrement PP
BattleScript_HitFromCritCalc::
	critcalc                                 # Critical hit check
	damagecalc                               # Gen III damage formula
	typecalc                                 # Type effectiveness
	adjustnormaldamage                       # Apply modifiers
BattleScript_HitFromAtkAnimation::
	attackanimation                          # Show move animation
	waitanimation
	effectivenesssound                       # "Super effective!" etc.
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET                # Visual HP bar update
	datahpupdate BS_TARGET                   # Actual HP update
	critmessage                              # "A critical hit!"
	waitmessage B_WAIT_TIME_LONG
	resultmessage                            # Effectiveness message
	waitmessage B_WAIT_TIME_LONG
	seteffectwithchance                      # No effect for basic Hit
	tryfaintmon BS_TARGET                    # Check and process faint
BattleScript_MoveEnd::
	moveendall
	end
```

From `pokeemerald-ref/src/data/battle_moves.h`:

```c
[MOVE_TACKLE] =
{
    .effect = EFFECT_HIT,
    .power = 35,                    // Updated to 40 in Gen V+, but Gen III uses 35
    .type = TYPE_NORMAL,
    .accuracy = 95,                  // 95% accuracy (not 100!)
    .pp = 35,
    .secondaryEffectChance = 0,      // No secondary effect
    .target = MOVE_TARGET_SELECTED,
    .priority = 0,                   // Normal priority
    .flags = FLAG_MAKES_CONTACT      // Physical contact
           | FLAG_PROTECT_AFFECTED   // Blocked by Protect
           | FLAG_MIRROR_MOVE_AFFECTED
           | FLAG_KINGS_ROCK_AFFECTED,
}
```

Key observations:
- **Tackle is 35 power in Gen III**, not 40 (that's Gen V+)
- **95% accuracy**, not 100% - important for testing!
- Surf has special handling (double damage vs. Pokemon underwater via Dive) - we can ignore this initially
- The `attackcanceler` step handles flinch/freeze/sleep checks - our Engine does this in pre-action phase
- PP reduction happens AFTER accuracy check but BEFORE damage calculation

## Command Sequence

```cpp
void Effect_Hit(BattleContext& ctx) {
    AccuracyCheck(ctx);
    if (ctx.move_failed) return;

    CalculateDamage(ctx);
    ApplyDamage(ctx);
    CheckFaint(ctx);
}
```

### Implementation Notes for Pass 1 (Minimal)

For the first implementation, we'll simplify:

**AccuracyCheck (simplified):**
- Always succeeds (skip the actual accuracy roll)
- TODO: Add real accuracy formula when implementing Thunder Wave

**CalculateDamage (simplified):**
- Skip stat stages (use base stats directly)
- Skip critical hits
- Skip type effectiveness (assume 1x)
- Skip STAB
- Skip weather/ability/item modifiers
- Skip random variance (deterministic damage)
- Formula: `damage = (22 * power * attack / defense) / 50 + 2`

**ApplyDamage (minimal):**
- Subtract damage from HP
- Clamp to 0

**CheckFaint (minimal):**
- Set faint flag if HP = 0

This gives us a working foundation. Each subsequent move will add ONE missing feature.

## Test Cases

```cpp
TEST(Effect_Hit, BasicDamage) {
    // Setup: Charmander (52 Atk) vs Bulbasaur (49 Def)
    // Tackle: 35 power
    // Expected (simplified): ~16 damage
    auto ctx = SetupContext(Charmander, Bulbasaur, Move::Tackle);
    Effect_Hit(ctx);
    EXPECT_LT(ctx.defender->current_hp, ctx.defender->max_hp);
    EXPECT_GT(ctx.damage_dealt, 0);
}

TEST(Effect_Hit, DamageScalesWithAttack) {
    // Higher attack = more damage
    auto ctx1 = SetupContext(LowAttackMon, StandardDefense, Move::Tackle);
    Effect_Hit(ctx1);

    auto ctx2 = SetupContext(HighAttackMon, StandardDefense, Move::Tackle);
    Effect_Hit(ctx2);

    EXPECT_LT(ctx1.damage_dealt, ctx2.damage_dealt);
}

TEST(Effect_Hit, DamageScalesWithDefense) {
    // Higher defense = less damage
    auto ctx1 = SetupContext(StandardAttack, LowDefenseMon, Move::Tackle);
    Effect_Hit(ctx1);

    auto ctx2 = SetupContext(StandardAttack, HighDefenseMon, Move::Tackle);
    Effect_Hit(ctx2);

    EXPECT_GT(ctx1.damage_dealt, ctx2.damage_dealt);
}

TEST(Effect_Hit, CanKO) {
    // Weak defender with 1 HP should faint
    auto ctx = SetupContext(Charmander, WeakBulbasaur(1), Move::Tackle);
    Effect_Hit(ctx);
    EXPECT_EQ(ctx.defender->current_hp, 0);
    EXPECT_TRUE(ctx.defender->is_fainted);
}

TEST(Effect_Hit, MinimumDamage) {
    // Even extreme defense should take at least 1 damage
    auto ctx = SetupContext(WeakAttacker, MaxDefense, Move::Tackle);
    Effect_Hit(ctx);
    EXPECT_GE(ctx.damage_dealt, 1);
}

// TODO: Add these tests when implementing accuracy
// TEST(Effect_Hit, CanMiss)
// TEST(Effect_Hit, MissDealsNoDamage)

// TODO: Add these tests when implementing type effectiveness
// TEST(Effect_Hit, TypeImmunity)
// TEST(Effect_Hit, SuperEffective)
// TEST(Effect_Hit, NotVeryEffective)

// TODO: Add when implementing critical hits
// TEST(Effect_Hit, CriticalHit)
```

## Related Effects

- `EFFECT_BURN_HIT` - Same as Hit, but 10% chance to burn
- `EFFECT_FREEZE_HIT` - Same as Hit, but 10% chance to freeze
- `EFFECT_PARALYZE_HIT` - Same as Hit, but 10% chance to paralyze
- `EFFECT_POISON_HIT` - Same as Hit, but 30% chance to poison
- `EFFECT_FLINCH_HIT` - Same as Hit, but 10% chance to flinch

All of these effects are `Effect_Hit` + one additional command for the secondary effect.

## Implementation Strategy

### Phase 1: Minimal (Today)
- Hardcoded/simplified damage formula
- No accuracy roll (always hits)
- No type effectiveness
- No critical hits
- **Goal:** See damage happen, verify structure works

### Phase 2: Add Accuracy (When implementing Thunder Wave)
- Real accuracy formula with stat stages
- Test misses vs. hits

### Phase 3: Add Critical Hits (When implementing high-crit moves)
- Crit chance calculation
- 2x damage multiplier
- Ignoring stat stages

### Phase 4: Add Type System (When implementing type-diverse moves)
- Type effectiveness lookup
- STAB calculation
- Immunities

### Phase 5: Add Modifiers (When implementing weather/abilities)
- Weather boost/nerf
- Ability multipliers
- Item multipliers
- Random variance

Each phase adds features to `CalculateDamage` without changing `Effect_Hit` - that's the power of the command pattern.

## Implementation Status

- [x] Specification complete
- [x] Pokeemerald cross-referenced
- [x] Command sequence defined
- [ ] Tests written
- [ ] Minimal implementation complete
- [ ] Integration tested
- [ ] Full Gen III accuracy (deferred to later moves)
