# Development Workflow

This document describes **how to develop features** to minimize rewrites and accumulated code debt. The core principle: **vertical implementation over horizontal implementation**.

---

## The Problem

The pattern that leads to rewrites:

```
Week 1: Implement all 70 commands (500 lines)
Week 2: Implement all 189 effects (1000 lines)
Week 3: Discover that commands don't integrate correctly
Week 4: Rewrite everything
```

This is **horizontal development** - building out one layer completely before moving to the next.

---

## The Solution: Vertical Development

Instead of building layers, build **features** - one complete slice at a time.

```
Day 1: Implement Tackle end-to-end
       └── AccuracyCheck → CalculateDamage → ApplyDamage → CheckFaint
       └── Run it, test it, fix it (50 lines)

Day 2: Implement Ember (adds burn)
       └── Reuse Day 1 commands + TryApplyBurn
       └── Discover integration issues early (20 new lines)

Day 3: Implement Thunder Wave (status only)
       └── No damage path, tests pure status
       └── (15 new lines)
```

Each day you have **working code** that does something complete. Integration issues are discovered with 50 lines of code, not 500.

---

## The Vertical Implementation Process

### Phase 1: Specify Before Coding

Before writing any implementation code:

1. **Choose one move** to implement
2. **Write its effect spec** in `docs-v2/behavior/move-effects/`
3. **Identify what commands you need** - which exist, which are new?
4. **Write test cases** in the spec (not code yet, just descriptions)
5. **Cross-reference pokeemerald** - how does it actually work?

**Time:** 30-60 minutes
**Output:** Complete effect specification document

### Phase 2: Write Failing Tests

Before implementing the commands:

1. **Create test file** for the move/effect
2. **Write tests for each behavior** from the spec
3. **Run tests** - they should all fail
4. **Commit failing tests** (optional but recommended)

```cpp
// test/unit/test_effect_ember.cpp

TEST(Effect_Ember, DealsDamage) {
    // This will fail - Effect_Ember doesn't exist yet
    auto ctx = SetupEmberContext();
    Effect_Ember(ctx);
    EXPECT_LT(ctx.defender->current_hp, ctx.defender->max_hp);
}

TEST(Effect_Ember, CanBurn) {
    // This will fail
}

TEST(Effect_Ember, FireTypeImmuneToBurn) {
    // This will fail
}
```

**Time:** 15-30 minutes
**Output:** Failing test suite that defines success

### Phase 3: Implement Minimally

Now write the code - but only what's needed to pass tests.

1. **Implement missing commands** one at a time
2. **Run tests after each command** - see progress
3. **Don't optimize** - make it work first
4. **Don't generalize** - solve this specific case

```cpp
// First pass - make it work
void TryApplyBurn(BattleContext& ctx, uint8_t chance) {
    if (ctx.move_failed) return;
    if (ctx.defender->HasType(Type::Fire)) return;
    if (ctx.defender->status1 != 0) return;

    if (Random(100) < chance) {
        ctx.defender->status1 = STATUS1_BURN;
    }
}
```

**Time:** 1-2 hours
**Output:** All tests passing

### Phase 4: Run Integration Test

One move working in isolation isn't enough. Run it in a real battle:

1. **Create a simple battle scenario** - two Pokemon, one uses your move
2. **Execute a full turn** - action selection through end-of-turn
3. **Verify state is correct** after the turn
4. **Check for crashes, assertion failures**

```cpp
TEST(Integration, EmberInBattle) {
    Engine engine = SetupBattle(Charmander, Bulbasaur);

    BattleAction action{ActionType::MOVE, Player::PLAYER, 0};  // Ember
    BattleAction enemy{ActionType::MOVE, Player::ENEMY, 0};    // Tackle

    engine.ExecuteTurn(action, enemy);

    // Verify Bulbasaur took damage
    auto& bulbasaur = engine.GetActivePokemon(Player::ENEMY);
    EXPECT_LT(bulbasaur.current_hp, bulbasaur.max_hp);
}
```

**Time:** 30 minutes
**Output:** Move works in real battle context

### Phase 5: Iterate

Now add the next move that builds on what you have:

1. **Pick a move that adds ONE new concept**
2. **Repeat phases 1-4**
3. **Refactor shared code** as patterns emerge

**Good second moves:**
- Ember → Thunder Wave (status without damage)
- Thunder Wave → Thunder (damage + status + accuracy variation)
- Thunder → Double-Edge (recoil)

---

## Choosing What to Implement Next

### Start Simple

| Priority | Move Type | Example | Why First |
|----------|-----------|---------|-----------|
| 1 | Basic damage | Tackle | Core loop |
| 2 | Damage + status | Ember | Adds one concept |
| 3 | Status only | Thunder Wave | Tests non-damage path |
| 4 | Stat change | Growl | Tests stat system |
| 5 | Recoil | Double-Edge | Tests self-damage |
| 6 | Weather | Sunny Day | Tests field state |
| 7 | Multi-turn | Solar Beam | Tests turn tracking |

### The Concept Ladder

Each move should add exactly ONE new concept:

```
Tackle        → Basic damage
Ember         → + Secondary effect (burn)
Thunder Wave  → Status without damage
Growl         → Stat modification
Sword Dance   → Self stat boost
Double-Edge   → Recoil damage
Giga Drain    → Drain HP
Protect       → Protection mechanics
Solar Beam    → Two-turn moves
Fly           → Semi-invulnerable
Substitute    → Substitute mechanics
Baton Pass    → Slot state transfer
```

---

## Test-Driven Development Checklist

For each move/feature:

- [ ] Effect spec written and cross-referenced with pokeemerald
- [ ] Test cases defined in spec
- [ ] Unit tests written (failing)
- [ ] Commands implemented
- [ ] Unit tests passing
- [ ] Integration test written
- [ ] Integration test passing
- [ ] Code reviewed for contract compliance
- [ ] Spec marked as implemented

---

## When to Refactor

**Refactor AFTER tests pass, not before.**

Signs you should refactor:
- You copy-pasted a code block for the third time
- A function has more than 3 parameters
- You're adding boolean flags to change behavior
- Tests are hard to write because setup is complex

Signs you should NOT refactor yet:
- You've only implemented 2-3 moves
- You "think" there's a pattern but aren't sure
- Everything is working fine

**Rule of Three:** Wait until you've done something three times before abstracting.

---

## Debugging Integration Issues

When something breaks during integration:

### 1. Isolate the failure

```cpp
// Add logging at each step
void ExecuteTurn(...) {
    DEBUG_LOG("Determining turn order...");
    DetermineActionOrder(...);

    DEBUG_LOG("Executing first action...");
    ExecuteAction(first);

    DEBUG_LOG("Checking battle over...");
    if (IsBattleOver()) return;

    // etc.
}
```

### 2. Check the contracts

- Is the Engine passing correct data to the effect?
- Is the effect calling commands in the right order?
- Are commands modifying the right state?

### 3. Verify state at boundaries

```cpp
void ExecuteMove(const BattleAction& action) {
    // State before
    DEBUG_LOG("Attacker HP before: %d", attacker.current_hp);

    // Execute
    g_MoveEffects[move.effect](ctx);

    // State after
    DEBUG_LOG("Attacker HP after: %d", attacker.current_hp);
    DEBUG_LOG("Defender HP after: %d", defender.current_hp);
}
```

### 4. Compare with pokeemerald

When behavior differs from expected, check pokeemerald source directly:

```bash
# Search pokeemerald for how it handles something
grep -r "STATUS1_BURN" pokeemerald/src/
```

---

## Documentation Maintenance

Keep docs in sync with implementation:

### When implementing a move:
1. Update effect spec with any discoveries
2. Mark implementation checkboxes complete
3. Add any new edge cases found during testing

### When adding a command:
1. Update `contracts/engine-commands.md` with the new command
2. Update `contracts/ownership-matrix.md` if it affects ownership

### When finding pokeemerald discrepancy:
1. Document in the relevant behavior spec
2. Note the line number in pokeemerald
3. Decide: match pokeemerald or intentional deviation?

---

## Build Targets

Use the right build for the right task:

| Task | Target | Notes |
|------|--------|-------|
| Active development | `make debug` | Full assertions, debug output |
| Running tests | `make utest` | Test binary, can be any size |
| Size check | `make` | Production build, check fits in 150KB |

**Workflow:**
```bash
# While developing
make debug && ./build/debug/poke-battle

# Running tests
make utest && ./build/test/poke-battle-tests

# Before committing - verify production build
make && ls -lh build/release/poke-battle.8xp
```

---

## Example: Implementing Tackle

### Day 1, Hour 1: Specification

Create `docs-v2/behavior/move-effects/tackle.md`:

```markdown
# Effect: HIT (Tackle)

## Behavior Specification
1. AccuracyCheck
2. CalculateDamage
3. ApplyDamage
4. CheckFaint

## Test Cases
- Basic damage is dealt
- Miss deals no damage
- Type immunity (Ghost vs Normal)
- Critical hit
```

### Day 1, Hour 2: Tests

Create `test/unit/test_effect_hit.cpp`:

```cpp
TEST(Effect_Hit, DealsDamage) { /* ... */ }
TEST(Effect_Hit, MissDealsNoDamage) { /* ... */ }
TEST(Effect_Hit, GhostImmuneToNormal) { /* ... */ }
TEST(Effect_Hit, CriticalHitDoublesDamage) { /* ... */ }
```

### Day 1, Hours 3-5: Implementation

Implement commands one at a time, running tests after each:

1. `AccuracyCheck` → run tests (some pass)
2. `CalculateDamage` → run tests (more pass)
3. `ApplyDamage` → run tests (more pass)
4. `CheckFaint` → run tests (all pass)

### Day 1, Hour 6: Integration

```cpp
TEST(Integration, TackleInBattle) {
    Engine engine = SetupBattle(Rattata, Pidgey);
    engine.ExecuteTurn(MoveAction(0), MoveAction(0));
    // Both used Tackle
    EXPECT_LT(Rattata.hp, Rattata.max_hp);
    EXPECT_LT(Pidgey.hp, Pidgey.max_hp);
}
```

### Day 1 Complete

You now have a working battle engine that can execute Tackle. It's minimal, but it works. Tomorrow you add Ember and discover if your burn system integrates correctly - with only 100 lines of code at risk, not 1000.

---

## Summary

1. **Vertical, not horizontal** - complete features, not complete layers
2. **Specify first** - think before coding
3. **Test first** - define success before implementing
4. **One concept at a time** - ladder up in complexity
5. **Integrate early** - run in real battle context
6. **Refactor late** - wait for patterns to emerge
7. **Document as you go** - keep specs in sync

This process is slower at the start but prevents the "rewrite every two weeks" cycle. You're trading initial speed for sustainable progress.

---

**Document Status:**
- [x] Initial specification
- [ ] Example walkthrough validated
- [ ] Integrated with existing build system docs
