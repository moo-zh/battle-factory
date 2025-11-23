/**
 * @file battle/commands/abilities.hpp
 * @brief Ability trigger functions
 *
 * Handles passive ability effects that trigger on various events:
 * - Switch-in (Intimidate, weather abilities)
 * - Taking damage (Static, Rough Skin)
 * - Using moves (Blaze, Overgrow)
 * - Immunity checks (Levitate, Water Absorb)
 */

#pragma once

#include "../../domain/ability.hpp"
#include "../../domain/stats.hpp"
#include "../context.hpp"
#include "stat_modify.hpp"

namespace battle {
namespace commands {

/**
 * @brief Trigger switch-in abilities
 *
 * Called when a Pokemon switches in (including battle start).
 * Processes abilities that activate on switch-in:
 * - Intimidate: Lowers opponent's Attack by 1 stage
 * - Weather abilities (Drizzle, Drought, Sand Stream): Set weather
 * - Trace: Copy opponent's ability
 *
 * DESIGN DECISION:
 * - Abilities are triggered via BattleContext (same pattern as move effects)
 * - Switch-in abilities affect the opponent (ctx.defender)
 * - Stat modifications use existing ModifyStatStage() command
 *
 * IMPLEMENTATION NOTES:
 * - For Intimidate: ModifyStatStage(..., STAT_ATK, -1, false)
 *   - affects_user = false means target is defender (opponent)
 * - Future abilities will add more cases here
 *
 * Based on pokeemerald: src/battle_util.c:2590 (AbilityBattleEffects)
 * Switch-in abilities are case ABILITYEFFECT_ON_SWITCHIN
 *
 * @param ctx Battle context with attacker (switching in) and defender (opponent)
 */
inline void TriggerSwitchInAbilities(BattleContext& ctx) {
    // Check the Pokemon switching in (attacker) for switch-in abilities
    switch (ctx.attacker->ability) {
        case domain::Ability::Intimidate:
            // Lower opponent's Attack by 1 stage
            // Note: ModifyStatStage with affects_user=false targets defender
            ModifyStatStage(ctx, domain::STAT_ATK, -1, false);
            break;

        case domain::Ability::None:
            // No ability, do nothing
            break;

            // Future abilities:
            // case domain::Ability::SandStream:
            //     SetWeather(ctx, Weather::Sandstorm, 0); // Permanent in Gen III
            //     break;
            // case domain::Ability::Drizzle:
            //     SetWeather(ctx, Weather::Rain, 0);
            //     break;
    }
}

}  // namespace commands
}  // namespace battle
