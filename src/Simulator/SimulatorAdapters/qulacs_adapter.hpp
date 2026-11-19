// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once
#include <cstdint>
#include <memory>
#include <random>

#include "Simulator/sim_interface.hpp"

class QuantumStateCpu;

/**
 * @brief Adapter implementing ISimulator on top of the Qulacs state-vector
 * simulator.
 *
 * Unlike Stim, which are limited to Clifford gates, a state vector
 * can apply any rotation angle (e.g., non-Clifford gates) and arbitrary
 * noise channels.
 */
class QulacsAdapter : public ISimulator {
public:
    QulacsAdapter(std::seed_seq &seed_seq, const uint8_t &num_qubits);
    ~QulacsAdapter() override;

    bool qubit_initialize(uint8_t qubit_address) override;
    bool single_qubit_gate(uint8_t qubit_address, Angle_t angle,
                           Axis_t axis) override;
    bool single_qubit_noise(uint8_t qubit_address, float x_noise_probability,
                            float y_noise_probability,
                            float z_noise_probability) override;
    bool two_qubit_gate(uint8_t qubit1_address, uint8_t qubit2_address,
                        Angle_t angle, Axis_t axis) override;
    bool two_qubit_depolarizing_noise(uint8_t qubit1_address,
                                      uint8_t qubit2_address,
                                      float depolarizing_probability) override;
    uint8_t qubit_measure(uint8_t qubit_address, bool basis) override;
    int8_t attempt_entanglement(uint16_t fidelity, uint8_t local_qubit,
                                uint8_t neighbour_address, bool is_initiator,
                                uint8_t qid) override;
    bool qubit_move(uint8_t qubit1_address, uint8_t qubit2_address) override;
    bool qubit_swap(uint8_t qubit1_address, uint8_t qubit2_address) override;
    bool entanglement_swap(uint8_t qubit1_address,
                           uint8_t qubit2_address) override;
    bool allocate_qubits(uint8_t num_qubits) override;

private:
    uint8_t registered_qubits = 0;
    std::mt19937_64 generator;
    std::unique_ptr<QuantumStateCpu> state;

    QulacsAdapter(const QulacsAdapter &) = delete;
    QulacsAdapter(QulacsAdapter &&) = delete;
    QulacsAdapter &operator=(const QulacsAdapter &) = delete;
    QulacsAdapter &operator=(QulacsAdapter &&) = delete;
};
