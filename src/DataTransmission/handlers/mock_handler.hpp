// Copyright (c) 2026, Quantum Internet Alliance, Delft University of
// Technology, Milen Kolev, Samuel Oslovich, Jake Smith, Stephanie Wehner
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*
** Created on: 25.08.2025
** Created by: mrkolev
** Description: Contains the implementation for a mock command handler.
**              The idea is fixed commands can be send in an uniform program
*/

#pragma once
#include <cstddef>
#include <cstdint>

#include "DataTransmission/transmision_handler.hpp"
#include "command_helper.hpp"

class MockHandler final : public ITransmissionHandler {
public:
    explicit MockHandler(int num_qubits = 1);
    ~MockHandler();
    int8_t receive_data(uint8_t *, uint8_t) override;
    int8_t send_data(const uint8_t *, uint8_t) override;
    void flush() override {};
    int8_t transfer(uint8_t const *tx_buffer, uint8_t const *rx_buffer,
                    const uint8_t length) override;

    static constexpr std::size_t random_clifford_depth = 10;
    static constexpr uint32_t random_clifford_seed = 1;

private:
    void fill_mock_message(uint8_t const *);
    void create_commands();
    void create_random_clifford_commands(int num_qubits, std::size_t depth,
                                         uint32_t seed);
    int latest_measure_response = -1;
    std::vector<BaseC *> command_list;

    int message_num = 0;

    MockHandler(const MockHandler &) = delete;
    MockHandler(const MockHandler &&) = delete;
    MockHandler &operator=(MockHandler &) = delete;
    MockHandler &operator=(MockHandler &&) = delete;
};
