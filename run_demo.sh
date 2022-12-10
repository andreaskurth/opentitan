#!/usr/bin/env bash

if test -z "$BAZEL"; then
    BAZEL=bazel
fi

if test -z "$TARGET"; then
    TARGET=fpga_cw310_rom
fi

run_test() {
    $BAZEL test \
        --test_output=streamed \
        --nocache_test_results \
        //sw/device/tests:${1}_${TARGET}
}

while true; do
    run_test aes_smoketest
    run_test otbn_smoketest
 done
