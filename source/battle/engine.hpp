/**
 * @file battle/engine.hpp
 * @brief Battle Engine - orchestrates turn execution
 *
 * IMPLEMENTATION STRATEGY: Walking Skeleton (ADR-001)
 *
 * Phase 1 (CURRENT): Minimal Engine - Tackle only, hardcoded turn order
 * - InitBattle: Set up two Pokemon
 * - ExecuteTurn: Player attacks, then enemy attacks (no speed check)
 * - ExecuteMove: Hardcoded Effect_Hit dispatch only
 * - Goal: Prove Engine ↔ Effect integration works
 *
 * Phase 2 (TODO): Add Effect_Paralyze (status-only move)
 * Phase 3 (TODO): Generalize dispatch with function pointer table
 * Phase 4 (TODO): Add turn order logic (speed-based)
 */

#pragma once

#include <stdint.h>

#include "../domain/move.hpp"
#include "state/pokemon.hpp"

namespace battle {

/**
 * @brief Action types for turn execution
 */
enum class ActionType : uint8_t {
    MOVE,  // Use a move
    // TODO: SWITCH, ITEM, RUN for later phases
};

/**
 * @brief Player identifier
 */
enum class Player : uint8_t {
    PLAYER = 0,
    ENEMY = 1,
};

/**
 * @brief Battle action (move selection)
 */
struct BattleAction {
    ActionType type;
    Player player;
    uint8_t move_slot;  // Which move to use (0-3)
    domain::Move move;  // Phase 2: Explicit move (TODO: lookup from move_slot)
};

/**
 * @brief Battle Engine - orchestrates turn execution
 *
 * Phase 1: Minimal implementation
 * - Only handles ActionType::MOVE
 * - Only dispatches Effect_Hit (Tackle)
 * - Hardcoded turn order (player always first)
 * - No switching, items, or fleeing
 */
class BattleEngine {
   public:
    /**
     * @brief Initialize a battle with two Pokemon
     * @param player_pokemon The player's Pokemon
     * @param enemy_pokemon The enemy's Pokemon
     */
    void InitBattle(const state::Pokemon& player_pokemon, const state::Pokemon& enemy_pokemon);

    /**
     * @brief Execute one turn of battle
     * @param player_action The player's action
     * @param enemy_action The enemy's action
     *
     * Phase 1: Player always goes first (ignores speed)
     */
    void ExecuteTurn(const BattleAction& player_action, const BattleAction& enemy_action);

    /**
     * @brief Check if battle is over
     * @return true if either Pokemon has fainted
     */
    bool IsBattleOver() const;

    /**
     * @brief Get the player's active Pokemon (for testing)
     */
    const state::Pokemon& GetPlayer() const { return player_; }

    /**
     * @brief Get the enemy's active Pokemon (for testing)
     */
    const state::Pokemon& GetEnemy() const { return enemy_; }

   private:
    /**
     * @brief Determine which player goes first this turn
     * @param player_action The player's action
     * @param enemy_action The enemy's action
     * @return true if player goes first, false if enemy goes first
     *
     * Phase 4: Speed-based turn order
     * - Compares move priorities (all 0 for Phase 4)
     * - Compares effective speeds (base speed * stat stages)
     * - Random tiebreaker if speeds equal
     */
    bool DetermineTurnOrder(const BattleAction& player_action, const BattleAction& enemy_action);

    /**
     * @brief Execute a single move
     * @param attacker The attacking Pokemon
     * @param defender The defending Pokemon
     * @param move The move being used
     *
     * Phase 1: Only handles Move::Tackle (Effect_Hit)
     */
    void ExecuteMove(state::Pokemon& attacker, state::Pokemon& defender, domain::Move move);

    // Battle state
    state::Pokemon player_;
    state::Pokemon enemy_;
};

}  // namespace battle
