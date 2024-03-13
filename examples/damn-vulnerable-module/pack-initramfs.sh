#!/bin/bash

cd $(dirname $0)/initramfs
find . | cpio -H newc -ov -F ../initramfs.cpio