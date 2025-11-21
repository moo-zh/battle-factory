# Effect: SPECIAL_ATTACK_UP_2 (Tail Glow)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SPECIAL_ATTACK_UP_2` (pokeemerald: 53)
**Introduced:** Generation III
**Example Moves:** Tail Glow, Nasty Plot
**Category:** Status (Self-targeting stat boost)

---

## Overview

Tail Glow is a **self-targeting status move** that raises the user's Special Attack stat by 2 stages. It's one of the strongest special attack boosting moves in the game, providing a significant offensive boost in a single turn.

**Key Concept:** Self-targeting stat boost (+2 stages)

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_TAIL_GLOW] = {
    .effect = EFFECT_SPECIAL_ATTACK_UP_2,
    .power = 0,
    .type = TYPE_BUG,
    .accuracy = 100,  // Self-targeting, doesn't check accuracy
    .pp = 20,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,
    .priority = 0,
}
```

### Execution Steps

```
1. No accuracy check (self-targeting move)
2. ModifyStatStage(STAT_SPATK, +2, affects_user=true)
3. Stat stage clamped at +6 (maximum)
4. No damage dealt
5. No CheckFaint
```

---

## State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon (user) | `stat_stages[STAT_SPATK]` | +2 (clamped at +6) |
| Pokemon (target) | - | No change |

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:57` - EFFECT_SPECIAL_ATTACK_UP_2 definition (effect 53)
- `src/data/battle_moves.h:3825-3834` - Tail Glow move data
- `data/battle_scripts_1.s` - Effect implementation (shares code with other stat boost moves)

**Key Mechanics:**
- Self-targeting move (MOVE_TARGET_USER)
- Raises Sp. Attack by 2 stages
- Cannot miss (accuracy check skipped for self-targeting)
- Stat stages cap at +6

---

## Implementation Notes

- Uses shared `ModifyStatStage` command with `affects_user=true`
- No accuracy check needed (self-targeting)
- Effect is reused by: Nasty Plot (Dark type, Gen IV)
- Similar pattern to: Swords Dance (Attack +2), Agility (Speed +2), Amnesia (Sp. Defense +2)

---

## Example Moves Using This Effect

- **Tail Glow** (Bug type, 20 PP) - Signature move of Manaphy/Xurkitree
- **Nasty Plot** (Dark type, 20 PP) - Gen IV move, same effect

---

## Related Effects

- `EFFECT_SPECIAL_ATTACK_UP` - Raises Sp. Attack by 1 stage (e.g., Meditate)
- `EFFECT_ATTACK_UP_2` - Raises Attack by 2 stages (Swords Dance)
- `EFFECT_SPECIAL_DEFENSE_UP_2` - Raises Sp. Defense by 2 stages (Amnesia)
