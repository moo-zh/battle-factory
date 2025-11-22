# Battle Factory Documentation

Welcome to the Battle Factory documentation! This project is a Gen III Pokémon battle engine implementation for the TI-84 CE calculator.

---

## Quick Start

**New to the project?** Start here:
1. Read [WORKFLOW.md](WORKFLOW.md) - Learn our vertical slice development approach
2. Browse [moves/](moves/) - See how moves are specified and implemented
3. Check [mechanics/](mechanics/) - Understand core battle mechanics

---

## Project Overview

Battle Factory is a faithful Gen III Pokémon battle engine that runs on the TI-84 CE calculator. We're implementing the battle system one move at a time using a **vertical slice approach** - each feature is fully complete before moving to the next.

### Current Status

**✅ 10/12 Moves Implemented** (83% of concept ladder)

| Move | Type | Concept Introduced |
|------|------|-------------------|
| ✅ Tackle | Normal | Basic damage |
| ✅ Ember | Fire | Secondary effect (burn) |
| ✅ Thunder Wave | Electric | Status without damage |
| ✅ Growl | Normal | Stat modification |
| ✅ Swords Dance | Normal | Self stat boost |
| ✅ Double-Edge | Normal | Recoil damage |
| ✅ Giga Drain | Grass | Drain HP |
| ✅ Protect | Normal | Protection mechanics |
| ✅ Solar Beam | Grass | Two-turn moves |
| ✅ Fly | Flying | Semi-invulnerable |
| ⬜ Substitute | Normal | Substitute mechanics |
| ⬜ Baton Pass | Normal | Stat transfer on switch |

**Test Coverage:** 414/414 tests passing

---

## Documentation Structure

### [WORKFLOW.md](WORKFLOW.md)
Our development methodology: **vertical slice implementation**

- Why we avoid horizontal layering
- The spec → test → implement → integrate cycle
- When to refactor (Rule of Three)
- How to choose what to implement next

**Read this first** if you want to contribute or understand our development process.

### [moves/](moves/)
Individual move specifications (17 moves documented)

Each spec includes:
- Effect ID and pokeemerald reference
- Gen III mechanics and move data
- Implementation requirements
- State changes and command sequence
- Test coverage

**Template:** [moves/_TEMPLATE.md](moves/_TEMPLATE.md)

**Examples:**
- [tackle.md](moves/tackle.md) - Basic damage move
- [protect.md](moves/protect.md) - Protection with degrading success rate
- [solar-beam.md](moves/solar-beam.md) - Two-turn charging move

### [mechanics/](mechanics/)
Core battle system mechanics

- [damage.md](mechanics/damage.md) - Damage calculation formula
- [turns.md](mechanics/turns.md) - Turn execution and ordering
- [states.md](mechanics/states.md) - State transitions and management

---

## Architecture Principles

### 1. Vertical Slices Over Horizontal Layers

❌ **Don't do this:**
```
Week 1: Implement all 70 commands
Week 2: Implement all 189 effects
Week 3: Discover they don't integrate
Week 4: Rewrite everything
```

✅ **Do this instead:**
```
Day 1: Implement Tackle end-to-end (works!)
Day 2: Implement Ember (reuses Day 1 + adds burn)
Day 3: Implement Thunder Wave (tests status-only path)
```

Each day you have **working, tested code**. Integration issues are caught with 50 lines at risk, not 500.

### 2. Test-Driven Development

Every feature follows this cycle:
1. **Specify** - Write effect spec with pokeemerald cross-reference
2. **Test** - Write failing tests for all behaviors
3. **Implement** - Make tests pass with minimal code
4. **Integrate** - Run in real battle context

### 3. Rule of Three

Don't abstract until you've done something **three times**. Premature abstraction is worse than duplication.

### 4. Pokemon Fidelity

We match Gen III mechanics from pokeemerald:
- Damage formula
- Stat stage multipliers
- RNG algorithms
- Move effects and ordering

---

## Implementation Status

### Completed Systems
- ✅ Basic damage calculation
- ✅ Type effectiveness
- ✅ Stat stages (-6 to +6)
- ✅ Status conditions (Burn, Paralysis)
- ✅ Recoil and drain moves
- ✅ Multi-hit moves (2-5 hits)
- ✅ Protection mechanics
- ✅ Two-turn moves
- ✅ Speed-based turn ordering
- ✅ Critical hits
- ✅ Accuracy checks

### Next Up
- ⬜ Semi-invulnerable state (Fly, Dig)
- ⬜ Substitute mechanics
- ⬜ Stat transfer on switch (Baton Pass)
- ⬜ Weather system (Sunny Day, Rain Dance)
- ⬜ Entry hazards (Spikes, Stealth Rock)

---

## Contributing

### Adding a New Move

1. Choose a move from the [concept ladder](WORKFLOW.md#the-concept-ladder)
2. Research mechanics in pokeemerald decomp (`.claude/pokeemerald/`)
3. Write specification using [moves/_TEMPLATE.md](moves/_TEMPLATE.md)
4. Write failing tests
5. Implement effect handler
6. Add to move database and dispatch table
7. Verify all tests pass

### Testing

```bash
# Build and run tests
make utest

# Tests run on TI-84 CE emulator via CEmu
# 376 tests covering all implemented moves
```

---

## Technical Details

### Target Hardware
- **Platform:** TI-84 Plus CE
- **Language:** C++ (C++11)
- **ROM Size Limit:** ~150 KB
- **Test Binary:** No size limit (runs on emulator)

### Build System
```bash
make           # Production build (size-constrained)
make utest     # Test build (runs on CEmu emulator)
make clean     # Clean build artifacts
```

### Project Structure
```
source/
  battle/
    engine.cpp           # Main battle engine
    effects/basic.hpp    # Move effect implementations
    commands/*.hpp       # Reusable battle commands
    state/pokemon.hpp    # Pokemon runtime state
  domain/
    move.hpp             # Move definitions
    species.hpp          # Species and types
    stats.hpp            # Stat definitions

test/
  unit/
    effects/             # Per-move effect tests
    test_helpers.hpp     # Shared test factories
  integration/           # Full battle tests

docs/                    # You are here!
```

---

## References

- **pokeemerald:** Gen III Pokémon Emerald decomp (reference in `.claude/pokeemerald/`)
- **Bulbapedia:** Move and mechanic documentation
- **Smogon:** Competitive battling mechanics
- **CEmu:** TI-84 Plus CE emulator for testing

---

## License

[Add license information]

---

## Credits

Built with vertical slice methodology and test-driven development.

Powered by Gen III mechanics from the pokeemerald decompilation project.

🤖 Documentation generated with [Claude Code](https://claude.com/claude-code)
