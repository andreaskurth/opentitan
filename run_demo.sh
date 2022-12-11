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
    clear
    if which figlet >/dev/null; then
        figlet -w 100 -f big "$2"
    fi
    $BAZEL test \
        --test_output=streamed \
        --nocache_test_results \
        //sw/device/tests:${1}_${TARGET}
}

while $running; do
    for t in \
            aes_smoketest,AES-256 \
            otbn_ecdsa_op_irq_test,"OTBN   ECDSA p256" \
            ; do
        OLDIFS=$IFS
        IFS=','
        set -- $t
        IFS=$OLDIFS
        run_test $1 "$2"
        if ! $running; then
            break
        fi
        sleep $PAUSE
    done
done
