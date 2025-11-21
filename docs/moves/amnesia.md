# Effect: SPECIAL_DEFENSE_UP_2 (Amnesia)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SPECIAL_DEFENSE_UP_2` (pokeemerald: 54)
**Introduced:** Generation I
**Example Moves:** Amnesia, Calm Mind (also raises Sp. Attack)
**Category:** Status (Self-targeting stat boost)

---

## Overview

Amnesia is a **self-targeting status move** that sharply raises the user's Special Defense stat by 2 stages. It provides a strong defensive boost against special attacks in a single turn.

**Key Concept:** Self-targeting stat boost (+2 stages)

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_AMNESIA] = {
    .effect = EFFECT_SPECIAL_DEFENSE_UP_2,
    .power = 0,
    .type = TYPE_PSYCHIC,
    .accuracy = 0,  // Self-targeting, doesn't check accuracy
    .pp = 20,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,
    .priority = 0,
}
```

### Execution Steps

```
1. No accuracy check (self-targeting move)
2. ModifyStatStage(STAT_SPDEF, +2, affects_user=true)
3. Stat stage clamped at +6 (maximum)
4. No damage dealt
5. No CheckFaint
```

---

## State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon (user) | `stat_stages[STAT_SPDEF]` | +2 (clamped at +6) |
| Pokemon (target) | - | No change |

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:58` - EFFECT_SPECIAL_DEFENSE_UP_2 definition (effect 54)
- `src/data/battle_moves.h:1732-1741` - Amnesia move data
- `data/battle_scripts_1.s` - Effect implementation (shares code with other stat boost moves)

**Key Mechanics:**
- Self-targeting move (MOVE_TARGET_USER)
- Raises Sp. Defense by 2 stages
- Cannot miss (accuracy check skipped for self-targeting)
- Stat stages cap at +6

---

## Implementation Notes

- Uses shared `ModifyStatStage` command with `affects_user=true`
- No accuracy check needed (self-targeting)
- Effect is reused by: Barrier (Psychic type, Gen I), Iron Defense targets Defense instead
- Similar pattern to: Iron Defense (Defense +2), Agility (Speed +2), Tail Glow (Sp. Attack +2)

---

## Historical Context

In Generation I, Amnesia was notoriously powerful because Special Attack and Special Defense were the same stat ("Special"). Raising Special by 2 stages boosted both offense and defense. In Gen II onwards, the stats were split, making Amnesia purely defensive.

---

## Example Moves Using This Effect

- **Amnesia** (Psychic type, 20 PP)
- **Barrier** (Psychic type, 20 PP) - Gen I move, same effect

Note: Acid Armor (Poison type) has the same effect in later generations but may use a different effect ID in Gen III.

---

## Related Effects

- `EFFECT_SPECIAL_DEFENSE_UP` - Raises Sp. Defense by 1 stage
- `EFFECT_DEFENSE_UP_2` - Raises Defense by 2 stages (Iron Defense)
- `EFFECT_SPECIAL_DEFENSE_DOWN_2` - Lowers Sp. Defense by 2 stages (Fake Tears)
