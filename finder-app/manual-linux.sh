#!/bin/bash
set -eu  

OUTDIR=${1:-/tmp/aeld}  # Using parameter expansion to simplify directory selection
KERNEL_REPO="git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git"
KERNEL_VERSION="v5.15.163"
BUSYBOX_VERSION="1_33_1"
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH="arm64"
CROSS_COMPILE="aarch64-none-linux-gnu-"

mkdir -p "$OUTDIR"
echo "Using output directory: $OUTDIR"

cd "$OUTDIR"
if [[ ! -d "linux-stable" ]]; then
    echo "Cloning Linux stable version $KERNEL_VERSION into $OUTDIR"
    git clone --depth=1 --branch "$KERNEL_VERSION" "$KERNEL_REPO" linux-stable
fi

if [[ ! -f "linux-stable/arch/$ARCH/boot/Image" ]]; then
    cd linux-stable
    echo "Checking out kernel version: $KERNEL_VERSION"
    git checkout "$KERNEL_VERSION"
    
    [[ ! -f .config ]] && make ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" defconfig
    make -j$(nproc) ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" all modules dtbs
fi

cd "$OUTDIR"
[[ -d "rootfs" ]] && sudo rm -rf rootfs && echo "Removed existing rootfs directory"

mkdir -p rootfs/{bin,etc,proc,sys,dev,home,lib,lib64,sbin,tmp,usr/bin,usr/lib,usr/sbin,var/log}

#clone and build busybox
if [[ ! -d "busybox" ]]; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout "$BUSYBOX_VERSION"
    make distclean
    make defconfig
else
    cd busybox
fi

make ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE"
make CONFIG_PREFIX="$OUTDIR/rootfs" ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" install

cd "$OUTDIR/rootfs"
echo "Library dependencies:"
"${CROSS_COMPILE}readelf" -a bin/busybox | grep -E "program interpreter|Shared library"

SYSROOT="$(${CROSS_COMPILE}gcc -print-sysroot)"
sudo cp -L ${SYSROOT}/lib/ld-linux-aarch64.* ${OUTDIR}/rootfs/lib
sudo cp -L ${SYSROOT}/lib64/{libm.so.*,libresolv.so.*,libc.so.*} ${OUTDIR}/rootfs/lib64

#create device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

#compile the finder app
cd "$FINDER_APP_DIR"
make CROSS_COMPILE="$CROSS_COMPILE"

# Copy necessary libraries from the sysroot
cp writer "${OUTDIR}/rootfs/home/"
mkdir -p "${OUTDIR}/rootfs/home/conf"
cp "${FINDER_APP_DIR}/finder.sh" "${OUTDIR}/rootfs/home/"
cp "${FINDER_APP_DIR}/conf/username.txt" "${OUTDIR}/rootfs/home/conf"
cp "${FINDER_APP_DIR}/conf/assignment.txt" "${OUTDIR}/rootfs/home/conf"
cp "${FINDER_APP_DIR}/finder-test.sh" "${OUTDIR}/rootfs/home/"
cp "${FINDER_APP_DIR}/autorun-qemu.sh" "${OUTDIR}/rootfs/home/"

#set the ownership of the root filesystem
cd "$OUTDIR/rootfs"
sudo chown -R root:root .

# create the initramfs
find . | cpio --create --format=newc --owner=root:root > "$OUTDIR/initramfs.cpio"
gzip -f "$OUTDIR/initramfs.cpio"

#copy the kernel image to the output directory
cp "$OUTDIR/linux-stable/arch/$ARCH/boot/Image" "$OUTDIR/"
