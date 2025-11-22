# Effect: SEMI_INVULNERABLE (Fly)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SEMI_INVULNERABLE` (pokeemerald: 155)
**Introduced:** Generation I
**Example Moves:** Fly, Dig, Bounce, Dive
**Category:** Damage (Two-turn move with semi-invulnerable state)

---

## Overview

Fly is a **two-turn move** that makes the user **semi-invulnerable** during the first turn. On Turn 1, the user flies into the air (consuming PP). On Turn 2, the user attacks from the air with high power. While airborne, the user **avoids most attacks** except specific moves that can hit airborne targets.

This introduces the **semi-invulnerable mechanic**, which is shared by moves like Dig (underground), Dive (underwater), and Bounce (airborne).

**Key Difference from Solar Beam:**
- Solar Beam: User is vulnerable while charging
- Fly: User is **invulnerable** while airborne (can't be targeted by most moves)

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_FLY] = {
    .effect = EFFECT_SEMI_INVULNERABLE,
    .power = 70,
    .type = TYPE_FLYING,
    .accuracy = 95,
    .pp = 15,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_SELECTED,
    .priority = 0,
    .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
}
```

### Battle Script Flow

**Turn 1 (Fly up):**
1. Check if STATUS2_MULTIPLETURNS is set → if yes, go to Turn 2
2. If not airborne yet:
   - Set STATUS2_MULTIPLETURNS flag
   - Set STATUS3_ON_AIR flag (semi-invulnerable)
   - Display message: "[Pokémon] flew up high!"
   - Consume PP
   - End turn (no damage dealt, user is now airborne)

**Turn 2 (Attack):**
1. STATUS2_MULTIPLETURNS is already set → proceed to attack
2. Clear STATUS2_MULTIPLETURNS flag
3. Clear STATUS3_ON_AIR flag (no longer semi-invulnerable)
4. Execute normal damage calculation (AccuracyCheck → CalculateDamage → ApplyDamage)
5. PP is NOT consumed (HITMARKER_NO_PPDEDUCT set)

**Semi-Invulnerable State (Turn 1 → Turn 2):**
- While STATUS3_ON_AIR is set, most moves **cannot target** the user
- Moves that bypass semi-invulnerable state:
  - Gust (2x damage vs airborne)
  - Twister (2x damage vs airborne)
  - Thunder (bypasses invulnerability)
  - Sky Uppercut (bypasses invulnerability)
  - Hurricane (bypasses invulnerability)
  - Whirlwind, Roar (force switch)

---

## State Tracking

Fly uses TWO volatile status flags:

```c
#define STATUS2_MULTIPLETURNS (1 << 12)  // Two-turn move active
#define STATUS3_ON_AIR        (1 << 6)   // Airborne (semi-invulnerable)
```

**STATUS2_MULTIPLETURNS:**
- Shared with all two-turn moves (Solar Beam, Razor Wind, etc.)
- Indicates a two-turn move is in progress

**STATUS3_ON_AIR:**
- Specific to airborne moves (Fly, Bounce)
- Makes the user **semi-invulnerable** (most moves can't hit)
- Checked by AccuracyCheck/targeting logic to determine if move can hit

**Other semi-invulnerable flags (for future):**
- STATUS3_UNDERGROUND (Dig)
- STATUS3_UNDERWATER (Dive)

---

## Implementation Requirements

### New Pokemon State Fields

Add to `battle::state::Pokemon`:

```cpp
// Semi-invulnerable state
bool is_semi_invulnerable;  // Volatile flag: currently semi-invulnerable (in air, underground, underwater)
SemiInvulnerableType semi_invulnerable_type;  // Which type (OnAir, Underground, Underwater)
```

**SemiInvulnerableType enum:**
```cpp
enum class SemiInvulnerableType : uint8_t {
    None = 0,
    OnAir,         // Fly, Bounce
    Underground,   // Dig
    Underwater     // Dive
};
```

### Effect Implementation

**Effect_Fly(BattleContext& ctx):**

```
Turn 1 Logic (Fly up):
1. Check if attacker->is_charging is false
   - If false: Begin flying
     - Set attacker->is_charging = true
     - Set attacker->charging_move = Move::Fly
     - Set attacker->is_semi_invulnerable = true
     - Set attacker->semi_invulnerable_type = SemiInvulnerableType::OnAir
     - Set ctx.move_failed = false (move succeeded in starting)
     - Return (end turn, no damage)

Turn 2 Logic (Attack from air):
2. Check if attacker->is_charging is true
   - If true: Execute attack
     - Clear attacker->is_charging = false
     - Clear attacker->is_semi_invulnerable = false
     - Clear attacker->semi_invulnerable_type = SemiInvulnerableType::None
     - Call AccuracyCheck
     - Call CalculateDamage
     - Call ApplyDamage
     - Call CheckFaint
```

### AccuracyCheck Modification (Future)

When implementing moves that can hit airborne targets, modify AccuracyCheck:

```cpp
// Before accuracy roll, check if target is semi-invulnerable
if (ctx.defender->is_semi_invulnerable) {
    // Check if current move can bypass semi-invulnerable state
    bool can_hit = CanHitSemiInvulnerable(ctx.move, ctx.defender->semi_invulnerable_type);
    if (!can_hit) {
        ctx.move_failed = true;
        return;  // Move cannot hit semi-invulnerable target
    }
}
```

**Note:** This AccuracyCheck modification is **deferred** until we implement a move that can hit airborne targets (Gust, Thunder, etc.). For now, we'll test Fly in isolation without defenders using Fly.

---

## Test Cases

### Basic Two-Turn Behavior

1. **Turn 1: Fly up (semi-invulnerable)**
   - Fly starts, user flies up
   - No damage dealt to defender
   - ctx.move_failed = false (move succeeded)
   - attacker->is_charging = true
   - attacker->is_semi_invulnerable = true
   - attacker->semi_invulnerable_type = SemiInvulnerableType::OnAir

2. **Turn 2: Attack from air**
   - Fly executes after flying up
   - Damage dealt to defender
   - attacker->is_charging = false (charging cleared)
   - attacker->is_semi_invulnerable = false (invulnerability cleared)
   - ctx.damage_dealt > 0

3. **No Damage on Turn 1**
   - Defender HP unchanged after Turn 1
   - Defender HP decreased after Turn 2

### Semi-Invulnerable Mechanics

4. **User is Semi-Invulnerable During Turn 1**
   - After Turn 1, attacker->is_semi_invulnerable == true
   - After Turn 2, attacker->is_semi_invulnerable == false

5. **Semi-Invulnerable Type is OnAir**
   - After Turn 1, attacker->semi_invulnerable_type == SemiInvulnerableType::OnAir
   - After Turn 2, attacker->semi_invulnerable_type == SemiInvulnerableType::None

### Edge Cases (Shared with Solar Beam)

6. **Interrupted Charging**
   - If attacker faints during Turn 1, charging is lost
   - If attacker switches out, charging is reset

7. **Accuracy Check Only on Turn 2**
   - Accuracy is checked when attack executes (Turn 2)
   - Not checked during fly-up turn (Turn 1)

8. **Miss After Flying**
   - If Fly misses on Turn 2, charging is still consumed
   - No damage dealt, is_charging cleared, is_semi_invulnerable cleared

9. **Protection During Turn 2**
   - If defender uses Protect on Turn 2, Fly is blocked
   - Charging is consumed, no damage dealt

### Stat Interactions

10. **Critical Hits**
    - Fly can crit on Turn 2 (normal damage calculation)

11. **Type Effectiveness**
    - Flying-type move, double damage vs Bug/Grass/Fighting
    - Half damage vs Rock/Steel/Electric

12. **Stat Stages Apply on Turn 2**
    - Attack stages affect damage calculation on Turn 2

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:159` - EFFECT_SEMI_INVULNERABLE definition
- `data/battle_scripts_1.s:1973-2003` - BattleScript_EffectSemiInvulnerable
- `data/battle_scripts_1.s:1990-1995` - BattleScript_FirstTurnFly
- `data/battle_scripts_1.s:1997-2003` - BattleScript_SecondTurnSemiInvulnerable
- `src/battle_script_commands.c:9009-9026` - Cmd_setsemiinvulnerablebit
- `src/data/battle_moves.h:250-262` - Fly move data
- `include/constants/battle.h:163-164,178` - STATUS3_ON_AIR, STATUS3_SEMI_INVULNERABLE

**Key Mechanics:**
- STATUS2_MULTIPLETURNS flag (bit 12 of status2) - shared with Solar Beam
- STATUS3_ON_AIR flag (bit 6 of status3) - airborne state
- STATUS3_SEMI_INVULNERABLE composite flag (ON_AIR | UNDERGROUND | UNDERWATER)
- HITMARKER_CHARGING flag
- HITMARKER_NO_PPDEDUCT on Turn 2
- B_MSG_TURN1_FLY message ID
- setsemiinvulnerablebit command
- clearsemiinvulnerablebit command

**Move-Specific Behavior:**
```c
switch (gCurrentMove) {
case MOVE_FLY:
case MOVE_BOUNCE:
    gStatuses3[gBattlerAttacker] |= STATUS3_ON_AIR;
    break;
case MOVE_DIG:
    gStatuses3[gBattlerAttacker] |= STATUS3_UNDERGROUND;
    break;
case MOVE_DIVE:
    gStatuses3[gBattlerAttacker] |= STATUS3_UNDERWATER;
    break;
}
```

---

## Differences from Solar Beam

| Feature | Solar Beam | Fly |
|---------|-----------|-----|
| Turn 1 State | Charging (vulnerable) | Airborne (semi-invulnerable) |
| Can be hit Turn 1? | Yes (all moves) | No (except Gust, Thunder, etc.) |
| Weather interaction | Sunny Day skips charge | None |
| Status flag | STATUS2_MULTIPLETURNS only | STATUS2_MULTIPLETURNS + STATUS3_ON_AIR |

---

## Notes

- This is the **first semi-invulnerable move** implementation
- Establishes pattern for Dig (underground), Dive (underwater), Bounce (airborne)
- Semi-invulnerable bypassing moves (Gust, Thunder, etc.) deferred until needed
- AccuracyCheck modification deferred until we have moves that can hit airborne targets
- Move-locking (can't switch moves mid-charge) inherited from Solar Beam pattern
- PP consumption: Turn 1 only (Turn 2 uses HITMARKER_NO_PPDEDUCT)

---

## Implementation Checklist

- [x] Add is_semi_invulnerable and semi_invulnerable_type fields to Pokemon struct
- [x] Add SemiInvulnerableType enum
- [x] Write failing tests for all test cases
- [x] Implement Effect_Fly with two-turn + semi-invulnerable logic
- [x] Initialize new fields in test helpers
- [x] Add Fly to Move enum and MOVE_DATABASE
- [x] Add Effect_Fly to EFFECT_DISPATCH table
- [x] All tests passing (414/414)
- [x] Mark specification as implemented

**Implementation Status:** ✅ Complete (2025-11-21)
