#!/bin/sh

OPENWRT_KERNEL="/home/prog/openwrt/2021-openwrt/last-openwrt/openwrt/build_dir/target-aarch64_cortex-a72_musl/linux-mvebu_cortexa72/linux-5.10.64"
DTC=${OPENWRT_KERNEL}/scripts/dtc/dtc

${DTC} -I dtb -O dts -o lhgg.dts lhgg.dtb
#${DTC} -I dtb -O dts -o rb5009.dts rb5009.dtb