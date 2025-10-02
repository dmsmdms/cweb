#!/bin/bash

function install() {
    local TMP_DIR=tmp/$1
    local CONFIG=$1.config
    local CONFIG_TMP=$1.tmp.config

    cp config/$CONFIG config/$CONFIG_TMP
    echo "CONFIG_BUILD_RELEASE=y" >> config/$CONFIG_TMP
    make TMP_DIR=$TMP_DIR $CONFIG_TMP
    make TMP_DIR=$TMP_DIR
    rm config/$CONFIG_TMP
}

install bot-admin
install crypto-parser
