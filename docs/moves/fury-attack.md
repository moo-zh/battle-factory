# Effect: MULTI_HIT (Fury Attack)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_MULTI_HIT` (pokeemerald: 29)
**Introduced:** Generation I
**Example Moves:** Fury Attack, Fury Swipes, Bone Rush, Pin Missile
**Category:** Damage (Multi-hit)

---

## Overview

Multi-hit moves attack 2-5 times in a single turn, with the number of hits determined randomly at the start of the move. This introduces the **multi-hit mechanic** - a single move execution that deals damage multiple times with independent damage rolls.

---

## Behavior Specification

Multi-hit moves attack 2-5 times in a single turn. The number of hits is determined randomly at the start of the move.

### Execution Flow

1. **Accuracy Check** - Checked once at the start
2. **Determine Hit Count** - Randomly select 2-5 hits (weighted distribution)
3. **Loop for each hit:**
   - Calculate damage (with fresh crit roll each hit)
   - Apply damage
   - Check if defender fainted → break loop early
   - Check if attacker fainted → break loop early
4. **Check Faint** - Final faint check after all hits

### Hit Count Distribution

Based on pokeemerald `Cmd_setmultihitcounter`:

```c
gMultiHitCounter = Random() & 3;  // 0-3
if (gMultiHitCounter > 1)
    gMultiHitCounter = (Random() & 3) + 2;  // 2-5
else
    gMultiHitCounter += 2;  // 2-3
```

This creates the following distribution:
- **2 hits**: 37.5% chance (3/8)
- **3 hits**: 37.5% chance (3/8)
- **4 hits**: 12.5% chance (1/8)
- **5 hits**: 12.5% chance (1/8)

### Key Mechanics

1. **Single Accuracy Check**: Accuracy is only checked once. If the move hits, all subsequent hits land.
2. **Independent Damage Rolls**: Each hit has its own damage calculation and crit roll.
3. **Early Termination**: The loop stops early if either Pokemon faints mid-sequence.
4. **Total Damage**: Damage accumulates across all hits.

### Example Moves

- **Fury Attack** (15 power, 85 accuracy, Normal type)
- **Double Slap** (15 power, 85 accuracy, Normal type)
- **Comet Punch** (18 power, 85 accuracy, Normal type)
- **Pin Missile** (25 power, 95 accuracy, Bug type)
- **Spike Cannon** (20 power, 100 accuracy, Normal type)

## Test Cases

### Basic Mechanics
1. **Multiple hits occur** - Move hits 2-5 times
2. **Damage accumulates** - Total damage = sum of all hits
3. **Hit count distribution** - Verify 2-5 hits with proper weighting
4. **Each hit calculates damage independently** - Variance between hits

### Accuracy
5. **Single accuracy check** - Accuracy only checked once at start
6. **Miss prevents all hits** - If accuracy fails, no damage dealt
7. **Hit guarantees all subsequent hits** - Once it hits, all hits land

### Early Termination
8. **Defender faints mid-sequence** - Loop stops when defender HP reaches 0
9. **No overkill damage** - HP clamped at 0, no negative HP
10. **Hit count reflects actual hits** - If defender faints on hit 3, only 3 hits occurred

### Critical Hits
11. **Independent crit rolls** - Each hit can crit independently
12. **Multiple crits possible** - Can have 0, 1, or multiple crits in one use

### Edge Cases
13. **Low HP defender** - Can KO with first hit
14. **Exact lethal damage** - Defender faints exactly on final hit
15. **Does not affect other stats** - No stat changes
16. **Does not cause status** - Pure damage effect

## pokeemerald References

### Battle Script
`pokeemerald/data/battle_scripts_1.s:BattleScript_EffectMultiHit`

Key commands:
- `setmultihitcounter 0` - Randomly determine 2-5 hits
- `decrementmultihit BattleScript_MultiHitLoop` - Loop until counter reaches 0
- `jumpifhasnohp BS_TARGET, BattleScript_MultiHitPrintStrings` - Early exit on faint

### Source Code
`pokeemerald/src/battle_script_commands.c`

- `Cmd_setmultihitcounter()` - Hit count RNG (lines ~6800-6815)
- `Cmd_decrementmultihit()` - Loop decrement logic

### Move Data
`pokeemerald/src/data/battle_moves.h:MOVE_FURY_ATTACK`

```c
[MOVE_FURY_ATTACK] = {
    .effect = EFFECT_MULTI_HIT,
    .power = 15,
    .type = TYPE_NORMAL,
    .accuracy = 85,
    .pp = 20,
}
```

## Implementation Notes

### State Tracking

The implementation needs to track:
- `hit_count` - Total number of hits that will occur (2-5)
- `hits_landed` - How many hits have actually landed so far
- `total_damage` - Cumulative damage across all hits

### Command Usage

```cpp
void Effect_MultiHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);

    // Determine 2-5 hits using pokeemerald's algorithm
    uint8_t hit_count = DetermineMultiHitCount();

    for (uint8_t i = 0; i < hit_count; i++) {
        commands::CalculateDamage(ctx);
        commands::ApplyDamage(ctx);

        // Early exit if defender fainted
        if (ctx.defender->current_hp == 0) {
            ctx.defender->is_fainted = true;
            break;
        }

        // Early exit if attacker fainted (rare)
        if (ctx.attacker->current_hp == 0) {
            ctx.attacker->is_fainted = true;
            break;
        }
    }

    commands::CheckFaint(ctx);
}
```

## Implementation Status

- [ ] Effect specification documented
- [ ] Test cases written
- [ ] Effect implemented
- [ ] Tests passing
- [ ] Integrated with move catalog

---

**Document Status:**
- [x] Initial specification
- [x] pokeemerald cross-reference
- [x] Test cases defined
- [ ] Implementation validated
