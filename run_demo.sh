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
    echo -e "\\033[35m$3\\033[m"
    $BAZEL test \
        --color=yes \
        --test_output=streamed \
        --nocache_test_results \
        //sw/device/tests:${1}_${TARGET} \
        2> \
    >(grep -v 'WARNING:' >&2)
}

while $running; do
    for t in \
            aes_smoketest,AES-256,"Use the AES hardware accelerator to encrypt and decrypt data" \
            otbn_ecdsa_op_irq_test,"OTBN   ECDSA p256","Use the OpenTitan Big Number Accelerator to sign and verify data" \
            otbn_rsa_test,"OTBN   RSA 512","Use the OpenTitan Big Number Accelerator to asymmetrically encrypt and decrypt data" \
            ; do
        OLDIFS=$IFS
        IFS=','
        set -- $t
        IFS=$OLDIFS
        run_test $1 "$2" "$3"
        if ! $running; then
            break
        fi
        sleep $PAUSE
    done
done
