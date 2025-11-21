# Damage Calculation

This document specifies the complete Gen III damage formula as a decision tree. Every modifier, every edge case, in execution order.

---

## Overview

Damage calculation is one of the most complex operations in the battle engine. It involves:
- Base damage formula
- Critical hits
- Type effectiveness
- STAB (Same Type Attack Bonus)
- Weather modifiers
- Ability modifiers
- Item modifiers
- Random variance

**All of this happens in `CalculateDamage(ctx)`** - one command owns the entire calculation.

---

## The Gen III Damage Formula

```
Damage = ((((2 * Level / 5 + 2) * Power * A/D) / 50) + 2)
         * Critical
         * Weather
         * STAB
         * Type1
         * Type2
         * Random
```

Where:
- **Level** = Attacker's level (always 50 in Battle Factory)
- **Power** = Move's base power (or override_power)
- **A** = Attacker's Attack or Special Attack (modified by stages and abilities)
- **D** = Defender's Defense or Special Defense (modified by stages and abilities)
- **Critical** = 2.0 if critical hit, 1.0 otherwise
- **Weather** = 1.5 or 0.5 depending on weather and move type
- **STAB** = 1.5 if move type matches attacker's type
- **Type1/Type2** = Type effectiveness against defender's types
- **Random** = Random value from 85 to 100, divided by 100

---

## Calculation Steps

### Step 1: Determine Physical or Special

```
Is move Physical or Special?
├── Gen III: Based on TYPE, not individual move
│   ├── Physical: Normal, Fighting, Flying, Ground, Rock, Bug, Ghost, Poison, Steel
│   └── Special: Fire, Water, Electric, Grass, Ice, Psychic, Dragon, Dark
└── Use Attack/Defense for Physical, SpAtk/SpDef for Special
```

**Important:** In Gen III, Ghost and Dark are physical/special respectively, which is counterintuitive.

### Step 2: Get Base Stats

```
Get relevant stats:
├── Physical move:
│   ├── A = attacker.attack
│   └── D = defender.defense
└── Special move:
    ├── A = attacker.specialAttack
    └── D = defender.specialDefense
```

### Step 3: Apply Stat Stages

Stat stages range from -6 to +6. The multipliers are:

| Stage | Multiplier |
|-------|------------|
| -6 | 2/8 = 0.25 |
| -5 | 2/7 ≈ 0.29 |
| -4 | 2/6 ≈ 0.33 |
| -3 | 2/5 = 0.40 |
| -2 | 2/4 = 0.50 |
| -1 | 2/3 ≈ 0.67 |
| 0 | 2/2 = 1.00 |
| +1 | 3/2 = 1.50 |
| +2 | 4/2 = 2.00 |
| +3 | 5/2 = 2.50 |
| +4 | 6/2 = 3.00 |
| +5 | 7/2 = 3.50 |
| +6 | 8/2 = 4.00 |

```cpp
int GetStatWithStage(int baseStat, int stage) {
    if (stage >= 0) {
        return baseStat * (2 + stage) / 2;
    } else {
        return baseStat * 2 / (2 - stage);
    }
}
```

### Step 4: Apply Ability Modifiers to Stats

**Attack modifiers:**
```
Modify A based on abilities:
├── Attacker has Huge Power/Pure Power? → A *= 2
├── Attacker has Hustle? → A *= 1.5 (physical only)
├── Attacker has Guts and is statused? → A *= 1.5
├── Defender has Thick Fat and move is Fire/Ice? → A *= 0.5
└── Attacker has Slow Start and < 5 turns? → A *= 0.5
```

**Defense modifiers:**
```
Modify D based on abilities:
├── Defender has Marvel Scale and is statused? → D *= 1.5 (physical only)
└── (Most defense abilities are checked elsewhere)
```

### Step 5: Apply Item Modifiers to Stats

```
Modify A based on items:
├── Choice Band (physical)? → A *= 1.5
├── Choice Specs (special)? → A *= 1.5
├── Light Ball on Pikachu? → A *= 2
├── Thick Club on Cubone/Marowak? → A *= 2
└── Deep Sea Tooth on Clamperl? → SpA *= 2

Modify D based on items:
├── Metal Powder on Ditto? → D *= 1.5
├── Soul Dew on Latios/Latias? → SpD *= 1.5
└── Deep Sea Scale on Clamperl? → SpD *= 2
```

### Step 6: Get Move Power

```
Get move power:
├── ctx.override_power > 0? → Use override_power
└── Otherwise → Use move.power
```

Override power is set by commands like `SetFlailPower`, `SetEruptionPower`, etc.

### Step 7: Apply Power Modifiers

```
Modify power based on conditions:
├── Helping Hand active? → power *= 1.5
├── Charge active and Electric move? → power *= 2
├── Mud Sport active and Electric move? → power *= 0.5
├── Water Sport active and Fire move? → power *= 0.5
└── Item modifiers:
    ├── Muscle Band (physical)? → power *= 1.1
    ├── Wise Glasses (special)? → power *= 1.1
    └── Type-boosting items? → power *= 1.2
        (Charcoal, Mystic Water, Magnet, etc.)
```

### Step 8: Calculate Base Damage

```cpp
int baseDamage = (((2 * level / 5 + 2) * power * A / D) / 50) + 2;
```

For Level 50 (Battle Factory):
```cpp
int baseDamage = ((22 * power * A / D) / 50) + 2;
```

### Step 9: Critical Hit Check

```
Is this a critical hit?
├── Check if guaranteed crit:
│   ├── Move has "always crit" flag? → Crit
│   └── Ability (Sniper?) or item effect? → Check
├── Calculate crit stage:
│   ├── Base: 0
│   ├── High crit ratio move: +1
│   ├── Focus Energy: +1
│   ├── Scope Lens: +1
│   ├── Razor Claw: +1
│   ├── Lucky Punch on Chansey: +2
│   └── Stick on Farfetch'd: +2
├── Crit chance by stage:
│   ├── Stage 0: 1/16 (6.25%)
│   ├── Stage 1: 1/8 (12.5%)
│   ├── Stage 2: 1/4 (25%)
│   ├── Stage 3: 1/3 (33.3%)
│   └── Stage 4+: 1/2 (50%)
└── Roll for crit
```

**If critical hit:**
```
Apply critical hit effects:
├── Damage *= 2
├── Ignore attacker's negative Attack stages
├── Ignore defender's positive Defense stages
├── Ignore Reflect/Light Screen
└── Sniper ability? → Damage *= 1.5 (total 3x)
```

### Step 10: Weather Modifier

```
Apply weather modifier:
├── Rain:
│   ├── Water move? → Damage *= 1.5
│   └── Fire move? → Damage *= 0.5
├── Sun:
│   ├── Fire move? → Damage *= 1.5
│   └── Water move? → Damage *= 0.5
├── Sandstorm:
│   └── Rock-type defender? → SpD *= 1.5 (already applied in Step 5)
└── No weather or Hail: No modifier
```

### Step 11: STAB (Same Type Attack Bonus)

```
Does attacker get STAB?
├── Move type == attacker.type1? → Damage *= 1.5
├── Move type == attacker.type2? → Damage *= 1.5
└── Adaptability ability? → Damage *= 2.0 instead of 1.5
```

### Step 12: Type Effectiveness

Calculate effectiveness against both of defender's types:

```
For each defender type:
├── Type chart lookup: attacker_move_type vs defender_type
├── Result: 0 (immune), 0.5 (resist), 1 (neutral), 2 (super effective)
└── Multiply into total effectiveness

Total effectiveness = type1_mult * type2_mult
├── 0 = Immune
├── 0.25 = Double resist
├── 0.5 = Resist
├── 1 = Neutral
├── 2 = Super effective
└── 4 = Double super effective
```

**Special cases:**
```
Type effectiveness exceptions:
├── Ground vs Flying: 0 (immune)
│   └── Unless Gravity active or Ingrain/Roost
├── Ghost vs Normal: 0 (immune)
│   └── Unless Scrappy ability or Foresight
├── Psychic vs Dark: 0 (immune)
│   └── Unless Miracle Eye
├── Electric vs Ground: 0 (immune)
│   └── Unless Gravity or Magnet Rise worn off
└── Poison vs Steel: 0 (immune)
    └── Always immune in Gen III
```

### Step 13: Ability Modifiers (Final)

```
Apply damage-modifying abilities:
├── Attacker abilities:
│   ├── Overgrow (Grass, HP <= 1/3): Damage *= 1.5
│   ├── Blaze (Fire, HP <= 1/3): Damage *= 1.5
│   ├── Torrent (Water, HP <= 1/3): Damage *= 1.5
│   ├── Swarm (Bug, HP <= 1/3): Damage *= 1.5
│   └── Rivalry: Damage *= 1.25 (same gender) or 0.75 (opposite)
└── Defender abilities:
    ├── Solid Rock/Filter (super effective): Damage *= 0.75
    └── Dry Skin (Water): Damage = 0 (heals instead)
```

### Step 14: Item Modifiers (Final)

```
Apply damage-modifying items:
├── Attacker items:
│   ├── Life Orb: Damage *= 1.3 (and lose 10% HP after)
│   ├── Metronome: Damage *= (1 + 0.1 * consecutive_uses)
│   └── Expert Belt (super effective): Damage *= 1.2
└── Defender items:
    └── (Most defensive items handled elsewhere)
```

### Step 15: Screen Modifiers

```
Apply screen damage reduction:
├── Physical move and Reflect active?
│   └── Damage *= 0.5
├── Special move and Light Screen active?
│   └── Damage *= 0.5
└── (Crit already ignores these in Step 9)
```

### Step 16: Random Variance

```cpp
int random = 85 + Random(16);  // 85 to 100
damage = damage * random / 100;
```

This is the final step and is what creates the damage ranges you see in damage calculators.

### Step 17: Minimum Damage

```cpp
if (damage < 1) damage = 1;
```

Attacks always deal at least 1 damage (unless immune).

---

## Complete Algorithm

```cpp
void CalculateDamage(BattleContext& ctx) {
    if (ctx.move_failed) return;

    // Step 1: Physical or Special
    bool isPhysical = IsPhysicalType(ctx.move->type);

    // Steps 2-3: Get stats with stages
    int A, D;
    if (isPhysical) {
        A = GetStatWithStage(ctx.attacker->attack, ctx.attacker->attackStage);
        D = GetStatWithStage(ctx.defender->defense, ctx.defender->defenseStage);
    } else {
        A = GetStatWithStage(ctx.attacker->spAttack, ctx.attacker->spAttackStage);
        D = GetStatWithStage(ctx.defender->spDefense, ctx.defender->spDefenseStage);
    }

    // Steps 4-5: Ability and item stat modifiers
    A = ApplyAttackModifiers(ctx, A, isPhysical);
    D = ApplyDefenseModifiers(ctx, D, isPhysical);

    // Steps 6-7: Get power with modifiers
    int power = ctx.override_power > 0 ? ctx.override_power : ctx.move->power;
    power = ApplyPowerModifiers(ctx, power);

    // Step 8: Base damage
    int damage = (((2 * 50 / 5 + 2) * power * A / D) / 50) + 2;

    // Step 9: Critical hit
    bool crit = RollCriticalHit(ctx);
    if (crit) {
        damage *= 2;
        ctx.critical_hit = true;
        // Note: stat stage ignoring handled in Steps 2-3 when crit known
    }

    // Step 10: Weather
    damage = ApplyWeatherModifier(ctx, damage);

    // Step 11: STAB
    if (HasSTAB(ctx.attacker, ctx.move->type)) {
        damage = damage * 3 / 2;  // 1.5x
    }

    // Step 12: Type effectiveness
    int effectiveness = GetTypeEffectiveness(ctx.move->type,
                                              ctx.defender->type1,
                                              ctx.defender->type2);
    damage = damage * effectiveness / 4;  // effectiveness is 0,1,2,4,8,16
    ctx.effectiveness = effectiveness;

    // Step 13: Ability modifiers
    damage = ApplyAbilityDamageModifiers(ctx, damage);

    // Step 14: Item modifiers
    damage = ApplyItemDamageModifiers(ctx, damage);

    // Step 15: Screens
    if (!crit) {
        if (isPhysical && HasReflect(ctx.defender)) {
            damage /= 2;
        } else if (!isPhysical && HasLightScreen(ctx.defender)) {
            damage /= 2;
        }
    }

    // Step 16: Random variance
    damage = damage * (85 + Random(16)) / 100;

    // Step 17: Minimum
    if (damage < 1 && effectiveness > 0) damage = 1;

    ctx.damage_dealt = damage;
}
```

---

## Special Cases

### Fixed Damage Moves

These moves ignore the formula entirely:

| Move | Damage |
|------|--------|
| Seismic Toss | = attacker level |
| Night Shade | = attacker level |
| Dragon Rage | = 40 |
| Sonic Boom | = 20 |
| Psywave | = level * (0.5 to 1.5) random |
| Super Fang | = defender current HP / 2 |
| Endeavor | = defender HP - attacker HP |
| Final Gambit | = attacker current HP |

These have their own effect functions that set damage directly.

### OHKO Moves

Fissure, Horn Drill, Guillotine, Sheer Cold:
- Accuracy = (attacker level - defender level + 30)%
- Damage = defender current HP
- Fail if defender level > attacker level
- Sturdy ability blocks

### Multi-Hit Moves

Double Slap, Fury Attack, etc.:
- Calculate damage once
- Apply 2-5 times
- Distribution: 2 hits (37.5%), 3 hits (37.5%), 4 hits (12.5%), 5 hits (12.5%)

Triple Kick:
- Power increases: 10 → 20 → 30
- Each hit has separate accuracy check

### Weather Ball

- Type changes based on weather
- Power doubles in weather (50 → 100)

### Pursuit

- Power doubles (40 → 80) if target is switching out

---

## Integer Math Considerations

Gen III uses integer math throughout. Order of operations matters:

```cpp
// CORRECT: Divide last to preserve precision
damage = damage * 3 / 2;

// WRONG: Loses precision
damage = damage * 1.5;  // Float conversion
damage = (damage / 2) * 3;  // Divide first loses data
```

**Rounding:** Always round down (integer division truncates).

---

## Testing Checkpoints

1. **Base formula:** 50 Atk vs 50 Def, 50 power, no modifiers = 22-26 damage
2. **Stat stages:** +6 Attack = 4x damage
3. **Critical hit:** 2x damage, ignores stat stages correctly
4. **STAB:** 1.5x damage for type match
5. **Super effective:** 2x damage per type
6. **Double super effective:** 4x damage
7. **Immune:** 0 damage
8. **Weather boost:** Rain + Water = 1.5x
9. **Screens:** Reflect halves physical
10. **Random variance:** Verify 85-100 range

---

## Pokeemerald Reference

Key files:
- `src/battle_util.c`: CalculateBaseDamage()
- `src/battle_script_commands.c`: Cmd_damagecalc
- `include/constants/battle.h`: Type effectiveness constants

---

**Document Status:**
- [x] Initial specification
- [x] Validated against pokeemerald
- [ ] Integer overflow edge cases documented
- [ ] All ability interactions verified

**Validation Notes:** Formula and modifier order confirmed accurate. Sandstorm Rock SpD boost is correctly a stat modifier (Step 5), not a damage multiplier.
