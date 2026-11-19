// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "qulacs_adapter.hpp"

#include <cppsim/state.hpp>
#include <string>

#include "logger/logger_api.hpp"

QulacsAdapter::QulacsAdapter(std::seed_seq &seed_seq, const uint8_t &num_qubits)
    : generator(seed_seq) {
    if (num_qubits > 0) {
        allocate_qubits(num_qubits);
    }
}

QulacsAdapter::~QulacsAdapter() = default;

bool QulacsAdapter::allocate_qubits(uint8_t num_qubits) {
    if (num_qubits == 0) {
        return true;
    }
    state = std::make_unique<QuantumStateCpu>(registered_qubits + num_qubits);
    registered_qubits += num_qubits;

    LOG_INFO("Allocating in Qulacs [ " + std::to_string(num_qubits) +
             " ] qubits. Total registered qubits: " +
             std::to_string(registered_qubits));
    return true;
}

bool QulacsAdapter::qubit_initialize(uint8_t qubit_address) { return false; }

bool QulacsAdapter::single_qubit_gate(uint8_t qubit_address, Angle_t angle,
                                      Axis_t axis) {
    return false;
}

bool QulacsAdapter::single_qubit_noise(uint8_t qubit_address,
                                       float x_noise_probability,
                                       float y_noise_probability,
                                       float z_noise_probability) {
    return false;
}

bool QulacsAdapter::two_qubit_gate(uint8_t qubit1_address,
                                   uint8_t qubit2_address, Angle_t angle,
                                   Axis_t axis) {
    return false;
}

bool QulacsAdapter::two_qubit_depolarizing_noise(
    uint8_t qubit1_address, uint8_t qubit2_address,
    float depolarizing_probability) {
    return false;
}

uint8_t QulacsAdapter::qubit_measure(uint8_t qubit_address, bool basis) {
    return 0;
}

int8_t QulacsAdapter::attempt_entanglement(uint16_t fidelity,
                                           uint8_t local_qubit,
                                           uint8_t neighbour_address,
                                           bool is_initiator, uint8_t qid) {
    return -1;
}

bool QulacsAdapter::qubit_move(uint8_t qubit1_address, uint8_t qubit2_address) {
    return false;
}

bool QulacsAdapter::qubit_swap(uint8_t qubit1_address, uint8_t qubit2_address) {
    return false;
}

bool QulacsAdapter::entanglement_swap(uint8_t qubit1_address,
                                      uint8_t qubit2_address) {
    return false;
}
