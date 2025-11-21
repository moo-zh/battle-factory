/**
 * @file battle/engine.cpp
 * @brief Battle Engine implementation (Phase 3: Generalized dispatch)
 */

#include "engine.hpp"

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
}

/**
 * @brief Calculate effective speed for turn order
 * @param pokemon The Pokemon whose speed to calculate
 * @return Effective speed (base speed * stat stage multiplier)
 *
 * Based on pokeemerald's GetWhoStrikesFirst function.
 * Formula: speed * (2 + stage) / 2  if stage >= 0
 *          speed * 2 / (2 - stage)  if stage < 0
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

    // Phase 4: No paralysis, weather, abilities, or items yet
    // Future phases will add:
    // - Paralysis (speed /= 4)
    // - Swift Swim/Chlorophyll (speed *= 2)
    // - Quick Claw (speed = UINT16_MAX)
    // - Macho Brace (speed /= 2)

    return speed;
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

    // Get move priorities (Phase 4: all moves are priority 0)
    // Future: Add priority field to MoveData
    int8_t player_priority = 0;
    int8_t enemy_priority = 0;

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
            ExecuteMove(player_, enemy_, player_action.move);
        }

        // Check if battle is over after player's move
        if (IsBattleOver()) {
            return;
        }

        // Enemy attacks second
        if (enemy_action.type == ActionType::MOVE) {
            ExecuteMove(enemy_, player_, enemy_action.move);
        }
    } else {
        // Enemy attacks first
        if (enemy_action.type == ActionType::MOVE) {
            ExecuteMove(enemy_, player_, enemy_action.move);
        }

        // Check if battle is over after enemy's move
        if (IsBattleOver()) {
            return;
        }

        // Player attacks second
        if (player_action.type == ActionType::MOVE) {
            ExecuteMove(player_, enemy_, player_action.move);
        }
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

}  // namespace battle
