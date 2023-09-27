// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/ip/pwrmgr/dif/dif_pwrmgr.h"
#include "sw/ip/pwrmgr/test/utils/pwrmgr_testutils.h"
#include "sw/ip/rstmgr/dif/dif_rstmgr.h"
#include "sw/ip/rstmgr/test/utils/rstmgr_testutils.h"
#include "sw/lib/sw/device/base/mmio.h"
#include "sw/lib/sw/device/runtime/log.h"

#include "hw/top_darjeeling/sw/autogen/top_darjeeling.h"

OTTF_DEFINE_TEST_CONFIG();

// When the test first runs the rstmgr's `reset_info` CSR should have the POR
// bit set, the code clears reset_info and puts the chip in deep sleep. The WFI
// causes core_sleeping to rise, and that causes the SV side to glitch the main
// power rail, causing a pwrmgr internally generated reset. The next time the
// test runs is after the power glitch reset, which is confirmed reading the
// `reset_info` CSR.
bool test_main(void) {
  dif_pwrmgr_t pwrmgr;
  dif_rstmgr_t rstmgr;

  // Initialize pwrmgr since this will put the chip in deep sleep.
  CHECK_DIF_OK(dif_pwrmgr_init(
      mmio_region_from_addr(TOP_DARJEELING_PWRMGR_AON_BASE_ADDR), &pwrmgr));

  // Initialize rstmgr since this will check some registers.
  CHECK_DIF_OK(dif_rstmgr_init(
      mmio_region_from_addr(TOP_DARJEELING_RSTMGR_AON_BASE_ADDR), &rstmgr));

  // Notice we are clearing rstmgr's RESET_INFO, so after the power glitch there
  // is only one bit set.

  if (UNWRAP(rstmgr_testutils_is_reset_info(&rstmgr, kDifRstmgrResetInfoPor))) {
    LOG_INFO("Powered up for the first time, begin test");

    CHECK(UNWRAP(pwrmgr_testutils_is_wakeup_reason(&pwrmgr, 0)) == true);

    CHECK_STATUS_OK(rstmgr_testutils_pre_reset(&rstmgr));

    // Configure deep sleep.
    CHECK_STATUS_OK(pwrmgr_testutils_enable_low_power(
        &pwrmgr, kDifPwrmgrWakeupRequestSourceThree, 0));

    // This causes core_sleeping to rise and triggers the injection of the
    // power glitch. Enter low power mode.
    LOG_INFO("Issue WFI to enter sleep");
    wait_for_interrupt();
  } else {
    LOG_INFO("Checking reset status.");
    CHECK_STATUS_OK(rstmgr_testutils_post_reset(
        &rstmgr,
        kDifRstmgrResetInfoPowerUnstable | kDifRstmgrResetInfoLowPowerExit, 0,
        0, 0, 0));
    LOG_INFO("Reset status indicates a power glitch and a deep sleep wakeup");
  }
  return true;
}
