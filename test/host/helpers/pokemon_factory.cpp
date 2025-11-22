/**
 * @file test/host/helpers/pokemon_factory.cpp
 * @brief Implementation of Pokemon factory functions
 */

#include "pokemon_factory.hpp"

namespace test {
namespace helpers {

battle::state::Pokemon CreateTestPokemon(domain::Species species, domain::Type type1,
                                         domain::Type type2, uint16_t hp, uint8_t atk, uint8_t def,
                                         uint8_t spa, uint8_t spd, uint8_t spe) {
    battle::state::Pokemon p;
    p.species = species;
    p.type1 = type1;
    p.type2 = type2;
    p.level = 5;
    p.attack = atk;
    p.defense = def;
    p.sp_attack = spa;
    p.sp_defense = spd;
    p.speed = spe;
    p.max_hp = hp;
    p.current_hp = hp;
    p.is_fainted = false;
    p.status1 = 0;  // No status

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    // Initialize protection state
    p.is_protected = false;
    p.protect_count = 0;

    // Initialize two-turn move state
    p.is_charging = false;
    p.charging_move = domain::Move::None;

    // Initialize semi-invulnerable state
    p.is_semi_invulnerable = false;
    p.semi_invulnerable_type = battle::state::SemiInvulnerableType::None;

    // Initialize substitute state
    p.has_substitute = false;
    p.substitute_hp = 0;

    return p;
}

battle::state::Pokemon CreateCharmander() {
    return CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire, domain::Type::None,
                             39,   // HP
                             52,   // Attack
                             43,   // Defense
                             60,   // Sp. Attack
                             50,   // Sp. Defense
                             65);  // Speed
}

battle::state::Pokemon CreateBulbasaur() {
    return CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass, domain::Type::Poison,
                             45,   // HP
                             49,   // Attack
                             49,   // Defense
                             65,   // Sp. Attack
                             65,   // Sp. Defense
                             45);  // Speed
}

battle::state::Pokemon CreatePikachu() {
    return CreateTestPokemon(domain::Species::Pikachu, domain::Type::Electric, domain::Type::None,
                             35,   // HP
                             55,   // Attack
                             30,   // Defense
                             50,   // Sp. Attack
                             40,   // Sp. Defense
                             90);  // Speed
}

battle::state::Pokemon CreatePidgey() {
    return CreateTestPokemon(domain::Species::Pidgey, domain::Type::Normal, domain::Type::Flying,
                             40,   // HP
                             45,   // Attack
                             40,   // Defense
                             35,   // Sp. Attack
                             35,   // Sp. Defense
                             56);  // Speed
}

battle::state::Pokemon CreatePokemonWithStats(uint8_t atk, uint8_t def, uint8_t spe, uint16_t hp) {
    return CreateTestPokemon(domain::Species::None, domain::Type::Normal, domain::Type::None, hp,
                             atk, def,
                             50,  // Default Sp. Attack
                             50,  // Default Sp. Defense
                             spe);
}

}  // namespace helpers
}  // namespace test
