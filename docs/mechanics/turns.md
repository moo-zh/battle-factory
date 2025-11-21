# Turn Execution Flow

This document defines the **complete execution order** for a single battle turn. This is the foundational behavior spec - everything else plugs into this flow.

## Why Order Matters

Gen III has strict ordering for effects. Getting it wrong causes subtle bugs:
- If weather damage happens before Leftovers, Pokemon die when they should survive
- If stat drops happen before damage, the damage is calculated incorrectly
- If faint checks happen at wrong times, abilities like Aftermath don't trigger correctly

**Rule:** When in doubt about order, check pokeemerald's battle_main.c and battle_script_commands.c.

---

## Turn Overview

```
┌─────────────────────────────────────────────────────┐
│ 1. TURN START                                       │
│    - Both players select actions                    │
│    - Determine turn order                           │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│ 2. ACTION EXECUTION (for each action in order)      │
│    - Pre-action checks (can actor act?)             │
│    - Execute action (move/switch)                   │
│    - Post-action processing (faints)                │
│    - Check battle over                              │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│ 3. END OF TURN                                      │
│    - Weather damage                                 │
│    - Status damage                                  │
│    - Item effects                                   │
│    - Counter decrements                             │
└─────────────────────────────────────────────────────┘
```

---

## Phase 1: Turn Start

### 1.1 Action Selection

Both players simultaneously select their actions:
- **Move** (choice 0-3)
- **Switch** (choice 0-5, target party slot)
- **Item** (Battle Factory doesn't use items mid-battle)
- **Run** (forfeit)

### 1.2 Turn Order Determination

Actions are sorted by priority:

```
1. Switches always go first (priority +7)
2. Move priority (Quick Attack +1, Counter -5, etc.)
3. Speed comparison (higher goes first)
4. Random tiebreaker (50/50)
```

**Decision Tree:**

```
Compare actions A and B:
├── Is A a switch and B is not? → A first
├── Is B a switch and A is not? → B first
├── A.priority > B.priority? → A first
├── B.priority > A.priority? → B first
├── A.speed > B.speed? → A first
├── B.speed > A.speed? → B first
└── Random(2) == 0? → A first, else B first
```

**Speed Calculation:**

```cpp
effective_speed = base_speed
    * stat_stage_multiplier[speed_stage]  // -6 to +6
    * (has_paralysis ? 0.25 : 1.0)        // paralysis quarters speed
    * ability_modifier                     // Swift Swim, Chlorophyll, etc.
    * item_modifier                        // Choice Scarf, Quick Claw
```

**Note:** Quick Claw is checked here. If it activates (20% chance), that Pokemon gets +7 priority for this turn only.

---

## Phase 2: Action Execution

For each action in turn order:

### 2.1 Pre-Action Checks

Before the actor can act, check if they're prevented:

```
Can this Pokemon act?
├── Is fainted? → Skip action (shouldn't happen, but guard)
├── Is frozen?
│   ├── 20% chance to thaw → Continue
│   └── Otherwise → Skip action, display "frozen solid"
├── Is asleep?
│   ├── Decrement sleep counter
│   ├── Counter reaches 0? → Wake up, continue
│   └── Otherwise → Skip action (except Sleep Talk, Snore)
├── Is paralyzed?
│   └── 25% chance → Skip action, display "fully paralyzed"
├── Is flinching?
│   └── Clear flinch flag, skip action, display "flinched"
├── Is confused?
│   ├── Decrement confusion counter
│   ├── Counter reaches 0? → Snap out, continue
│   └── 50% chance → Hit self (40 power, typeless), skip action
├── Is infatuated?
│   └── 50% chance → Skip action, display "immobilized by love"
└── Continue to action execution
```

**Order matters here.** Check in this exact order because:
- Frozen Pokemon can't flinch (they're frozen)
- Sleeping Pokemon can't be confused (they're asleep)
- Confusion self-hit happens even if infatuated

### 2.2 Execute Action

#### 2.2.1 Switch Action

```
ExecuteSwitch(player, target_slot):
  1. Get outgoing Pokemon
  2. Clear outgoing's SlotState (stat stages, volatile status)
     - EXCEPT if Baton Pass (handled in move effect)
  3. Update active index to target_slot
  4. Get incoming Pokemon
  5. Apply entry hazards:
     a. Spikes: 1 layer = 1/8, 2 = 1/6, 3 = 1/4 max HP
        - Flying types and Levitate immune
     b. Toxic Spikes: 1 layer = poison, 2 = badly poison
        - Flying types and Levitate immune
        - Poison types absorb (remove all layers)
        - Steel types immune
     c. Stealth Rock: Type-effectiveness damage
        - 1/8 base, multiplied by type chart vs Rock
  6. Trigger switch-in abilities:
     - Intimidate: Lower opponent's Attack by 1
     - Trace: Copy opponent's ability
     - Download: Raise Atk or SpA based on opponent's defenses
  7. Check for faint from hazards
```

#### 2.2.2 Move Action

```
ExecuteMove(attacker, move_index):
  1. Get move from attacker's moveset
  2. Check PP
     ├── PP == 0 → Use Struggle instead
     └── PP > 0 → Decrement PP by 1
  3. Determine target(s) based on move's target type
     - Single opponent
     - Both opponents (doubles, not applicable here)
     - Self
     - All (earthquake)
  4. For each target:
     a. Create BattleContext
     b. Call effect function: g_MoveEffects[move.effect](ctx)
     c. Process any queued faint
  5. Update move tracking:
     - Set "last move used" for Encore, Disable
     - Set "last target" for Pursuit
     - Set "move this turn" for Counter, Mirror Coat
```

#### 2.2.3 Effect Function Execution

This is where the command chain runs. See `docs-v2/contracts/engine-commands.md` for details.

Typical flow:
```
Effect_XXX(ctx):
  AccuracyCheck(ctx)      → May set move_failed
  CalculateDamage(ctx)    → Sets ctx.damage
  ApplyDamage(ctx)        → Reduces defender HP
  ApplySecondaryEffect(ctx) → Status, stat change, etc.
  CheckFaint(ctx)         → Sets faint flag
```

### 2.3 Post-Action Processing

After each action completes:

```
PostActionProcessing():
  1. Process faints:
     For each fainted Pokemon this action:
       a. Trigger Aftermath (if applicable)
       b. Award experience (not in Battle Factory)
       c. Check if team wiped → End battle
       d. If battle continues, prompt for replacement
          - In Battle Factory: immediate forced switch

  2. Check for battle end:
     ├── All player Pokemon fainted → Player loses
     └── All enemy Pokemon fainted → Player wins

  3. If battle continues:
     - Process any Destiny Bond
     - Process any Grudge (sets move PP to 0)
```

**Important:** Faints are processed **after each action**, not at end of turn. This means:
- If Pokemon A KOs Pokemon B, B's replacement comes in before Pokemon C acts
- Pursuit can hit during the switch

---

## Phase 3: End of Turn

All end-of-turn effects happen after both Pokemon have acted (or been prevented from acting). Order is critical.

### 3.1 Future Attack Resolution

```
For each side:
  If Future Sight/Doom Desire counter reaches 0:
    a. Deal stored damage to current Pokemon in that slot
    b. Check faint
```

### 3.2 Wish Resolution

```
For each side:
  If Wish counter reaches 0:
    a. Heal current Pokemon in that slot by 50% of Wisher's max HP
```

### 3.3 Healing Effects

Process in speed order (faster Pokemon first):

```
For each Pokemon (by speed):
  - Ingrain: Heal 1/16 max HP
  - Aqua Ring: Heal 1/16 max HP
  - Rain Dish (in rain): Heal 1/16 max HP
```

### 3.4 Damaging Effects

Process in speed order:

```
For each Pokemon (by speed):
  - Leech Seed: Lose 1/8 max HP, opponent heals same amount
    - Check faint after Leech Seed
  - Nightmare (while asleep): Lose 1/4 max HP
  - Curse (ghost): Lose 1/4 max HP
  - Check faint after each effect
```

### 3.5 Binding Damage

```
For each Pokemon (by speed):
  If bound (Wrap, Bind, Fire Spin, etc.):
    a. Lose 1/16 max HP (or 1/8 with Binding Band)
    b. Decrement bind counter
    c. If counter = 0, release from bind
    d. Check faint
```

### 3.6 Status Damage

Process in speed order:

```
For each Pokemon (by speed):
  If burned:
    - Lose 1/8 max HP
    - Check faint

  If poisoned:
    - Lose 1/8 max HP
    - Check faint

  If badly poisoned:
    - Lose (turn_count/16) max HP (1/16, 2/16, 3/16, ...)
    - Increment toxic counter
    - Check faint
```

### 3.7 Perish Song

```
For each Pokemon with Perish count:
  a. Decrement Perish counter
  b. Display "Perish count fell to X"
  c. If counter = 0:
     - Pokemon faints
     - Process faint
```

### 3.8 Weather Damage

Weather damages simultaneously (not speed order):

```
If weather is Sandstorm:
  For each Pokemon:
    - Skip if Rock, Ground, or Steel type
    - Skip if Sand Veil or Sand Stream ability
    - Lose 1/16 max HP
    - Check faint

If weather is Hail:
  For each Pokemon:
    - Skip if Ice type
    - Skip if Snow Cloak or Ice Body ability
    - Lose 1/16 max HP
    - Check faint
```

### 3.9 Uproar

```
For each Pokemon in Uproar:
  a. Decrement Uproar counter
  b. If counter = 0, Uproar ends
  c. Wake up all sleeping Pokemon on field
```

### 3.10 Speed-Ordered Effects

Process remaining effects in speed order:

```
For each Pokemon (by speed):
  - Outrage/Thrash/Petal Dance: Decrement counter, confuse if ended
  - Disable: Decrement counter, clear if ended
  - Encore: Decrement counter, clear if ended
  - Taunt: Decrement counter, clear if ended
  - Magnet Rise: Decrement counter, clear if ended
```

### 3.11 Item Effects

Process in speed order:

```
For each Pokemon (by speed):
  - Leftovers: Heal 1/16 max HP
  - Black Sludge: Heal 1/16 if Poison type, else lose 1/8
  - Sticky Barb: Lose 1/8 max HP
  - Check faint (for damaging items)
```

### 3.12 Ability Effects

```
For each Pokemon (by speed):
  - Speed Boost: Raise Speed by 1 stage
  - Shed Skin: 30% chance to cure status
  - Hydration (in rain): Cure status
```

### 3.13 Counter Decrements

Decrement all field/side effect counters:

```
Weather:
  - Decrement weather counter
  - If counter = 0, weather clears

Screens (per side):
  - Decrement Reflect counter, clear if 0
  - Decrement Light Screen counter, clear if 0
  - Decrement Safeguard counter, clear if 0
  - Decrement Mist counter, clear if 0
  - Decrement Tailwind counter, clear if 0

Terrain:
  - Decrement terrain counter, clear if 0
```

### 3.14 Zen Mode / Illusion

```
For each Pokemon:
  - Darmanitan with Zen Mode: Transform if HP <= 50%
  - Illusion: Already handled on damage taken
```

### 3.15 Turn Counter Increment

```
turn_number += 1
```

---

## Summary: Complete Turn Sequence

```cpp
void Engine::ExecuteTurn(const BattleAction& player_action,
                         const BattleAction& enemy_action) {
    // Phase 1: Turn Start
    const BattleAction* first;
    const BattleAction* second;
    DetermineActionOrder(player_action, enemy_action, first, second);

    // Phase 2: Action Execution
    ExecuteAction(*first);
    if (IsBattleOver()) return;

    ExecuteAction(*second);
    if (IsBattleOver()) return;

    // Phase 3: End of Turn
    ProcessEndOfTurn();

    turn_number_++;
}

void Engine::ExecuteAction(const BattleAction& action) {
    state::Pokemon& actor = GetActivePokemon(action.player);

    // Pre-action checks
    if (!CanAct(actor)) return;

    // Execute
    switch (action.type) {
        case ActionType::MOVE:
            ExecuteMove(action);
            break;
        case ActionType::SWITCH:
            ExecuteSwitch(action);
            break;
        // ...
    }

    // Post-action: faint processing
    ProcessFaints();
}

void Engine::ProcessEndOfTurn() {
    // Order matters! Each section has sub-ordering by speed
    ProcessFutureAttacks();      // 3.1
    ProcessWish();               // 3.2
    ProcessHealingEffects();     // 3.3
    ProcessDamageEffects();      // 3.4 Leech Seed, Nightmare, Curse
    ProcessBindingDamage();      // 3.5
    ProcessStatusDamage();       // 3.6 Burn, Poison, Toxic
    ProcessPerishSong();         // 3.7
    ProcessWeatherDamage();      // 3.8 Sandstorm, Hail
    ProcessUproar();             // 3.9
    ProcessSpeedOrderedEffects();// 3.10 Disable, Encore, etc.
    ProcessItemEffects();        // 3.11 Leftovers
    ProcessAbilityEffects();     // 3.12 Speed Boost
    ProcessCounterDecrements();  // 3.13 Weather, Screens
}
```

---

## Processing Order Notes

### Why Order Matters

The specific order of end-of-turn effects matters for edge cases:

1. **Status damage before weather:** A Pokemon that takes burn damage (1/8 HP) will then take weather damage (1/16 HP). This can cause deaths that wouldn't occur in reverse order.

2. **Perish Song before weather:** If a Pokemon's Perish count reaches 0 and they faint, they don't take weather damage afterward.

3. **Leftovers timing:** Leftovers heal happens AFTER status and weather damage, allowing a Pokemon to survive damage that would otherwise KO it.

4. **Speed ordering consistency:** All speed-ordered effects use the same ordering throughout the turn. A Pokemon that acted first in the main turn acts first in end-of-turn processing.

### Gen III Specific Details

- Weather damage happens simultaneously to all Pokemon (not speed-ordered)
- Status damage is speed-ordered
- Item effects (Leftovers) are speed-ordered
- Ability effects (Speed Boost) are speed-ordered

---

## Edge Cases and Gotchas

### Faint Timing

- **Destiny Bond:** If user faints this turn, attacker also faints (even if they switched)
- **Aftermath:** Damages attacker when contact move KOs this Pokemon
- **Self-KO clause:** If both Pokemon faint on same turn, the one who moved last loses

### Speed Ties in End-of-Turn

When two Pokemon have the same speed, the one that **acted first** in the turn goes first in end-of-turn processing.

### Weather Ability Interaction

- Pokemon with Drought/Drizzle/Sand Stream/Snow Warning trigger on switch-in
- This happens in entry hazard phase, so weather is active for same-turn end-of-turn

### Pursuit

Pursuit hits during the switch action, not after. Execution flow:
1. Defender selects Switch
2. Attacker selects Pursuit
3. Turn order: Switch goes first (priority +7)
4. But Pursuit interrupts: Pursuit executes before the switch completes
5. If defender survives, switch continues

---

## Testing Checkpoints

When implementing turn execution, verify these scenarios:

1. **Basic turn:** Both use damaging moves, faster goes first
2. **Speed tie:** Verify random tiebreaker works
3. **Switch priority:** Switch always goes before moves
4. **Move priority:** Quick Attack goes before Tackle
5. **Status prevention:** Frozen Pokemon can't act
6. **Sleep counter:** Pokemon wakes up after correct turns
7. **Confusion self-hit:** Correct damage, correct frequency
8. **Entry hazards:** Spikes damage on switch
9. **End-of-turn order:** Weather before Leftovers before decrement
10. **Faint timing:** Destiny Bond triggers correctly

---

## Pokeemerald Reference

Key files to cross-reference:
- `src/battle_main.c`: BattleTurnPassed(), HandleEndTurn_BattleWon()
- `src/battle_script_commands.c`: Cmd_accuracycheck, Cmd_moveend
- `data/battle_scripts_1.s`: Battle script bytecode

---

**Document Status:**
- [x] Initial specification
- [x] Validated against pokeemerald
- [x] Cross-referenced with STATE_DOMAIN_MAPPING
- [ ] Test cases written

**Validation Notes:** End-of-turn order confirmed as status damage → Perish Song → weather damage. Added processing order notes section explaining why order matters.
