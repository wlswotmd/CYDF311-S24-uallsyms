# uallsyms

## Quick Start
```shell
# Install dependencies
sudo apt install gcc make autoconf libtool qemu-system-x86

# Compile and install uallsyms library
git clone https://github.com/wlswotmd/CYDF311-S24-uallsyms.git
cd CYDF311-S24-uallsyms
./autogen.sh
./configure
make
sudo make install

# Pack initramfs (PWD: /path/to/CYDF311-S24-uallsyms)
cd ./examples/damn-vulnerable-module
./pack-initramfs.sh

# Compile the exploit binary and test it (PWD: /path/to/CYDF311-S24-uallsyms/examples/damn-vulnerable-module)
cd ./exploits/exploit-uallsyms
make 
../../run.sh -e ./exploit

# Run the exploit binary (inside QEMU)
./exploit

# Make sure you are root
id
# expected output: uid=0(root) gid=0(root)
```

## Dependencies
```bash
sudo apt install gcc make autoconf libtool qemu-system-x86
```

## Compiling

```bash
git clone https://github.com/wlswotmd/CYDF311-S24-uallsyms.git
cd CYDF311-S24-uallsyms
./autogen.sh
./configure
make
```