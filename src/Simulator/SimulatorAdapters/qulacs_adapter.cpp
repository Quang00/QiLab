// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "qulacs_adapter.hpp"

#include <cppsim/gate_factory.hpp>
#include <cppsim/state.hpp>
#include <numbers>
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
    state = std::make_unique<QuantumStateCpu>(registered_qubits + num_qubits);
    registered_qubits += num_qubits;

    LOG_INFO("Allocating in Qulacs [ " + std::to_string(num_qubits) +
             " ] qubits. Total registered qubits: " +
             std::to_string(registered_qubits));
    return true;
}

bool QulacsAdapter::qubit_initialize(uint8_t qubit_address) {
    std::unique_ptr<QuantumGateBase> measure(
        gate::Measurement(qubit_address, 0, static_cast<UINT>(generator())));
    measure->update_quantum_state(state.get());
    if (state->get_classical_value(0) == 1) {
        std::unique_ptr<QuantumGateBase> flip(gate::X(qubit_address));
        flip->update_quantum_state(state.get());
    }

    LOG_INFO("Initialized qubit " + std::to_string(qubit_address) +
             " to |0> state.");
    return true;
}

bool QulacsAdapter::single_qubit_gate(uint8_t qubit_address, Angle_t angle,
                                      Axis_t axis) {
    if (axis != Axis_t::X_AXIS && axis != Axis_t::Y_AXIS &&
        axis != Axis_t::Z_AXIS) {
        LOG_WARNING("Invalid axis for single-qubit gate: " +
                    std::to_string(axis));
        return false;
    }

    const double theta =
        (angle < 16 ? angle : angle - 32) * std::numbers::pi / 16.0;

    std::unique_ptr<QuantumGateBase> rotation;
    if (axis == Axis_t::X_AXIS) {
        rotation.reset(gate::RotX(qubit_address, theta));
    } else if (axis == Axis_t::Y_AXIS) {
        rotation.reset(gate::RotY(qubit_address, theta));
    } else {
        rotation.reset(gate::RotZ(qubit_address, theta));
    }
    rotation->update_quantum_state(state.get());

    LOG_INFO(
        "Applying single-qubit gate: qubit=" + std::to_string(qubit_address) +
        " angle=" + std::to_string(angle) + " axis=" + std::to_string(axis));
    return true;
}

bool QulacsAdapter::single_qubit_noise(uint8_t qubit_address,
                                       float x_noise_probability,
                                       float y_noise_probability,
                                       float z_noise_probability) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double r = uniform(generator);

    std::unique_ptr<QuantumGateBase> pauli;
    if (r < x_noise_probability) {
        pauli.reset(gate::X(qubit_address));
    } else if (r < x_noise_probability + y_noise_probability) {
        pauli.reset(gate::Y(qubit_address));
    } else if (r < x_noise_probability + y_noise_probability +
                       z_noise_probability) {
        pauli.reset(gate::Z(qubit_address));
    }
    if (pauli) {
        pauli->update_quantum_state(state.get());
    }

    LOG_INFO(
        "Applied single-qubit noise: qubit=" + std::to_string(qubit_address) +
        " x_noise=" + std::to_string(x_noise_probability) +
        " y_noise=" + std::to_string(y_noise_probability) +
        " z_noise=" + std::to_string(z_noise_probability));
    return true;
}

bool QulacsAdapter::two_qubit_gate(uint8_t qubit1_address,
                                   uint8_t qubit2_address, Angle_t angle,
                                   Axis_t axis) {
    if (qubit1_address == qubit2_address) {
        LOG_ERROR("Cannot apply a two qubit gate to the same qubit 1:  " +
                  std::to_string(qubit1_address) +
                  ", 2: " + std::to_string(qubit2_address));
        return false;
    }
    if (axis != Axis_t::X_AXIS && axis != Axis_t::Y_AXIS &&
        axis != Axis_t::Z_AXIS) {
        LOG_WARNING("Invalid axis for two-qubit gate: " + std::to_string(axis));
        return false;
    }

    const double theta =
        (angle < 16 ? angle : angle - 32) * std::numbers::pi / 16.0;
    const UINT pauli = axis + 1;

    std::unique_ptr<QuantumGateBase> rotation(gate::PauliRotation(
        {qubit1_address, qubit2_address}, {pauli, pauli}, -theta));
    rotation->update_quantum_state(state.get());

    LOG_INFO(
        "Applying two-qubit gate: qubits=" + std::to_string(qubit1_address) +
        "," + std::to_string(qubit2_address) +
        " angle=" + std::to_string(angle) + " axis=" + std::to_string(axis));
    return true;
}

bool QulacsAdapter::two_qubit_depolarizing_noise(
    uint8_t qubit1_address, uint8_t qubit2_address,
    float depolarizing_probability) {
    if (qubit1_address == qubit2_address) {
        LOG_ERROR(
            "Cannot apply two-qubit depolarizing noise to the same qubit " +
            std::to_string(qubit1_address));
        return false;
    }

    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double r = uniform(generator);
    if (r < depolarizing_probability) {
        const UINT k = std::uniform_int_distribution<UINT>(1, 15)(generator);
        std::unique_ptr<QuantumGateBase> pauli(
            gate::Pauli({qubit1_address, qubit2_address}, {k % 4, k / 4}));
        pauli->update_quantum_state(state.get());
    }

    LOG_INFO("Applied two-qubit depolarizing noise: q1=" +
             std::to_string(qubit1_address) +
             " q2=" + std::to_string(qubit2_address) +
             " p=" + std::to_string(depolarizing_probability));
    return true;
}

uint8_t QulacsAdapter::qubit_measure(uint8_t qubit_address, bool basis) {
    std::unique_ptr<QuantumGateBase> measure(
        gate::Measurement(qubit_address, 0, static_cast<UINT>(generator())));
    measure->update_quantum_state(state.get());
    const uint8_t measurement = state->get_classical_value(0);

    LOG_INFO("Measured qubit " + std::to_string(qubit_address) + " in " +
             (basis ? "X" : "Z") +
             "-basis with result: " + std::to_string(measurement));
    return measurement;
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
    if (qubit1_address == qubit2_address) {
        LOG_ERROR("Cannot swap qubit : " + std::to_string(qubit1_address) +
                  " with itself!");
        return false;
    }

    std::unique_ptr<QuantumGateBase> swap(
        gate::SWAP(qubit1_address, qubit2_address));
    swap->update_quantum_state(state.get());

    LOG_INFO("Swapped qubits " + std::to_string(qubit1_address) + " and " +
             std::to_string(qubit2_address));
    return true;
}

bool QulacsAdapter::entanglement_swap(uint8_t qubit1_address,
                                      uint8_t qubit2_address) {
    LOG_ERROR(
        "Entanglement_swap is not implemented yet! The nodes should instead "
        "send the protocol jointly (set of gates) to perform the swap.");
    return false;
}
