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
    {domain::Move::None, domain::Type::Normal, 0, 0, 0, 0},

    // Move::Tackle
    {domain::Move::Tackle, domain::Type::Normal, 40, 100, 35, 0},

    // Move::Ember
    {domain::Move::Ember, domain::Type::Fire, 40, 100, 25, 10},

    // Move::ThunderWave
    {domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100},

    // Move::Growl
    {domain::Move::Growl, domain::Type::Normal, 0, 100, 40, 0},

    // Move::TailWhip
    {domain::Move::TailWhip, domain::Type::Normal, 0, 100, 30, 0},

    // Move::SwordsDance
    {domain::Move::SwordsDance, domain::Type::Normal, 0, 0, 30, 0},

    // Move::DoubleEdge
    {domain::Move::DoubleEdge, domain::Type::Normal, 120, 100, 15, 0},

    // Move::GigaDrain
    {domain::Move::GigaDrain, domain::Type::Grass, 60, 100, 5, 0},

    // Move::IronDefense
    {domain::Move::IronDefense, domain::Type::Normal, 0, 0, 15, 0},

    // Move::StringShot
    {domain::Move::StringShot, domain::Type::Bug, 0, 95, 40, 0},

    // Move::Agility
    {domain::Move::Agility, domain::Type::Psychic, 0, 0, 30, 0},

    // Move::TailGlow
    {domain::Move::TailGlow, domain::Type::Bug, 0, 0, 20, 0},

    // Move::FakeTears
    {domain::Move::FakeTears, domain::Type::Dark, 0, 100, 20, 0},

    // Move::Amnesia
    {domain::Move::Amnesia, domain::Type::Psychic, 0, 0, 20, 0},

    // Move::FuryAttack
    {domain::Move::FuryAttack, domain::Type::Normal, 15, 85, 20, 0},
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

void BattleEngine::ExecuteTurn(const BattleAction& player_action,
                               const BattleAction& enemy_action) {
    // Phase 3: Still hardcoded turn order - player always goes first
    // TODO Phase 4: Determine order based on speed and priority

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
