/**
 * @file battle/commands/faint.hpp
 * @brief Faint checking command
 */

#pragma once

#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Check if defender has fainted and set flag
 *
 * CONTRACT:
 * - Inputs: ctx.defender->current_hp
 * - Outputs: Sets ctx.defender->is_fainted if HP = 0
 * - Does: Check if HP <= 0 and set faint flag
 * - Does NOT: Process the faint (switch-in, exp, etc.) - that's Engine's job
 */
inline void CheckFaint(BattleContext& ctx) {
    if (ctx.defender->current_hp == 0) {
        ctx.defender->is_fainted = true;
    }
}

}  // namespace commands
}  // namespace battle
