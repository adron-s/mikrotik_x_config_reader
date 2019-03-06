#!/bin/sh

OPENWRT_KERNEL="/home/prog/openwrt/lede-all/2019-openwrt-all/openwrt-ipq4xxx/build_dir/target-arm_cortex-a7+neon-vfpv4_musl_eabi/linux-ipq40xx/linux-4.19.23"
DTC=${OPENWRT_KERNEL}/scripts/dtc/dtc

${DTC} -I dtb -O dts -o lhgg.dts lhgg.dtb