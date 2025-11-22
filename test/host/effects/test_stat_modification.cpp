/**
 * @file test/host/effects/test_stat_modification.cpp
 * @brief Stat modification effect tests
 *
 * Tests for moves that modify battle stats (Attack, Defense, Speed, Sp. Attack, Sp. Defense).
 * Consolidates tests from 9 archived foundation test files into a single organized suite.
 *
 * Migrated from:
 * - test_attack_down.cpp (Growl)
 * - test_attack_up_2.cpp (Swords Dance)
 * - test_defense_down.cpp (Tail Whip)
 * - test_defense_up_2.cpp (Iron Defense)
 * - test_speed_down.cpp (String Shot)
 * - test_speed_up_2.cpp (Agility)
 * - test_special_attack_up_2.cpp (Tail Glow)
 * - test_special_defense_down_2.cpp (Fake Tears)
 * - test_special_defense_up_2.cpp (Amnesia)
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace domain;  // For STAT_ constants

/**
 * @brief Test fixture for stat modification tests
 */
class StatModificationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

// ============================================================================
// Attack Down Tests (Growl) - Migrated from test_attack_down.cpp
// ============================================================================

TEST_F(StatModificationTest, AttackDown_LowersAttackStage) {
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1) << "Growl should lower Attack by 1 stage";
}

TEST_F(StatModificationTest, AttackDown_DoesNotDealDamage) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.current_hp, original_hp) << "Growl should not deal damage";
    EXPECT_EQ(ctx.damage_dealt, 0) << "Growl should not calculate damage";
}

TEST_F(StatModificationTest, AttackDown_CanStackMultipleTimes) {
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_AttackDown(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -3) << "Growl should stack to -3";
}

TEST_F(StatModificationTest, AttackDown_MinimumStageMinus6) {
    defender.stat_stages[STAT_ATK] = -6;
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -6) << "Attack stage should not go below -6";
}

TEST_F(StatModificationTest, AttackDown_CanLowerFromPositiveStages) {
    defender.stat_stages[STAT_ATK] = 2;
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], 1) << "Growl should lower from +2 to +1";
}

TEST_F(StatModificationTest, AttackDown_DoesNotModifyAttacker) {
    int8_t original_stage = attacker.stat_stages[STAT_ATK];
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], original_stage)
        << "Growl should not affect attacker's stats";
}

TEST_F(StatModificationTest, AttackDown_DoesNotAffectOtherStats) {
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1) << "Attack should be -1";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0) << "Defense should be unchanged";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0) << "Speed should be unchanged";
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], 0) << "Sp. Attack should be unchanged";
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], 0) << "Sp. Defense should be unchanged";
}

TEST_F(StatModificationTest, AttackDown_DoesNotCauseFaint) {
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);

    battle::effects::Effect_AttackDown(ctx);

    EXPECT_FALSE(defender.is_fainted) << "Growl should not cause faint";
    EXPECT_GT(defender.current_hp, 0) << "Defender should still have HP";
}

TEST_F(StatModificationTest, AttackDown_IntegrationWithDamage) {
    domain::MoveData tackle = CreateTackle();
    battle::state::Pokemon defender1 = CreateBulbasaur();
    battle::state::Pokemon defender2 = CreateBulbasaur();

    // Normal damage
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender1, &tackle);
    battle::effects::Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with -1 Attack
    attacker.stat_stages[STAT_ATK] = -1;
    battle::random::Initialize(42);
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender2, &tackle);
    battle::effects::Effect_Hit(ctx2);
    uint16_t reduced_damage = ctx2.damage_dealt;

    EXPECT_LT(reduced_damage, normal_damage) << "Damage should be reduced with -1 Attack stage";

    // Allow 2 damage rounding error
    uint16_t expected_damage = (normal_damage * 2) / 3;
    int16_t diff = std::abs((int16_t)reduced_damage - (int16_t)expected_damage);
    EXPECT_LE(diff, 2) << "Reduced damage should be approximately 2/3 of normal damage";
}

// ============================================================================
// Attack Up Tests (Swords Dance) - Migrated from test_attack_up_2.cpp
// ============================================================================

TEST_F(StatModificationTest, AttackUp2_RaisesAttackStage) {
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +2)
        << "Swords Dance should raise user's Attack by 2 stages";
}

TEST_F(StatModificationTest, AttackUp2_DoesNotDealDamage) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(defender.current_hp, original_hp) << "Swords Dance should not deal damage";
    EXPECT_EQ(ctx.damage_dealt, 0) << "Swords Dance should not calculate damage";
}

TEST_F(StatModificationTest, AttackUp2_CanStackToMax) {
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_AttackUp2(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6) << "Swords Dance should stack to +6 (cap)";
}

TEST_F(StatModificationTest, AttackUp2_MaximumStagePlus6) {
    attacker.stat_stages[STAT_ATK] = +6;
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6) << "Attack stage should not go above +6";
}

TEST_F(StatModificationTest, AttackUp2_CapsAtPlus6FromPlus5) {
    attacker.stat_stages[STAT_ATK] = +5;
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +6)
        << "Swords Dance should cap at +6 (only +1 effective from +5)";
}

TEST_F(StatModificationTest, AttackUp2_CanRaiseFromNegativeStages) {
    attacker.stat_stages[STAT_ATK] = -3;
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], -1) << "Swords Dance should raise from -3 to -1";
}

TEST_F(StatModificationTest, AttackUp2_DoesNotModifyDefender) {
    int8_t original_stage = defender.stat_stages[STAT_ATK];
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], original_stage)
        << "Swords Dance should not affect defender's stats";
}

TEST_F(StatModificationTest, AttackUp2_DoesNotAffectOtherStats) {
    domain::MoveData swords_dance = CreateSwordsDance();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &swords_dance);

    battle::effects::Effect_AttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +2) << "Attack should be +2";
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], 0) << "Defense should be unchanged";
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], 0) << "Speed should be unchanged";
    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], 0) << "Sp. Attack should be unchanged";
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], 0) << "Sp. Defense should be unchanged";
}

TEST_F(StatModificationTest, AttackUp2_IntegrationDoublesDamage) {
    domain::MoveData tackle = CreateTackle();
    battle::state::Pokemon defender1 = CreateBulbasaur();
    battle::state::Pokemon defender2 = CreateBulbasaur();

    // Normal damage
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender1, &tackle);
    battle::effects::Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with +2 Attack (2x multiplier)
    attacker.stat_stages[STAT_ATK] = +2;
    battle::random::Initialize(42);
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender2, &tackle);
    battle::effects::Effect_Hit(ctx2);
    uint16_t boosted_damage = ctx2.damage_dealt;

    EXPECT_GT(boosted_damage, normal_damage) << "Damage should be increased with +2 Attack stage";

    // Allow 2 damage rounding error
    uint16_t expected_damage = normal_damage * 2;
    int16_t diff = std::abs((int16_t)boosted_damage - (int16_t)expected_damage);
    EXPECT_LE(diff, 2) << "Boosted damage should be approximately 2x of normal damage";
}

// ============================================================================
// Defense Modification Tests - Migrated from test_defense_down.cpp & test_defense_up_2.cpp
// ============================================================================

TEST_F(StatModificationTest, DefenseDown_LowersDefenseStage) {
    domain::MoveData tail_whip = CreateTailWhip();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tail_whip);

    battle::effects::Effect_DefenseDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], -1) << "Tail Whip should lower Defense by 1 stage";
}

TEST_F(StatModificationTest, DefenseUp2_RaisesDefenseStage) {
    domain::MoveData iron_defense = CreateIronDefense();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &iron_defense);

    battle::effects::Effect_DefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +2)
        << "Iron Defense should raise user's Defense by 2 stages";
}

TEST_F(StatModificationTest, Defense_MinMaxBounds) {
    // Test minimum
    defender.stat_stages[STAT_DEF] = -6;
    domain::MoveData tail_whip = CreateTailWhip();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &tail_whip);
    battle::effects::Effect_DefenseDown(ctx1);
    EXPECT_EQ(defender.stat_stages[STAT_DEF], -6) << "Defense should not go below -6";

    // Test maximum
    attacker.stat_stages[STAT_DEF] = +6;
    domain::MoveData iron_defense = CreateIronDefense();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &iron_defense);
    battle::effects::Effect_DefenseUp2(ctx2);
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], +6) << "Defense should not go above +6";
}

// ============================================================================
// Speed Modification Tests - Migrated from test_speed_down.cpp & test_speed_up_2.cpp
// ============================================================================

TEST_F(StatModificationTest, SpeedDown_LowersSpeedStage) {
    domain::MoveData string_shot = CreateStringShot();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &string_shot);

    battle::effects::Effect_SpeedDown(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -1) << "String Shot should lower Speed by 1 stage";
}

TEST_F(StatModificationTest, SpeedUp2_RaisesSpeedStage) {
    domain::MoveData agility = CreateAgility();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &agility);

    battle::effects::Effect_SpeedUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +2)
        << "Agility should raise user's Speed by 2 stages";
}

TEST_F(StatModificationTest, Speed_MinMaxBounds) {
    // Test minimum
    defender.stat_stages[STAT_SPEED] = -6;
    domain::MoveData string_shot = CreateStringShot();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &string_shot);
    battle::effects::Effect_SpeedDown(ctx1);
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -6) << "Speed should not go below -6";

    // Test maximum
    attacker.stat_stages[STAT_SPEED] = +6;
    domain::MoveData agility = CreateAgility();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &agility);
    battle::effects::Effect_SpeedUp2(ctx2);
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +6) << "Speed should not go above +6";
}

// ============================================================================
// Special Attack Tests - Migrated from test_special_attack_up_2.cpp
// ============================================================================

TEST_F(StatModificationTest, SpecialAttackUp2_RaisesSpecialAttackStage) {
    domain::MoveData tail_glow = CreateTailGlow();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tail_glow);

    battle::effects::Effect_SpecialAttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], +2)
        << "Tail Glow should raise user's Sp. Attack by 2 stages";
}

TEST_F(StatModificationTest, SpecialAttack_StacksToMax) {
    domain::MoveData tail_glow = CreateTailGlow();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tail_glow);

    battle::effects::Effect_SpecialAttackUp2(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_SpecialAttackUp2(ctx);
    ctx.move_failed = false;
    battle::effects::Effect_SpecialAttackUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], +6) << "Tail Glow should stack to +6 (cap)";
}

// ============================================================================
// Special Defense Tests - Migrated from test_special_defense_down_2.cpp &
// test_special_defense_up_2.cpp
// ============================================================================

TEST_F(StatModificationTest, SpecialDefenseDown2_LowersSpecialDefenseStage) {
    domain::MoveData fake_tears = CreateFakeTears();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &fake_tears);

    battle::effects::Effect_SpecialDefenseDown2(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], -2)
        << "Fake Tears should lower Sp. Defense by 2 stages";
}

TEST_F(StatModificationTest, SpecialDefenseUp2_RaisesSpecialDefenseStage) {
    domain::MoveData amnesia = CreateAmnesia();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &amnesia);

    battle::effects::Effect_SpecialDefenseUp2(ctx);

    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], +2)
        << "Amnesia should raise user's Sp. Defense by 2 stages";
}

TEST_F(StatModificationTest, SpecialDefense_MinMaxBounds) {
    // Test minimum
    defender.stat_stages[STAT_SPDEF] = -6;
    domain::MoveData fake_tears = CreateFakeTears();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &fake_tears);
    battle::effects::Effect_SpecialDefenseDown2(ctx1);
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], -6) << "Sp. Defense should not go below -6";

    // Test maximum
    attacker.stat_stages[STAT_SPDEF] = +6;
    domain::MoveData amnesia = CreateAmnesia();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &amnesia);
    battle::effects::Effect_SpecialDefenseUp2(ctx2);
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], +6) << "Sp. Defense should not go above +6";
}

// ============================================================================
// Cross-Stat Tests - Verify isolation between different stats
// ============================================================================

TEST_F(StatModificationTest, StatModifications_AreIndependent) {
    // Modify each stat differently
    attacker.stat_stages[STAT_ATK] = +2;
    attacker.stat_stages[STAT_DEF] = -1;
    attacker.stat_stages[STAT_SPEED] = +1;
    attacker.stat_stages[STAT_SPATK] = -2;
    attacker.stat_stages[STAT_SPDEF] = +3;

    // Apply Growl (should only affect Attack)
    domain::MoveData growl = CreateGrowl();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &growl);
    battle::effects::Effect_AttackDown(ctx);

    // Verify only Attack changed on defender, attacker stats unchanged
    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1) << "Only Attack should be modified";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], 0);

    // Verify attacker stats unchanged
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], +2);
    EXPECT_EQ(attacker.stat_stages[STAT_DEF], -1);
    EXPECT_EQ(attacker.stat_stages[STAT_SPEED], +1);
    EXPECT_EQ(attacker.stat_stages[STAT_SPATK], -2);
    EXPECT_EQ(attacker.stat_stages[STAT_SPDEF], +3);
}
