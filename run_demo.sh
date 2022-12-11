#!/usr/bin/env bash

trap handle_ctrl_c INT
running=true

function handle_ctrl_c() {
    running=false
}

if test -z "$BAZEL"; then
    BAZEL=bazel
fi

if test -z "$TARGET"; then
    TARGET=fpga_cw310_rom
fi

if test -z "$PAUSE"; then
    PAUSE=7
fi

run_test() {
    $BAZEL test \
        --test_output=streamed \
        --nocache_test_results \
        //sw/device/tests:${1}_${TARGET}
}

while $running; do
    for t in aes_smoketest otbn_ecdsa_op_irq_test; do
        run_test $t
        if ! $running; then
            break
        fi
        sleep $PAUSE
    done
done
