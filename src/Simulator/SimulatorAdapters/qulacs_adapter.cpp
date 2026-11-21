// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "qulacs_adapter.hpp"

#include <cppsim/state.hpp>
#include <csim/stat_ops.hpp>
#include <csim/update_ops.hpp>
#include <numbers>
#include <string>
#include <type_traits>

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
    data = state->data_c();
    dim = state->dim;

    LOG_INFO("Allocating in Qulacs [ " + std::to_string(num_qubits) +
             " ] qubits. Total registered qubits: " +
             std::to_string(registered_qubits));
    return true;
}

uint8_t QulacsAdapter::measure_z(uint8_t qubit_address) {
    const double p0 = M0_prob(qubit_address, data, dim);
    const bool one =
        std::uniform_real_distribution<double>(0.0, 1.0)(generator) >= p0;
    if (one) {
        P1_gate(qubit_address, data, dim);
        normalize(1.0 - p0, data, dim);
    } else {
        P0_gate(qubit_address, data, dim);
        normalize(p0, data, dim);
    }
    return one ? 1 : 0;
}

bool QulacsAdapter::qubit_initialize(uint8_t qubit_address) {
    if (measure_z(qubit_address) == 1) {
        X_gate(qubit_address, data, dim);
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

    if (axis == Axis_t::X_AXIS) {
        RX_gate(qubit_address, -theta, data, dim);
    } else if (axis == Axis_t::Y_AXIS) {
        RY_gate(qubit_address, -theta, data, dim);
    } else {
        RZ_gate(qubit_address, -theta, data, dim);
    }

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

    if (r < x_noise_probability) {
        X_gate(qubit_address, data, dim);
    } else if (r < x_noise_probability + y_noise_probability) {
        Y_gate(qubit_address, data, dim);
    } else if (r < x_noise_probability + y_noise_probability +
                       z_noise_probability) {
        Z_gate(qubit_address, data, dim);
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
    // Same axis rotation convention as in StimAdapterNVCenters:
    constexpr UINT pauli_z = 3;
    const UINT targets[2] = {qubit1_address, qubit2_address};
    const UINT paulis[2] = {pauli_z, static_cast<UINT>(axis) + 1};
    multi_qubit_Pauli_rotation_gate_partial_list(targets, paulis, 2, -theta,
                                                 data, dim);

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
        const UINT targets[2] = {qubit1_address, qubit2_address};
        const UINT paulis[2] = {k % 4, k / 4};
        multi_qubit_Pauli_gate_partial_list(targets, paulis, 2, data, dim);
    }

    LOG_INFO("Applied two-qubit depolarizing noise: q1=" +
             std::to_string(qubit1_address) +
             " q2=" + std::to_string(qubit2_address) +
             " p=" + std::to_string(depolarizing_probability));
    return true;
}

uint8_t QulacsAdapter::qubit_measure(uint8_t qubit_address, bool basis) {
    const uint8_t measurement = measure_z(qubit_address);

    LOG_INFO("Measured qubit " + std::to_string(qubit_address) + " in " +
             (basis ? "X" : "Z") +
             "-basis with result: " + std::to_string(measurement));
    return measurement;
}

int8_t QulacsAdapter::attempt_entanglement(uint16_t fidelity,
                                           uint8_t local_qubit,
                                           uint8_t neighbour_address,
                                           bool is_initiator, uint8_t qid) {
    if (local_qubit == neighbour_address) {
        LOG_ERROR("Cannot entangle qubit " + std::to_string(local_qubit) +
                  " with itself!");
        return -1;
    }
    if (!is_initiator) {
        LOG_INFO(
            "attempt_entanglement (responder): pair created by initiator; Bell "
            "state provided by manager");
        return 0;
    }

    qubit_initialize(local_qubit);
    qubit_initialize(neighbour_address);
    H_gate(local_qubit, data, dim);
    CNOT_gate(local_qubit, neighbour_address, data, dim);

    const int8_t bell_state = static_cast<int8_t>(
        std::uniform_int_distribution<int>(0, 3)(generator));
    switch (bell_state) {
        case 0:  // |phi+> = (|00> + |11>) / sqrt(2)
            break;
        case 1:  // |phi-> = (|00> - |11>) / sqrt(2)
            Z_gate(local_qubit, data, dim);
            break;
        case 2:  // |psi+> = (|01> + |10>) / sqrt(2)
            X_gate(neighbour_address, data, dim);
            break;
        case 3:  // |psi-> = (|01> - |10>) / sqrt(2)
            X_gate(neighbour_address, data, dim);
            Z_gate(local_qubit, data, dim);
            break;
    }

    static constexpr const char *bell_names[] = {"|phi+>", "|phi->", "|psi+>",
                                                 "|psi->"};
    LOG_INFO("attempt_entanglement (initiator): created " +
             std::string(bell_names[bell_state]) + " on Q" +
             std::to_string(local_qubit) + ",Q" +
             std::to_string(neighbour_address));
    return bell_state;
}

bool QulacsAdapter::qubit_move(uint8_t qubit1_address, uint8_t qubit2_address) {
    if (qubit1_address == qubit2_address) {
        LOG_ERROR("Cannot move qubit : " + std::to_string(qubit1_address) +
                  " to itself!");
        return false;
    }

    SWAP_gate(qubit1_address, qubit2_address, data, dim);
    qubit_initialize(qubit1_address);

    LOG_INFO("Moved qubit " + std::to_string(qubit1_address) + " to " +
             std::to_string(qubit2_address));
    return true;
}

bool QulacsAdapter::qubit_swap(uint8_t qubit1_address, uint8_t qubit2_address) {
    if (qubit1_address == qubit2_address) {
        LOG_ERROR("Cannot swap qubit : " + std::to_string(qubit1_address) +
                  " with itself!");
        return false;
    }

    SWAP_gate(qubit1_address, qubit2_address, data, dim);

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
