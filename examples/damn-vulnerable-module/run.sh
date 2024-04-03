#!/bin/bash

print_usage()
{
    echo "Usage: $0 [Options]..."
    echo "      -h      display this help and exit"
    echo "      -a      architecture of kernel (default: x86_64; x86_64 is now available)"
    echo "      -v      version of kernel (default: 6.6.21; 6.6.21 is now available)"
    echo "      -f      path to initramfs (default: $(dirname $0)/initramfs.cpio)"
    echo "      -e      path to exploit (default: \$PWD/exploit)"
}

resolve_kernel_image_path()
{
    karch=$1
    kversion=$2

    kernel_image_path=$(dirname $0)/../kernels/linux-$kversion-$karch/bzImage

    if [ ! -f $kernel_image_path ]; then
        echo "Error: $kernel_image_path not exists"
        exit 1
    fi

    echo "$kernel_image_path"
}

run_vm()
{
    kernel=$1
    initramfs=$2
    exploit=$3

    args=(
        -m 1G
        -smp cpus=1
        -cpu qemu64,+smep,+smap,+rdrand
        -kernel $kernel
        -initrd $initramfs
        -append "root=/dev/vda1 console=tty1 console=ttyS0 quiet loglevel=3"
        -monitor /dev/null
        -nographic
        -hda $exploit # host: /path/to/exploit/file, guest: /dev/sda
        -s
    )

    if [ -e "/dev/kvm" ]; then
        args+=("-enable-kvm")
    fi

    sudo qemu-system-x86_64 "${args[@]}" 2>&1
}

main()
{
    set -e
    
    options=$(getopt ha:v:f:e: "$@")
    set -- $options

    # default options
    karch="x86_64"
    kversion="6.6.21"
    initramfs="$(dirname $0)/initramfs.cpio"
    exploit="$PWD/exploit"

    # parse options
    while [ -n "$1" ]; do
        case "$1" in
            -h) print_usage
                exit 0;;
            -a) karch=$2
                shift;;
            -v) kversion=$2
                shift;;
            -f) initramfs=$2
                shift;;
            -e) exploit=$2
                shift;;
            --) shift
                break;;
            *)
                echo "Invalid option $1"
                print_usage
                exit 1;;
        esac
        shift
    done

    kernel_image=$(resolve_kernel_image_path $karch $kversion)

    run_vm $kernel_image $initramfs $exploit
}

main $@