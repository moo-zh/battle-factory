# Effect: SUBSTITUTE (Substitute)

**Status:** ✅ Implemented
**Effect ID:** `EFFECT_SUBSTITUTE` (pokeemerald: 95)
**Introduced:** Generation I
**Example Moves:** Substitute
**Category:** Status (Creates substitute to absorb damage)

---

## Overview

Substitute is a **status move** that creates a decoy (substitute) that takes damage in place of the user. The substitute is created by sacrificing **25% of the user's max HP** (rounded down, minimum 1 HP). Once created, the substitute absorbs all incoming damage until it breaks.

**Key Mechanics:**
- Costs 25% of max HP to create
- Fails if user has ≤25% HP (can't afford the cost)
- Fails if user already has a substitute
- Substitute has HP equal to the HP cost (25% of max HP)
- Substitute absorbs damage until its HP reaches 0
- User is protected from most status moves while substitute is active

---

## Gen III Mechanics

### Move Data (pokeemerald)
```c
[MOVE_SUBSTITUTE] = {
    .effect = EFFECT_SUBSTITUTE,
    .power = 0,
    .type = TYPE_NORMAL,
    .accuracy = 0,  // Never misses (self-targeting)
    .pp = 10,
    .secondaryEffectChance = 0,
    .target = MOVE_TARGET_USER,
    .priority = 0,
    .flags = FLAG_SNATCH_AFFECTED,
}
```

### Battle Script Flow

**Success Path:**
1. Check if user already has STATUS2_SUBSTITUTE → if yes, fail (already has substitute)
2. Calculate HP cost: max_hp / 4 (minimum 1)
3. Check if user has enough HP (current_hp > cost) → if not, fail (insufficient HP)
4. Deduct HP cost from user
5. Set STATUS2_SUBSTITUTE flag
6. Store substitute HP in gDisableStructs[battler].substituteHP
7. Clear STATUS2_WRAPPED if active (sub breaks bind moves)
8. Display message: "[Pokémon] made a SUBSTITUTE!"

**Failure Paths:**
- **Already has substitute**: "But [Pokémon] already has a SUBSTITUTE!"
- **Insufficient HP**: "It failed! [Pokémon] does not have enough HP!"

---

## State Tracking

Substitute uses TWO pieces of state:

```c
#define STATUS2_SUBSTITUTE (1 << 24)  // pokeemerald: include/constants/battle.h:148
```

**STATUS2_SUBSTITUTE:**
- Volatile status flag (bit 24 of status2)
- Indicates substitute is active
- Cleared when substitute breaks (HP reaches 0)

**gDisableStructs[battler].substituteHP:**
- Stores the substitute's current HP
- Initialized to 25% of max HP when created
- Decremented when substitute takes damage
- When reaches 0, substitute breaks (STATUS2_SUBSTITUTE cleared)

---

## Implementation Requirements

### New Pokemon State Fields

Add to `battle::state::Pokemon`:

```cpp
// Substitute state
bool has_substitute;      // Volatile flag: substitute is active
uint16_t substitute_hp;   // Substitute's current HP (0 when no substitute)
```

### Effect Implementation

**Effect_Substitute(BattleContext& ctx):**

```
1. Check if already has substitute
   - If has_substitute == true:
     - Set move_failed = true
     - Return (fail: already has substitute)

2. Calculate HP cost
   - cost = max_hp / 4
   - if cost == 0: cost = 1 (minimum cost)

3. Check if can afford cost
   - If current_hp <= cost:
     - Set move_failed = true
     - Return (fail: insufficient HP)

4. Create substitute
   - Deduct HP: current_hp -= cost
   - Set has_substitute = true
   - Set substitute_hp = cost
   - Set move_failed = false (success)
```

### Damage Application Modification (Future)

When implementing damage to Pokemon with substitutes, modify `ApplyDamage`:

```cpp
void ApplyDamage(BattleContext& ctx) {
    if (ctx.move_failed) return;

    // Check if defender has substitute
    if (ctx.defender->has_substitute) {
        // Damage goes to substitute instead
        if (ctx.damage_dealt >= ctx.defender->substitute_hp) {
            // Substitute breaks
            uint16_t overflow = ctx.damage_dealt - ctx.defender->substitute_hp;
            ctx.defender->substitute_hp = 0;
            ctx.defender->has_substitute = false;
            // Overflow damage is NOT dealt to Pokemon (substitute absorbed it all)
            ctx.damage_dealt = ctx.defender->substitute_hp;  // For display purposes
        } else {
            // Substitute survives
            ctx.defender->substitute_hp -= ctx.damage_dealt;
        }
        return;  // User's HP unchanged
    }

    // Normal damage (no substitute)
    // ... existing logic ...
}
```

**Note:** This ApplyDamage modification is **deferred** until we test Substitute with actual attacks. For now, we'll test Substitute creation in isolation.

---

## Test Cases

### Basic Substitute Creation

1. **Creates substitute successfully**
   - User with full HP creates substitute
   - User HP decreased by 25% of max HP
   - has_substitute = true
   - substitute_hp = 25% of max HP
   - move_failed = false

2. **Substitute HP calculation**
   - User with 100 max HP → substitute has 25 HP
   - User with 40 max HP → substitute has 10 HP
   - User with 3 max HP → substitute has 0 HP (rounds down to 0, but minimum is 1)
   - User with 1 max HP → substitute has 0 HP (but requires minimum 1 HP cost)

3. **Minimum HP cost**
   - Even if max_hp / 4 == 0, cost should be at least 1 HP
   - User with 3 max HP → cost = 1 HP (not 0)

### Failure Cases

4. **Fails if insufficient HP**
   - User has 25 HP, max HP 100 (exactly 25%)
   - Substitute should FAIL (needs >25% to create)
   - User HP unchanged
   - has_substitute = false
   - move_failed = true

5. **Fails if already has substitute**
   - User creates substitute (success)
   - User tries to create another substitute (fail)
   - move_failed = true
   - Existing substitute unchanged

6. **Edge case: exactly at threshold**
   - User has HP = max_hp / 4
   - Should FAIL (needs MORE than cost to create)

### HP Deduction

7. **HP deducted correctly**
   - User starts with 100 HP (max 100)
   - Creates substitute
   - User HP should be 75 (100 - 25)

8. **Cannot reduce HP to 0 or below**
   - User with low HP cannot create substitute if it would leave them with ≤0 HP

### Multiple Uses

9. **Can create substitute again after it breaks**
   - Create substitute (success)
   - Break substitute (set has_substitute = false manually in test)
   - Create substitute again (success)

---

## pokeemerald Cross-Reference

**Files:**
- `include/constants/battle_move_effects.h:95` - EFFECT_SUBSTITUTE definition
- `data/battle_scripts_1.s:1085-1109` - BattleScript_EffectSubstitute
- `src/battle_script_commands.c:7808-7833` - Cmd_setsubstitute
- `src/data/battle_moves.h:2135-2147` - Substitute move data
- `include/constants/battle.h:148` - STATUS2_SUBSTITUTE flag

**Key Mechanics:**
- STATUS2_SUBSTITUTE flag (bit 24 of status2)
- gDisableStructs[battler].substituteHP - stores substitute's HP
- HP cost: `maxHP / 4` (minimum 1)
- Condition: `current_hp > cost` (strict inequality)
- Clears STATUS2_WRAPPED when substitute is created
- HITMARKER_IGNORE_SUBSTITUTE set during creation (sub damage doesn't apply to itself)

**Implementation Details:**
```c
u32 hp = gBattleMons[gBattlerAttacker].maxHP / 4;
if (gBattleMons[gBattlerAttacker].maxHP / 4 == 0)
    hp = 1;

if (gBattleMons[gBattlerAttacker].hp <= hp)  {
    // FAIL: insufficient HP
    gBattleMoveDamage = 0;
    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SUBSTITUTE_FAILED;
} else {
    // SUCCESS: create substitute
    gBattleMoveDamage = hp;  // HP to deduct
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_SUBSTITUTE;
    gDisableStructs[gBattlerAttacker].substituteHP = hp;
    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_SUBSTITUTE;
}
```

---

## Notes

- This is the **first substitute mechanic** implementation
- Substitute HP is **independent** of user's HP (doesn't track percentage)
- Substitute absorbs damage from **all sources** (attacks, recoil, etc.)
- Status moves that target the opponent (e.g., Thunder Wave) are **blocked** by substitute
- Moves that target the user (e.g., Swords Dance) work **normally** with substitute active
- Some moves bypass substitute (Sound-based moves, etc.) - deferred for future implementation
- ApplyDamage modifications deferred until we need to test substitute blocking attacks

---

## Implementation Checklist

- [x] Add has_substitute and substitute_hp fields to Pokemon struct
- [x] Write failing tests for all test cases
- [x] Implement Effect_Substitute with HP cost calculation
- [x] Initialize new fields in test helpers
- [x] Add Substitute to Move enum and MOVE_DATABASE
- [x] Add Effect_Substitute to EFFECT_DISPATCH table
- [x] All tests passing (449/449)
- [x] Mark specification as implemented

**Implementation Status:** ✅ Complete (2025-11-21)
