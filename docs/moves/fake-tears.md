# Effect: SPECIAL_DEFENSE_DOWN_2 (Fake Tears)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SPECIAL_DEFENSE_DOWN_2` (pokeemerald: 62)
**Introduced:** Generation III
**Example Moves:** Fake Tears, Metal Sound
**Category:** Status (Opponent-targeting stat reduction)

---

## Overview

Fake Tears is a **status move** that sharply lowers the target's Special Defense stat by 2 stages. This makes the target significantly more vulnerable to special attacks.

**Key Concept:** Opponent-targeting stat reduction (-2 stages)

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_FAKE_TEARS] = {
    .effect = EFFECT_SPECIAL_DEFENSE_DOWN_2,
    .power = 0,
    .type = TYPE_DARK,
    .accuracy = 100,
    .pp = 20,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_SELECTED,
    .priority = 0,
}
```

### Execution Steps

```
1. AccuracyCheck (100% accuracy)
2. If protected, move fails
3. ModifyStatStage(STAT_SPDEF, -2, affects_user=false)
4. Stat stage clamped at -6 (minimum)
5. No damage dealt
6. No CheckFaint
```

---

## State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon (target) | `stat_stages[STAT_SPDEF]` | -2 (clamped at -6) |
| Pokemon (user) | - | No change |

---

## Protection Interaction

Fake Tears is blocked by Protect:
- If target is protected (`is_protected = true`), move fails
- Stat stages are not modified
- Move is consumed (PP reduced)

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:66` - EFFECT_SPECIAL_DEFENSE_DOWN_2 definition (effect 62)
- `src/data/battle_moves.h:4072-4081` - Fake Tears move data
- `data/battle_scripts_1.s` - Effect implementation (shares code with other stat reduction moves)

**Key Mechanics:**
- Opponent-targeting move (MOVE_TARGET_SELECTED)
- Lowers Sp. Defense by 2 stages
- 100% accuracy
- Blocked by Protect/Detect
- Stat stages floor at -6

---

## Implementation Notes

- Uses shared `ModifyStatStage` command with `affects_user=false`
- Requires accuracy check (can miss)
- Respects protection mechanics
- Similar pattern to: Growl (Attack -1), Tail Whip (Defense -1), String Shot (Speed -1)

---

## Example Moves Using This Effect

- **Fake Tears** (Dark type, 20 PP, 100% accuracy)
- **Metal Sound** (Steel type, 40 PP, 85% accuracy) - Gen III move, same effect

---

## Related Effects

- `EFFECT_SPECIAL_DEFENSE_DOWN` - Lowers Sp. Defense by 1 stage
- `EFFECT_DEFENSE_DOWN_2` - Lowers Defense by 2 stages
- `EFFECT_SPECIAL_DEFENSE_UP_2` - Raises Sp. Defense by 2 stages (Amnesia)
