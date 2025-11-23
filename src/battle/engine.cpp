/**
 * @file battle/engine.cpp
 * @brief Battle Engine implementation (Phase 3: Generalized dispatch)
 */

#include "engine.hpp"

#include <cstddef>

#include "context.hpp"
#include "effects/basic.hpp"

namespace battle {

// ============================================================================
// PHASE 3: Move Database and Effect Dispatch Table
// ============================================================================

/**
 * @brief Move database - contains all implemented moves with their stats
 *
 * This replaces the hardcoded GetMoveData_Hardcoded function from Phase 2.
 * Indexed by Move enum value.
 */
static const domain::MoveData MOVE_DATABASE[] = {
    // Move::None
    {domain::Move::None, domain::Type::Normal, 0, 0, 0, 0, 0},

    // Move::Tackle
    {domain::Move::Tackle, domain::Type::Normal, 40, 100, 35, 0, 0},

    // Move::Ember
    {domain::Move::Ember, domain::Type::Fire, 40, 100, 25, 10, 0},

    // Move::ThunderWave
    {domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0},

    // Move::Growl
    {domain::Move::Growl, domain::Type::Normal, 0, 100, 40, 0, 0},

    // Move::TailWhip
    {domain::Move::TailWhip, domain::Type::Normal, 0, 100, 30, 0, 0},

    // Move::SwordsDance
    {domain::Move::SwordsDance, domain::Type::Normal, 0, 0, 30, 0, 0},

    // Move::DoubleEdge
    {domain::Move::DoubleEdge, domain::Type::Normal, 120, 100, 15, 0, 0},

    // Move::GigaDrain
    {domain::Move::GigaDrain, domain::Type::Grass, 60, 100, 5, 0, 0},

    // Move::IronDefense
    {domain::Move::IronDefense, domain::Type::Normal, 0, 0, 15, 0, 0},

    // Move::StringShot
    {domain::Move::StringShot, domain::Type::Bug, 0, 95, 40, 0, 0},

    // Move::Agility
    {domain::Move::Agility, domain::Type::Psychic, 0, 0, 30, 0, 0},

    // Move::TailGlow
    {domain::Move::TailGlow, domain::Type::Bug, 0, 0, 20, 0, 0},

    // Move::FakeTears
    {domain::Move::FakeTears, domain::Type::Dark, 0, 100, 20, 0, 0},

    // Move::Amnesia
    {domain::Move::Amnesia, domain::Type::Psychic, 0, 0, 20, 0, 0},

    // Move::FuryAttack
    {domain::Move::FuryAttack, domain::Type::Normal, 15, 85, 20, 0, 0},

    // Move::Protect
    {domain::Move::Protect, domain::Type::Normal, 0, 0, 10, 0, 4},

    // Move::SolarBeam
    {domain::Move::SolarBeam, domain::Type::Grass, 120, 100, 10, 0, 0},

    // Move::Fly
    {domain::Move::Fly, domain::Type::Flying, 70, 95, 15, 0, 0},

    // Move::Substitute
    {domain::Move::Substitute, domain::Type::Normal, 0, 0, 10, 0, 0},

    // Move::BatonPass
    {domain::Move::BatonPass, domain::Type::Normal, 0, 0, 40, 0, 0},

    // Move::Sandstorm
    {domain::Move::Sandstorm, domain::Type::Rock, 0, 0, 10, 0, 0},

    // Move::QuickAttack
    {domain::Move::QuickAttack, domain::Type::Normal, 40, 100, 30, 0, 1},
};

/**
 * @brief Effect function pointer type
 */
using EffectFunction = void (*)(BattleContext&);

/**
 * @brief Effect dispatch table - maps Move enum to effect function
 *
 * This replaces the hardcoded if/else chain from Phase 2.
 * Indexed by Move enum value.
 */
static const EffectFunction EFFECT_DISPATCH[] = {
    nullptr,                              // Move::None
    effects::Effect_Hit,                  // Move::Tackle
    effects::Effect_BurnHit,              // Move::Ember
    effects::Effect_Paralyze,             // Move::ThunderWave
    effects::Effect_AttackDown,           // Move::Growl
    effects::Effect_DefenseDown,          // Move::TailWhip
    effects::Effect_AttackUp2,            // Move::SwordsDance
    effects::Effect_RecoilHit,            // Move::DoubleEdge
    effects::Effect_DrainHit,             // Move::GigaDrain
    effects::Effect_DefenseUp2,           // Move::IronDefense
    effects::Effect_SpeedDown,            // Move::StringShot
    effects::Effect_SpeedUp2,             // Move::Agility
    effects::Effect_SpecialAttackUp2,     // Move::TailGlow
    effects::Effect_SpecialDefenseDown2,  // Move::FakeTears
    effects::Effect_SpecialDefenseUp2,    // Move::Amnesia
    effects::Effect_MultiHit,             // Move::FuryAttack
    effects::Effect_Protect,              // Move::Protect
    effects::Effect_SolarBeam,            // Move::SolarBeam
    effects::Effect_Fly,                  // Move::Fly
    effects::Effect_Substitute,           // Move::Substitute
    effects::Effect_BatonPass,            // Move::BatonPass
    effects::Effect_Sandstorm,            // Move::Sandstorm
    effects::Effect_Hit,                  // Move::QuickAttack
};

/**
 * @brief Number of entries in move database
 */
constexpr size_t MOVE_DATABASE_SIZE = sizeof(MOVE_DATABASE) / sizeof(MOVE_DATABASE[0]);

/**
 * @brief Number of entries in effect dispatch table
 */
constexpr size_t EFFECT_DISPATCH_SIZE = sizeof(EFFECT_DISPATCH) / sizeof(EFFECT_DISPATCH[0]);

/**
 * @brief Get move data from database
 * @param move The move to look up
 * @return MoveData for the move
 */
static const domain::MoveData& GetMoveData(domain::Move move) {
    uint8_t index = static_cast<uint8_t>(move);

    // Bounds check
    if (index >= MOVE_DATABASE_SIZE) {
        return MOVE_DATABASE[0];  // Return None if out of bounds
    }

    return MOVE_DATABASE[index];
}

/**
 * @brief Get effect function from dispatch table
 * @param move The move to look up
 * @return Effect function pointer, or nullptr if not implemented
 */
static EffectFunction GetEffectFunction(domain::Move move) {
    uint8_t index = static_cast<uint8_t>(move);

    // Bounds check
    if (index >= EFFECT_DISPATCH_SIZE) {
        return nullptr;
    }

    return EFFECT_DISPATCH[index];
}

// ============================================================================
// Battle Engine Implementation
// ============================================================================

void BattleEngine::InitBattle(const state::Pokemon& player_pokemon,
                              const state::Pokemon& enemy_pokemon) {
    player_ = player_pokemon;
    enemy_ = enemy_pokemon;

    // Initialize field state (clear weather)
    field_.weather = domain::Weather::None;
    field_.weather_duration = 0;
}

/**
 * @brief Calculate effective speed for turn order
 * @param pokemon The Pokemon whose speed to calculate
 * @return Effective speed (base speed * stat stage multiplier * status modifier)
 *
 * Based on pokeemerald's GetWhoStrikesFirst function.
 * Formula: speed * (2 + stage) / 2  if stage >= 0
 *          speed * 2 / (2 - stage)  if stage < 0
 *
 * Then apply status modifiers (paralysis divides by 4).
 */
static uint16_t CalculateEffectiveSpeed(const state::Pokemon& pokemon) {
    uint16_t speed = pokemon.speed;
    int8_t stage = pokemon.stat_stages[domain::STAT_SPEED];

    // Apply stat stage multiplier
    if (stage >= 0) {
        speed = (speed * (2 + stage)) / 2;
    } else {
        speed = (speed * 2) / (2 - stage);
    }

    // Apply paralysis speed reduction (75% reduction = divide by 4)
    // Based on pokeemerald: if (status1 & STATUS1_PARALYSIS) speed /= 4
    if (pokemon.status1 & domain::Status1::PARALYSIS) {
        speed /= 4;
    }

    // Future phases will add:
    // - Swift Swim/Chlorophyll (speed *= 2 in weather)
    // - Quick Claw (speed = UINT16_MAX)
    // - Macho Brace (speed /= 2)

    return speed;
}

/**
 * @brief Check if a Pokemon can act this turn (not prevented by status)
 * @param pokemon The Pokemon to check
 * @return true if Pokemon can act, false if prevented by status
 *
 * Checks for status conditions that prevent action:
 * - Paralysis: 25% chance to be fully paralyzed and unable to move
 * - Freeze: Cannot move (20% chance to thaw - not implemented yet)
 * - Sleep: Cannot move (counts down - not implemented yet)
 *
 * Based on pokeemerald's CheckMoveLimitations function.
 */
static bool CanActThisTurn(const state::Pokemon& pokemon) {
    // Check paralysis - 25% chance to be fully paralyzed
    // Based on pokeemerald: if (gBattleMons[battler].status1 & STATUS1_PARALYSIS)
    //                       if (Random() % 100 < 25) // fully paralyzed
    if (pokemon.status1 & domain::Status1::PARALYSIS) {
        if (random::Random(100) < 25) {
            // TODO: Display message: "[Pokemon] is paralyzed! It can't move!"
            return false;
        }
    }

    // TODO: Check freeze status (cannot move, 20% thaw chance)
    // TODO: Check sleep status (cannot move, counts down each turn)

    return true;  // Can act normally
}

/**
 * @brief Determine which player goes first
 * @param player_action Player's action
 * @param enemy_action Enemy's action
 * @return true if player goes first, false if enemy goes first
 *
 * Phase 4 implementation:
 * 1. Compare move priorities (all moves are priority 0 for now)
 * 2. Compare effective speeds (base speed * stat stages)
 * 3. If tied, 50/50 random
 *
 * Based on pokeemerald's GetWhoStrikesFirst.
 */
bool BattleEngine::DetermineTurnOrder(const BattleAction& player_action,
                                      const BattleAction& enemy_action) {
    // Phase 4: Only handle moves (no switching yet)
    if (player_action.type != ActionType::MOVE || enemy_action.type != ActionType::MOVE) {
        return true;  // Default to player first
    }

    // Get move priorities from move data
    const domain::MoveData& player_move_data = GetMoveData(player_action.move);
    const domain::MoveData& enemy_move_data = GetMoveData(enemy_action.move);
    int8_t player_priority = player_move_data.priority;
    int8_t enemy_priority = enemy_move_data.priority;

    // Compare priorities first
    if (player_priority > enemy_priority) {
        return true;  // Player has higher priority
    } else if (enemy_priority > player_priority) {
        return false;  // Enemy has higher priority
    }

    // Same priority - compare speeds
    uint16_t player_speed = CalculateEffectiveSpeed(player_);
    uint16_t enemy_speed = CalculateEffectiveSpeed(enemy_);

    if (player_speed > enemy_speed) {
        return true;  // Player is faster
    } else if (enemy_speed > player_speed) {
        return false;  // Enemy is faster
    }

    // Same speed - 50/50 random (based on pokeemerald: Random() & 1)
    return (random::Random(2) == 0);
}

void BattleEngine::ExecuteTurn(const BattleAction& player_action,
                               const BattleAction& enemy_action) {
    // Phase 4: Determine turn order based on speed and priority
    bool player_goes_first = DetermineTurnOrder(player_action, enemy_action);

    if (player_goes_first) {
        // Player attacks first
        if (player_action.type == ActionType::MOVE) {
            // Check if player can act (not prevented by paralysis/freeze/sleep)
            if (CanActThisTurn(player_)) {
                ExecuteMove(player_, enemy_, player_action.move);
            }
        }

        // Check if battle is over after player's move
        if (IsBattleOver()) {
            return;
        }

        // Enemy attacks second
        if (enemy_action.type == ActionType::MOVE) {
            // Check if enemy can act
            if (CanActThisTurn(enemy_)) {
                ExecuteMove(enemy_, player_, enemy_action.move);
            }
        }
    } else {
        // Enemy attacks first
        if (enemy_action.type == ActionType::MOVE) {
            // Check if enemy can act
            if (CanActThisTurn(enemy_)) {
                ExecuteMove(enemy_, player_, enemy_action.move);
            }
        }

        // Check if battle is over after enemy's move
        if (IsBattleOver()) {
            return;
        }

        // Player attacks second
        if (player_action.type == ActionType::MOVE) {
            // Check if player can act
            if (CanActThisTurn(player_)) {
                ExecuteMove(player_, enemy_, player_action.move);
            }
        }
    }

    // End-of-turn processing (status damage, weather, etc.)
    // Only process if battle isn't already over
    if (!IsBattleOver()) {
        EndOfTurn();
    }
}

bool BattleEngine::IsBattleOver() const {
    return player_.is_fainted || enemy_.is_fainted;
}

void BattleEngine::ExecuteMove(state::Pokemon& attacker, state::Pokemon& defender,
                               domain::Move move) {
    // Set up battle context
    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.field = &field_;

    // Get move data from database (Phase 3: table lookup)
    const domain::MoveData& move_data = GetMoveData(move);
    ctx.move = &move_data;

    // Initialize execution state
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.recoil_dealt = 0;
    ctx.drain_received = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // 1.0x (normal effectiveness)
    ctx.hit_count = 0;
    ctx.override_power = 0;
    ctx.override_type = 0;

    // Phase 3: Generalized dispatch via function pointer table
    EffectFunction effect_fn = GetEffectFunction(move);

    if (effect_fn != nullptr) {
        effect_fn(ctx);
    } else {
        // Move not implemented - fail silently
        ctx.move_failed = true;
    }
}

void BattleEngine::EndOfTurn() {
    // Process status damage for player
    if (player_.status1 & domain::Status1::BURN) {
        // Burn damage: 1/8 max HP per turn
        // Based on pokeemerald: damage = pokemon->maxHP / 8
        // If max HP < 8, damage is 0 (integer division rounds down)
        uint16_t burn_damage = player_.max_hp / 8;

        // Apply damage only if > 0, clamping at 0
        if (burn_damage > 0) {
            if (burn_damage >= player_.current_hp) {
                player_.current_hp = 0;
                player_.is_fainted = true;
            } else {
                player_.current_hp -= burn_damage;
            }
        }

        // TODO: Display message: "[Pokemon] was hurt by its burn!"
    }

    // Process status damage for enemy
    if (enemy_.status1 & domain::Status1::BURN) {
        // Burn damage: 1/8 max HP per turn
        // If max HP < 8, damage is 0 (integer division rounds down)
        uint16_t burn_damage = enemy_.max_hp / 8;

        // Apply damage only if > 0, clamping at 0
        if (burn_damage > 0) {
            if (burn_damage >= enemy_.current_hp) {
                enemy_.current_hp = 0;
                enemy_.is_fainted = true;
            } else {
                enemy_.current_hp -= burn_damage;
            }
        }

        // TODO: Display message: "[Pokemon] was hurt by its burn!"
    }

    // TODO: Add poison damage (1/8 max HP)
    // TODO: Add toxic damage (increasing: turn/16 * max HP)
    // TODO: Add Leech Seed drain

    // Weather damage (Sandstorm, Hail: 1/16 max HP)
    // Only applies if weather is active
    if (field_.weather == domain::Weather::Sandstorm) {
        // Sandstorm damages non-Rock/Ground/Steel types
        // Process player
        if (!player_.is_fainted) {
            bool is_immune =
                (player_.type1 == domain::Type::Rock || player_.type2 == domain::Type::Rock ||
                 player_.type1 == domain::Type::Ground || player_.type2 == domain::Type::Ground ||
                 player_.type1 == domain::Type::Steel || player_.type2 == domain::Type::Steel);

            if (!is_immune) {
                uint16_t weather_damage = player_.max_hp / 16;

                // Apply damage only if > 0, clamping at 0
                if (weather_damage > 0) {
                    if (weather_damage >= player_.current_hp) {
                        player_.current_hp = 0;
                        player_.is_fainted = true;
                    } else {
                        player_.current_hp -= weather_damage;
                    }
                }

                // TODO: Display message: "[Pokemon] is buffeted by the sandstorm!"
            }
        }

        // Process enemy
        if (!enemy_.is_fainted) {
            bool is_immune =
                (enemy_.type1 == domain::Type::Rock || enemy_.type2 == domain::Type::Rock ||
                 enemy_.type1 == domain::Type::Ground || enemy_.type2 == domain::Type::Ground ||
                 enemy_.type1 == domain::Type::Steel || enemy_.type2 == domain::Type::Steel);

            if (!is_immune) {
                uint16_t weather_damage = enemy_.max_hp / 16;

                // Apply damage only if > 0, clamping at 0
                if (weather_damage > 0) {
                    if (weather_damage >= enemy_.current_hp) {
                        enemy_.current_hp = 0;
                        enemy_.is_fainted = true;
                    } else {
                        enemy_.current_hp -= weather_damage;
                    }
                }

                // TODO: Display message: "[Pokemon] is buffeted by the sandstorm!"
            }
        }
    }

    // TODO: Hail weather damage (same pattern, checks for Ice type immunity)

    // Decrement weather duration
    if (field_.weather_duration > 0) {
        field_.weather_duration--;

        // Clear weather when duration reaches 0
        if (field_.weather_duration == 0) {
            field_.weather = domain::Weather::None;
            // TODO: Display message: "The sandstorm subsided."
        }
    }

    // TODO: Decrement screen counters (Light Screen, Reflect)
    // TODO: Check for Future Sight delayed damage
}

}  // namespace battle
