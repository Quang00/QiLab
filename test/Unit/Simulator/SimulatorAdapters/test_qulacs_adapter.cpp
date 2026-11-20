// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "DataTypes/commands.hpp"
#include "Simulator/SimulatorAdapters/qulacs_adapter.hpp"

namespace {

std::unique_ptr<QulacsAdapter> make_adapter(uint8_t num_qubits) {
    std::seed_seq seed{1, 2, 3, 4, 5};
    return std::make_unique<QulacsAdapter>(seed, num_qubits);
}

}  // namespace

TEST(QulacsAdapterTest, SingleQubitGateX) {
    // X |0> = |1>
    auto adapter = make_adapter(2);
    ASSERT_TRUE(adapter->allocate_qubits(2));
    ASSERT_TRUE(adapter->qubit_initialize(3));
    ASSERT_TRUE(
        adapter->single_qubit_gate(3, Angle_t::_MINUS_PI, Axis_t::X_AXIS));
    EXPECT_EQ(adapter->qubit_measure(3, false), 1);
}

TEST(QulacsAdapterTest, SingleQubitGate4TGates) {
    // Rx(-π/2) T T T T Rx(π/2) |0> = |1>
    auto adapter = make_adapter(2);
    ASSERT_TRUE(adapter->qubit_initialize(0));
    ASSERT_TRUE(
        adapter->single_qubit_gate(0, Angle_t::_8_PI_16, Axis_t::X_AXIS));
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(
            adapter->single_qubit_gate(0, Angle_t::_4_PI_16, Axis_t::Z_AXIS));
    }
    ASSERT_TRUE(
        adapter->single_qubit_gate(0, Angle_t::_MINUS_8_PI_16, Axis_t::X_AXIS));
    EXPECT_EQ(adapter->qubit_measure(0, false), 1);
}

TEST(QulacsAdapterTest, TwoQubitGateZX) {
    // ZX |00> = |01>
    auto adapter = make_adapter(2);
    ASSERT_TRUE(adapter->qubit_initialize(0));
    ASSERT_TRUE(adapter->qubit_initialize(1));
    ASSERT_TRUE(
        adapter->two_qubit_gate(0, 1, Angle_t::_MINUS_PI, Axis_t::X_AXIS));
    EXPECT_EQ(adapter->qubit_measure(0, false), 0);
    EXPECT_EQ(adapter->qubit_measure(1, false), 1);
}

TEST(QulacsAdapterTest, AttemptEntanglementPsiPlus) {
    auto adapter = make_adapter(2);
    EXPECT_EQ(adapter->attempt_entanglement(1, 0, 1, true, 0), 2);
    EXPECT_NE(adapter->qubit_measure(0, false),
              adapter->qubit_measure(1, false));
}

TEST(QulacsAdapterTest, QubitSwapQubits) {
    auto adapter = make_adapter(2);
    ASSERT_TRUE(adapter->qubit_initialize(0));
    ASSERT_TRUE(adapter->qubit_initialize(1));
    ASSERT_TRUE(
        adapter->single_qubit_gate(0, Angle_t::_MINUS_PI, Axis_t::X_AXIS));
    ASSERT_TRUE(adapter->qubit_swap(0, 1));
    EXPECT_EQ(adapter->qubit_measure(0, false), 0);
    EXPECT_EQ(adapter->qubit_measure(1, false), 1);
}
