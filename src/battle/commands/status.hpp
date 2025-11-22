/**
 * @file battle/commands/status.hpp
 * @brief Status condition application commands
 */

#pragma once

#include "../../domain/status.hpp"
#include "../context.hpp"
#include "../random.hpp"

namespace battle {
namespace commands {

/**
 * @brief Attempt to inflict Burn status on defender
 *
 * CONTRACT:
 * - Inputs: ctx.defender, chance (0-100)
 * - Outputs: Sets ctx.defender->status1 to BURN if successful
 * - Does: Check immunities, roll RNG, apply burn
 * - Does NOT: Deal damage, check accuracy (already done)
 *
 * IMMUNITIES CHECKED (Pass 1):
 * - Fire type (immune to burn)
 * - Already has a status (status1 != 0)
 * - Target fainted (current_hp == 0)
 *
 * NOT YET IMPLEMENTED (future passes):
 * - Water Veil ability
 * - Leaf Guard ability + Sun weather
 * - Safeguard field effect
 * - Substitute (Gen III: Substitute blocks damage but NOT secondary status)
 *
 * @param ctx Battle context containing attacker, defender, move
 * @param chance Burn chance (0-100, e.g., 10 for 10%)
 */
inline void TryApplyBurn(BattleContext& ctx, uint8_t chance) {
    // Guard: skip if move already failed
    if (ctx.move_failed)
        return;

    // Guard: skip if target fainted (damage already applied)
    if (ctx.defender->current_hp == 0)
        return;

    // Check immunities
    // Fire type is immune to burn
    if (ctx.defender->type1 == domain::Type::Fire || ctx.defender->type2 == domain::Type::Fire) {
        return;
    }

    // Already has a status condition (Sleep, Poison, Burn, etc.)
    if (ctx.defender->status1 != 0) {
        return;
    }

    // TODO (future): Check Water Veil ability
    // TODO (future): Check Leaf Guard ability + Sun weather
    // TODO (future): Check Safeguard on defender's side

    // Roll for burn
    if (random::Random(100) < chance) {
        ctx.defender->status1 = domain::Status1::BURN;
        // TODO (future): Add battle message: "[Pokemon] was burned!"
    }
}

/**
 * @brief Attempt to inflict Paralysis status on defender
 *
 * CONTRACT:
 * - Inputs: ctx.defender, chance (0-100)
 * - Outputs: Sets ctx.defender->status1 to PARALYSIS if successful
 * - Does: Check immunities, roll RNG, apply paralysis
 * - Does NOT: Deal damage, check accuracy (already done)
 *
 * IMMUNITIES CHECKED:
 * - Electric type (immune to paralysis from Electric-type moves like Thunder Wave)
 * - Already has a status (status1 != 0)
 * - Target fainted (current_hp == 0)
 *
 * NOT YET IMPLEMENTED (future passes):
 * - Limber ability
 * - Safeguard field effect
 * - Substitute (blocks paralysis in Gen III, unlike burn)
 *
 * @param ctx Battle context containing attacker, defender, move
 * @param chance Paralysis chance (0-100, usually 100 for Thunder Wave)
 */
inline void TryApplyParalysis(BattleContext& ctx, uint8_t chance) {
    // Guard: skip if move already failed
    if (ctx.move_failed)
        return;

    // Guard: skip if target fainted
    if (ctx.defender->current_hp == 0)
        return;

    // Check Electric type immunity
    // In Gen III, Electric types cannot be paralyzed by Electric-type moves
    // Note: Body Slam (Normal-type) CAN paralyze Electric types
    if (ctx.move->type == domain::Type::Electric) {
        if (ctx.defender->type1 == domain::Type::Electric ||
            ctx.defender->type2 == domain::Type::Electric) {
            // TODO: Display message: "It doesn't affect [Pokemon]..."
            return;
        }
    }

    // Already has a status condition (Sleep, Poison, Burn, etc.)
    if (ctx.defender->status1 != 0) {
        return;
    }

    // TODO (future): Check Limber ability
    // TODO (future): Check Safeguard on defender's side

    // Roll for paralysis
    if (random::Random(100) < chance) {
        ctx.defender->status1 = domain::Status1::PARALYSIS;
        // TODO (future): Add battle message: "[Pokemon] was paralyzed!"
    }
}

}  // namespace commands
}  // namespace battle
