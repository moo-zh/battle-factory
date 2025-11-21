/**
 * @file battle/commands/faint.hpp
 * @brief Faint checking command
 */

#pragma once

#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Check if Pokemon has fainted and set flag
 *
 * CONTRACT:
 * - Inputs: ctx.defender or ctx.attacker (based on check_attacker), current_hp
 * - Outputs: Sets target->is_fainted if HP = 0
 * - Does: Check if HP <= 0 and set faint flag
 * - Does NOT: Process the faint (switch-in, exp, etc.) - that's Engine's job
 *
 * TARGETING:
 * - check_attacker = false (default): Check defender (normal case)
 * - check_attacker = true: Check attacker (for recoil moves, self-destruct)
 *
 * EDGE CASES:
 * - Attacker can faint from recoil (Double-Edge, Brave Bird)
 * - Both Pokemon can faint in same turn (defender from damage, attacker from recoil)
 * - HP = 0 is the only faint condition (no negative HP)
 *
 * @param ctx Battle context containing attacker and defender
 * @param check_attacker If true, check attacker; if false, check defender (default)
 */
inline void CheckFaint(BattleContext& ctx, bool check_attacker = false) {
    // Select target based on check_attacker flag
    state::Pokemon* target = check_attacker ? ctx.attacker : ctx.defender;

    // Set faint flag if HP is 0
    if (target->current_hp == 0) {
        target->is_fainted = true;
    }
}

}  // namespace commands
}  // namespace battle
