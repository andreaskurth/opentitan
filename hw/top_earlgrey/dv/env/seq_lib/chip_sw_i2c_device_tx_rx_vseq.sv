// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

class chip_sw_i2c_device_tx_rx_vseq extends chip_sw_base_vseq;
  `uvm_object_utils(chip_sw_i2c_device_tx_rx_vseq)

  `uvm_object_new

  rand int i2c_idx;
  constraint i2c_idx_c {
    i2c_idx inside {[0:NUM_I2CS-1]};
  }

  rand int byte_count;
  constraint i2c_byte_count_c {
    byte_count inside {[30:60]};
  }

  rand bit [6:0] i2c_device_address_0;
  rand bit [6:0] i2c_device_address_1;

  int clock_period_nanos = 41;
  int i2c_clock_period_nanos = 1000;
  int rise_fall_nanos = 10;
  int rise_cycles = ((rise_fall_nanos - 1) / clock_period_nanos) + 1;
  int fall_cycles = ((rise_fall_nanos - 1) / clock_period_nanos) + 1;
  int clock_period_cycles = ((i2c_clock_period_nanos - 1) / clock_period_nanos) + 1;
  int half_period_cycles = ((i2c_clock_period_nanos/2 - 1) / clock_period_nanos) + 1;

  virtual task cpu_init();
    bit[7:0] clock_period_nanos_arr[1] = {clock_period_nanos};
    bit[7:0] rise_fall_nanos_arr[1] = {rise_fall_nanos};
    bit[7:0] i2c_clock_period_nanos_arr[4] = {<<byte{i2c_clock_period_nanos}};
    bit[7:0] i2c_idx_arr[1];
    bit[7:0] i2c_device_address_0_arr[1];
    bit[7:0] i2c_device_address_1_arr[1];
    bit[7:0] byte_count_arr[1];

    void'($value$plusargs("i2c_idx=%0d", i2c_idx));
    `DV_CHECK(i2c_idx inside {[0:NUM_I2CS-1]})

    i2c_idx_arr = {i2c_idx};
    i2c_device_address_0_arr = {i2c_device_address_0};
    i2c_device_address_1_arr = {i2c_device_address_1};
    byte_count_arr = {byte_count};
    super.cpu_init();
    // need to figure out a better way to calculate this based on tb clock frequency
    sw_symbol_backdoor_overwrite("kClockPeriodNanos", clock_period_nanos_arr);
    sw_symbol_backdoor_overwrite("kI2cRiseFallNanos", rise_fall_nanos_arr);
    sw_symbol_backdoor_overwrite("kI2cClockPeriodNanos", i2c_clock_period_nanos_arr);
    // sw_symbol_backdoor_overwrite("kI2cIdx", i2c_idx_arr);
    sw_symbol_backdoor_overwrite("kI2cDeviceAddress0", i2c_device_address_0_arr);
    // sw_symbol_backdoor_overwrite("kI2cDeviceMask0", i2c_device_mask_0);
    sw_symbol_backdoor_overwrite("kI2cDeviceAddress1", i2c_device_address_0_arr);
    // sw_symbol_backdoor_overwrite("kI2cDeviceMask1", i2c_device_mask_1);
    sw_symbol_backdoor_overwrite("kI2cByteCount", byte_count_arr);
    // sw_symbol_backdoor_overwrite("expected_data", i2c_device_tx_data);
  endtask

  virtual task body();
    bit [7:0] i2c_device_tx_data[$];
    super.body();

    // enable the monitor
    cfg.m_i2c_agent_cfgs[i2c_idx].en_monitor = 1'b1;
    cfg.m_i2c_agent_cfgs[i2c_idx].if_mode = Host;

    // Enbale appropriate interface
    cfg.chip_vif.enable_i2c(.inst_num(i2c_idx), .enable(1));


    // tClockLow needs to be "slightly" shorter than the actual clock low period
    cfg.m_i2c_agent_cfgs[i2c_idx].timing_cfg.tClockLow = half_period_cycles - 2;

    // tClockPulse needs to be "slightly" longer than the clock period.
    cfg.m_i2c_agent_cfgs[i2c_idx].timing_cfg.tClockPulse = half_period_cycles + 1;

    // Wait for i2c_device to fill tx_fifo
    cfg.clk_rst_vif.wait_clks(100);
    `DV_WAIT(cfg.sw_logger_vif.printed_log == "Data written to fifo");

    send_i2c_host_rx_data(i2c_device_tx_data, byte_count);

  endtask

  virtual task send_i2c_host_rx_data(ref bit [7:0] device_data[$],input int size);
    i2c_target_base_seq m_i2c_host_seq;
    `uvm_create_obj(i2c_target_base_seq, m_i2c_host_seq)
    //`uvm_create_on(m_i2c_host_seq, p_sequencer.i2c_host_sequencer_h)

    `uvm_send(m_i2c_host_seq)
    // device_data = m_i2c_host_seq.rsp.data;
  endtask


endclass : chip_sw_i2c_device_tx_rx_vseq
