# State Transitions

This document defines **when** state changes occur. For **what** state exists in each domain, see `docs/architecture/battle-engine/STATE_DOMAIN_MAPPING.md`.

---

## Overview

State transitions happen in response to battle events. This document answers:
- "When does weather decrement?"
- "When do stat stages clear?"
- "When does toxic counter reset?"

---

## Event-Driven State Changes

### Event: Turn Start

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| - | - | No state changes at turn start | - |

Turn start only involves action selection and turn order calculation.

### Event: Move Execution

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `pp[move_index]` | Decrement by 1 | Engine::ExecuteMove |
| SlotState | `last_move` | Set to move used | Engine::ExecuteMove |
| SlotState | `last_landed_move` | Set if move hit | Effect function |
| SlotState | `protect.physical_dmg` | Set if physical damage taken | ApplyDamage |
| SlotState | `protect.special_dmg` | Set if special damage taken | ApplyDamage |

### Event: Damage Applied

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Decrease by damage | ApplyDamage |
| SlotState | `disable.substitute_hp` | Decrease if sub active | ApplyDamage |

### Event: Status Applied

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `status1` | Set status flag | TryApplyBurn, etc. |
| PokemonState | `sleep_turns` | Set 1-3 for sleep | TryApplySleep |
| PokemonState | `toxic_counter` | Set to 1 | TryApplyBadPoison |

### Event: Status Cured

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `status1` | Clear status flag | CureStatus |
| PokemonState | `toxic_counter` | Reset to 0 | CureStatus |

### Event: Stat Stage Modified

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `stat_stages[stat]` | Increase/decrease 1-3 | ModifyStat |

Note: Clamped to -6 to +6 range.

### Event: Volatile Status Applied

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `status2` | Set flag | Various commands |
| SlotState | `disable.confusion_turns` | Set 2-5 | ApplyConfusion |
| SlotState | `disable.encore_timer` | Set to 3-6 | ApplyEncore |

### Event: Weather Set

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| FieldState | `weather` | Set weather type | SetWeather |
| FieldState | `weather_duration` | Set to 5 (or 8 with Rock) | SetWeather |

### Event: Screen Set

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SideState | `reflect` / `light_screen` | Set to true | SetScreen |
| SideState | `reflect_turns` | Set to 5 (or 8 with Clay) | SetScreen |

### Event: Hazard Set

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SideState | `spikes_layers` | Increment (max 3) | AddSpikes |
| SideState | `toxic_spikes_layers` | Increment (max 2) | AddToxicSpikes |
| SideState | `stealth_rock` | Set to true | AddStealthRock |

---

## Switch Events

### Event: Switch Out (Normal)

**All SlotState for that slot is cleared.**

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `stat_stages[*]` | Reset to 0 | Engine::ExecuteSwitch |
| SlotState | `status2` | Set to 0 | Engine::ExecuteSwitch |
| SlotState | `status3` | Set to 0 | Engine::ExecuteSwitch |
| SlotState | `disable` | Reset all fields | Engine::ExecuteSwitch |
| SlotState | `protect` | Reset all fields | Engine::ExecuteSwitch |
| SlotState | `history` | Reset all fields | Engine::ExecuteSwitch |

**PokemonState is NOT touched** - HP, status1, PP stay with the Pokemon.

### Event: Switch Out (Baton Pass)

**Partial SlotState preserved.**

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `stat_stages[*]` | **PRESERVED** | Effect_BatonPass |
| SlotState | `status2` | Keep: Confusion, Focus Energy, Substitute, Cursed, Mean Look | Effect_BatonPass |
| SlotState | `status3` | Keep: Leech Seed, Lock-On, Perish Song, Ingrain, Mud/Water Sport | Effect_BatonPass |
| SlotState | `disable.substitute_hp` | **PRESERVED** | Effect_BatonPass |
| SlotState | Other | Reset | Effect_BatonPass |

### Event: Switch In

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Damage from hazards | Engine::ExecuteSwitch |
| SlotState | - | Initialized to defaults (or Baton Pass data) | Engine::ExecuteSwitch |
| SideState | `toxic_spikes_layers` | Clear if Poison type switches in | Engine::ExecuteSwitch |

### Event: Faint

Same as Switch Out, plus:

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Already 0 | - |
| FieldState | - | Destiny Bond may KO attacker | ProcessFaints |

---

## End-of-Turn Transitions

These happen in strict order (see `turn-execution.md` for full list).

### Future Sight Resolution

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| FieldState | `future_sight.counter[slot]` | Decrement | ProcessFutureAttacks |
| PokemonState | `current_hp` | Damage if counter = 0 | ProcessFutureAttacks |

### Healing Effects

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Heal from Ingrain, Aqua Ring | ProcessHealingEffects |
| FieldState | `wish.counter[slot]` | Decrement | ProcessWish |
| PokemonState | `current_hp` | Heal if Wish counter = 0 | ProcessWish |

### Damaging Effects

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Leech Seed damage | ProcessDamageEffects |
| PokemonState | `current_hp` | Opponent heals from Leech Seed | ProcessDamageEffects |
| PokemonState | `current_hp` | Nightmare damage (if asleep) | ProcessDamageEffects |
| PokemonState | `current_hp` | Curse (ghost) damage | ProcessDamageEffects |

### Binding Damage

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | 1/16 or 1/8 max HP | ProcessBindingDamage |
| SlotState | `disable.bind_timer` | Decrement | ProcessBindingDamage |
| SlotState | `status2 & WRAPPED` | Clear if timer = 0 | ProcessBindingDamage |

### Status Damage

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | 1/8 from Burn | ProcessStatusDamage |
| PokemonState | `current_hp` | 1/8 from Poison | ProcessStatusDamage |
| PokemonState | `toxic_counter` | Increment | ProcessStatusDamage |
| PokemonState | `current_hp` | (counter/16) from Toxic | ProcessStatusDamage |

### Perish Song

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `disable.perish_song_timer` | Decrement | ProcessPerishSong |
| PokemonState | `current_hp` | Set to 0 if timer = 0 | ProcessPerishSong |

### Weather Damage

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | 1/16 from Sandstorm/Hail | ProcessWeatherDamage |

### Item Effects

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| PokemonState | `current_hp` | Heal 1/16 from Leftovers | ProcessItemEffects |
| PokemonState | `current_hp` | Heal 1/16 from Black Sludge (Poison) | ProcessItemEffects |
| PokemonState | `current_hp` | Damage 1/8 from Black Sludge (non-Poison) | ProcessItemEffects |

### Ability Effects

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| SlotState | `stat_stages[SPEED]` | +1 from Speed Boost | ProcessAbilityEffects |
| PokemonState | `status1` | Clear from Shed Skin (30%) | ProcessAbilityEffects |
| PokemonState | `status1` | Clear from Hydration (in rain) | ProcessAbilityEffects |

### Counter Decrements

| Domain | State | Change | Owner |
|--------|-------|--------|-------|
| FieldState | `weather_duration` | Decrement | ProcessCounterDecrements |
| FieldState | `weather` | Clear if duration = 0 | ProcessCounterDecrements |
| SideState | `reflect_turns` | Decrement | ProcessCounterDecrements |
| SideState | `reflect` | Clear if turns = 0 | ProcessCounterDecrements |
| SideState | `light_screen_turns` | Decrement | ProcessCounterDecrements |
| SideState | `light_screen` | Clear if turns = 0 | ProcessCounterDecrements |
| SideState | `safeguard_turns` | Decrement | ProcessCounterDecrements |
| SideState | `safeguard` | Clear if turns = 0 | ProcessCounterDecrements |
| SlotState | `disable.taunt_timer` | Decrement | ProcessCounterDecrements |
| SlotState | `disable.encore_timer` | Decrement | ProcessCounterDecrements |
| SlotState | `disable.disable_timer` | Decrement | ProcessCounterDecrements |

---

## Turn-Scoped State

Some state is only valid for the current turn and must be cleared.

### Cleared at Turn Start

| Domain | State | Purpose |
|--------|-------|---------|
| SlotState | `protect.protected` | Protect/Detect |
| SlotState | `protect.endured` | Endure |
| SlotState | `status2 & FLINCHED` | Flinch |
| SlotState | `disable.is_first_turn` | Fake Out eligibility |

### Cleared After Each Action

| Domain | State | Purpose |
|--------|-------|---------|
| SlotState | `protect.helping_hand` | Helping Hand |

---

## Toxic Counter Mechanics

The toxic counter deserves special attention because it behaves differently than other state:

### When Counter Changes

| Event | Change |
|-------|--------|
| Badly poisoned applied | Set to 1 |
| End of turn (badly poisoned) | Increment by 1 |
| Switched out | **Reset to 1** (not 0!) |
| Cured (Rest, item, etc.) | Reset to 0 |

**Key insight:** Toxic counter resets on switch, but the Pokemon is still badly poisoned. When it switches back in, damage starts at 1/16 again.

### Damage Formula

```cpp
damage = maxHp * toxic_counter / 16;
toxic_counter++;  // For next turn
```

Cap at 15 (15/16 max HP per turn).

---

## State Transition Diagram

### Primary Status Lifecycle

```
           ┌─────────────┐
           │    None     │
           └──────┬──────┘
                  │ TryApplyStatus
                  ▼
           ┌─────────────┐
      ┌────│   Active    │────┐
      │    └─────────────┘    │
      │                       │
      │ CureStatus            │ Switch (for Toxic counter only)
      │ Shed Skin             │
      │ Rest (special)        │
      │                       │
      ▼                       ▼
┌─────────────┐        ┌─────────────┐
│    None     │        │   Active    │ (counter reset)
└─────────────┘        └─────────────┘
```

### Stat Stage Lifecycle

```
           ┌─────────────┐
           │   Neutral   │ (switch in)
           │   Stage 0   │
           └──────┬──────┘
                  │
        ┌─────────┼─────────┐
        │         │         │
        ▼         │         ▼
   ┌─────────┐    │    ┌─────────┐
   │ Stage-N │    │    │ Stage+N │
   │ (min -6)│    │    │ (max +6)│
   └────┬────┘    │    └────┬────┘
        │         │         │
        │    ModifyStat     │
        │   (can go either) │
        │         │         │
        └─────────┼─────────┘
                  │
                  │ Switch out (not Baton Pass)
                  │ Haze
                  ▼
           ┌─────────────┐
           │   Neutral   │
           │   Stage 0   │
           └─────────────┘
```

### Weather Lifecycle

```
           ┌─────────────┐
           │    None     │
           └──────┬──────┘
                  │ SetWeather
                  ▼
           ┌─────────────┐
           │   Active    │◄───┐
           │ Duration: 5 │    │
           └──────┬──────┘    │
                  │           │ New weather
                  │ End of    │ (replaces)
                  │ turn      │
                  ▼           │
           ┌─────────────┐    │
           │   Active    │────┘
           │ Duration--  │
           └──────┬──────┘
                  │
                  │ Duration = 0
                  ▼
           ┌─────────────┐
           │    None     │
           └─────────────┘
```

---

## Common Mistakes

### 1. Forgetting toxic counter reset on switch

```cpp
// WRONG: toxic counter stays high after switching
void SwitchIn(Pokemon& mon) {
    // Missing: mon.toxic_counter = 1;
}

// RIGHT: reset counter but keep status
void SwitchIn(Pokemon& mon) {
    if (mon.HasStatus(STATUS1_TOXIC_POISON)) {
        mon.toxic_counter = 1;  // Start over
    }
}
```

### 2. Clearing PokemonState on switch

```cpp
// WRONG: clearing status when switching out
void SwitchOut(uint8_t slot) {
    pokemon[slot]->status1 = 0;  // NO! Status persists
}

// RIGHT: only clear SlotState
void SwitchOut(uint8_t slot) {
    slots[slot] = SlotState{};  // Clear volatile only
    // PokemonState untouched
}
```

### 3. Wrong order for end-of-turn

```cpp
// WRONG: Leftovers before weather damage
void EndOfTurn() {
    ProcessItemEffects();     // Leftovers heals
    ProcessWeatherDamage();   // Then takes damage
}

// RIGHT: weather damage first
void EndOfTurn() {
    ProcessWeatherDamage();   // Takes damage
    ProcessItemEffects();     // Then heals
}
```

### 4. Stat stages on Pokemon instead of Slot

```cpp
// WRONG: stat stages stored on Pokemon
struct Pokemon {
    int8_t attackStage;  // NO!
};

// RIGHT: stat stages are per-slot
struct SlotState {
    int8_t stat_stages[7];  // YES!
};
```

---

## Testing Checkpoints

1. **Switch clears stat stages:** +6 Attack → switch out → switch in → 0 Attack
2. **Baton Pass preserves stages:** +6 Attack → Baton Pass → +6 Attack
3. **Toxic counter resets:** Toxic counter 5 → switch out → switch in → counter 1
4. **Status persists:** Burned → switch out → switch in → still burned
5. **Leech Seed cleared:** Leech Seed → switch out → not seeded
6. **Weather decrements:** Set Rain → 5 turns → clears
7. **Screens decrement:** Set Reflect → 5 turns → clears

---

**Document Status:**
- [x] Initial specification
- [x] References STATE_DOMAIN_MAPPING
- [x] Validated against pokeemerald
- [ ] All counter edge cases verified

**Validation Notes:** 4-domain architecture confirmed. Toxic counter reset on switch (to 1, not 0) verified. Baton Pass preservation list accurate.
