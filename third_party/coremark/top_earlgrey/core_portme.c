/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/
#include "coremark.h"
#include "third_party/coremark/top_earlgrey/core_portme.h"

#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/dif/dif_uart.h"
#include "sw/device/lib/runtime/print.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_test_config.h"
#include "sw/device/lib/testing/test_framework/status.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;
/* Porting : Timing functions
        How to capture time and convert to seconds must be ported to whatever is
   supported by the platform. e.g. Read value from on board RTC, read value from
   cpu clock cycles performance counter etc. Sample implementation for standard
   time.h and windows.h definitions included.
*/
CORETIMETYPE
barebones_clock()
{
  ee_u32 result;
  asm volatile("csrr %0, mcycle;" : "=r"(result));
  return result;
}
/* Define : TIMER_RES_DIVIDER
        Divider to trade off timer resolution and total time that can be
   measured.

        Use lower values to increase resolution, but make sure that overflow
   does not occur. If there are issues with the return value overflowing,
   increase this value.
        */
#define GETMYTIME(_t)              (*_t = barebones_clock())
#define MYTIMEDIFF(fin, ini)       ((fin) - (ini))
#define TIMER_RES_DIVIDER          1
#define SAMPLE_TIME_IMPLEMENTATION 1
#define CLOCKS_PER_SEC             10000000
#define EE_TICKS_PER_SEC           (CLOCKS_PER_SEC / TIMER_RES_DIVIDER)

/** Define Host specific (POSIX), or target specific global time variables. */
static CORETIMETYPE start_time_val, stop_time_val;
static ee_u32 start_num_instr_ret, stop_num_instr_ret,
              start_num_cycles_lsu, stop_num_cycles_lsu,
              start_num_cycles_if, stop_num_cycles_if,
              start_num_loads, stop_num_loads,
              start_num_stores, stop_num_stores,
              start_num_jumps, stop_num_jumps,
              start_num_branches, stop_num_branches,
              start_num_branches_taken, stop_num_branches_taken,
              start_num_instr_ret_c, stop_num_instr_ret_c,
              start_num_cycles_mul_wait, stop_num_cycles_mul_wait,
              start_num_cycles_div_wait, stop_num_cycles_div_wait;

/* Function : start_time
        This function will be called right before starting the timed portion of
   the benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or zeroing some system parameters - e.g. setting the cpu clocks
   cycles to 0.
*/
void
start_time(void)
{
    // Inhibit all performance counters before reading out their values prior to
    // benchmarking.
    CSR_WRITE(CSR_REG_MCOUNTINHIBIT, 0xffffffff);

    // Save start value of all performance counters.
    // TODO: extend to 64 bit
    CSR_READ(CSR_REG_MINSTRET, &start_num_instr_ret);
    CSR_READ(CSR_REG_MHPMCOUNTER3, &start_num_cycles_lsu);
    CSR_READ(CSR_REG_MHPMCOUNTER4, &start_num_cycles_if);
    CSR_READ(CSR_REG_MHPMCOUNTER5, &start_num_loads);
    CSR_READ(CSR_REG_MHPMCOUNTER6, &start_num_stores);
    CSR_READ(CSR_REG_MHPMCOUNTER7, &start_num_jumps);
    CSR_READ(CSR_REG_MHPMCOUNTER8, &start_num_branches);
    CSR_READ(CSR_REG_MHPMCOUNTER9, &start_num_branches_taken);
    CSR_READ(CSR_REG_MHPMCOUNTER10, &start_num_instr_ret_c);
    CSR_READ(CSR_REG_MHPMCOUNTER11, &start_num_cycles_mul_wait);
    CSR_READ(CSR_REG_MHPMCOUNTER12, &start_num_cycles_div_wait);
    GETMYTIME(&start_time_val);

    // Uninhibit all performance counters.
    CSR_WRITE(CSR_REG_MCOUNTINHIBIT, 0);
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the
   benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or other system parameters - e.g. reading the current value of
   cpu cycles counter.
*/
void
stop_time(void)
{
    // Inhibit all performance counters.
    CSR_WRITE(CSR_REG_MCOUNTINHIBIT, 0xffffffff);

    // Save stop value of all performance counters.
    // TODO: extend to 64 bit
    CSR_READ(CSR_REG_MINSTRET, &stop_num_instr_ret);
    CSR_READ(CSR_REG_MHPMCOUNTER3, &stop_num_cycles_lsu);
    CSR_READ(CSR_REG_MHPMCOUNTER4, &stop_num_cycles_if);
    CSR_READ(CSR_REG_MHPMCOUNTER5, &stop_num_loads);
    CSR_READ(CSR_REG_MHPMCOUNTER6, &stop_num_stores);
    CSR_READ(CSR_REG_MHPMCOUNTER7, &stop_num_jumps);
    CSR_READ(CSR_REG_MHPMCOUNTER8, &stop_num_branches);
    CSR_READ(CSR_REG_MHPMCOUNTER9, &stop_num_branches_taken);
    CSR_READ(CSR_REG_MHPMCOUNTER10, &stop_num_instr_ret_c);
    CSR_READ(CSR_REG_MHPMCOUNTER11, &stop_num_cycles_mul_wait);
    CSR_READ(CSR_REG_MHPMCOUNTER12, &stop_num_cycles_div_wait);
    GETMYTIME(&stop_time_val);

    CSR_WRITE(CSR_REG_MCOUNTINHIBIT, 0);
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.

        Actual value returned may be cpu cycles, milliseconds or any other
   value, as long as it can be converted to seconds by <time_in_secs>. This
   methodology is taken to accommodate any hardware or simulated platform. The
   sample implementation returns millisecs by default, and the resolution is
   controlled by <TIMER_RES_DIVIDER>
*/
CORE_TICKS
get_time(void)
{
    CORE_TICKS elapsed
        = (CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
    return elapsed;
}
/* Function : time_in_secs
        Convert the value returned by get_time to seconds.

        The <secs_ret> type is used to accommodate systems with no support for
   floating point. Default implementation implemented by the EE_TICKS_PER_SEC
   macro above.
*/
secs_ret
time_in_secs(CORE_TICKS ticks)
{
    secs_ret retval = ((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
    return retval;
}

ee_u32 default_num_contexts = 1;

OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = false,
                        .can_clobber_uart = true, );
dif_uart_t uart;

/* Function : portable_init
        Target specific initialization code
        Test for some common mistakes.
*/
void
portable_init(core_portable *p, int *argc, char *argv[])
{
    (void)argc; // prevent unused warning
    (void)argv; // prevent unused warning

    if (sizeof(ee_ptr_int) != sizeof(ee_u8 *))
    {
        ee_printf(
            "ERROR! Please define ee_ptr_int to a type that holds a "
            "pointer!\n");
    }
    if (sizeof(ee_u32) != 4)
    {
        ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
    }
    p->portable_id = 1;

    test_status_set(kTestStatusInTest);
    CHECK_DIF_OK(dif_uart_init(
                 mmio_region_from_addr(TOP_EARLGREY_UART0_BASE_ADDR), &uart));
    CHECK_DIF_OK(
                  dif_uart_configure(&uart, (dif_uart_config_t){
                                            .baudrate = kUartBaudrate,
                                            .clk_freq_hz = kClockFreqPeripheralHz,
                                            .parity_enable = kDifToggleDisabled,
                                            .parity = kDifUartParityEven,
                                            }));
    base_uart_stdout(&uart);

    // Print address of data that the core will operate on.
    // TODO: Additionally (and probably instead of printing), it would make
    // sense to check that this address is in the fastest available memory.
    ee_printf("addr of data: 0x%08p\n", p);

    // Print CPUCTRLSTS CSR to check that the instruction instruction cache is
    // enabled, data-independent timing is disabled, and the insertion of dummy
    // instructions is also disabled.
    // TODO: Instead of printing, we should check the respective bits and raise
    // an error if one of the assumptions above is violated.
    uint32_t cpuctrlsts;
    CSR_READ(CSR_REG_CPUCTRL, &cpuctrlsts);
    ee_printf("cpuctrlsts = 0x%03x\n", cpuctrlsts);
}

/* Function : portable_fini
        Target specific final code
*/
void
portable_fini(core_portable *p)
{
    ee_printf("\nPerformance counter delta to before start:\n");
    ee_printf("NumInstrRet = %d\n", stop_num_instr_ret - start_num_instr_ret);
    ee_printf("NumCyclesLSU = %d\n", stop_num_cycles_lsu - start_num_cycles_lsu);
    ee_printf("NumCyclesIF = %d\n", stop_num_cycles_if - start_num_cycles_if);
    ee_printf("NumLoads = %d\n", stop_num_loads - start_num_loads);
    ee_printf("NumStores = %d\n", stop_num_stores - start_num_stores);
    ee_printf("NumJumps = %d\n", stop_num_jumps - start_num_jumps);
    ee_printf("NumBranches = %d\n", stop_num_branches - start_num_branches);
    ee_printf("NumBranchesTaken = %d\n", stop_num_branches_taken - start_num_branches_taken);
    ee_printf("NumInstrRetC = %d\n", stop_num_instr_ret_c - start_num_instr_ret_c);
    ee_printf("NumCyclesMulWait = %d\n", stop_num_cycles_mul_wait - start_num_cycles_mul_wait);
    ee_printf("NumCyclesDivWait = %d\n", stop_num_cycles_div_wait - start_num_cycles_div_wait);
    ee_printf("\n");

    p->portable_id = 0;

    test_status_set(kTestStatusPassed);
}
