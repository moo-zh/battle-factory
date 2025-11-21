# Effect: [EFFECT_NAME]

> Template for documenting move effects. Copy this file and fill in the sections.

## Overview

**Effect ID:** `EFFECT_XXX`
**Pokeemerald equivalent:** `EFFECT_XXX` in `data/battle_scripts_1.s`
**Example moves:** Move1, Move2, Move3
**Category:** Damage / Status / Field / Stat Modification / Special

## Behavior Specification

### What This Effect Does

Plain English description of the effect. Be precise.

### Execution Steps

```
1. [First thing that happens]
2. [Second thing that happens]
   a. [Sub-step if needed]
   b. [Another sub-step]
3. [Third thing]
```

### State Changes

| Domain | Field | Change |
|--------|-------|--------|
| Pokemon | `status` | Set to `Burn` |
| Pokemon | `hp` | Reduced by damage |
| Slot | `statStages[ATK]` | No change |

### Edge Cases

- **[Case 1]:** What happens when X?
- **[Case 2]:** What happens when Y?

## Pokeemerald Reference

```c
// From pokeemerald src/battle_script_commands.c or data/battle_scripts_1.s
// Paste relevant pokeemerald code here
```

Key observations:
- [What does pokeemerald do that might not be obvious?]

## Command Sequence

```cpp
void Effect_XXX(BattleContext& ctx) {
    // Pseudocode or actual implementation
    AccuracyCheck(ctx);
    CalculateDamage(ctx);
    ApplyDamage(ctx);
    // etc.
}
```

## Test Cases

```cpp
TEST(Effect_XXX, BasicBehavior) {
    // What to test
}

TEST(Effect_XXX, EdgeCase1) {
    // Edge case test
}
```

## Related Effects

- `EFFECT_YYY` - Similar but different because...
- `EFFECT_ZZZ` - Often confused with this one

## Implementation Status

- [ ] Specification complete
- [ ] Command sequence defined
- [ ] Tests written
- [ ] Implementation complete
- [ ] Integration tested
