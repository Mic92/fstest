# $FreeBSD: src/tools/regression/fstest/Makefile,v 1.1 2007/01/17 01:42:07 pjd Exp $

#CFLAGS+=-DHAS_LCHMOD
#CFLAGS+=-DHAS_CHFLAGS
#CFLAGS+=-DHAS_LCHFLAGS
#CFLAGS+=-DHAS_TRUNCATE64
#CFLAGS+=-DHAS_STAT64

DISK = sgxlkl-disk.img
ESCALATE_CMD = sudo
IMAGE_SIZE_MB = 100
PROG = fstest-server
FINAL_CFLAGS = -g -fPIE -pie -Wall -pthread $(CFLAGS)

all: $(PROG)

SRC = $(wildcard *.c)

$(PROG):	$(SRC)
	../../build/host-musl/bin/musl-gcc $(FINAL_CFLAGS) $(SRC) -o $@

check: $(DISK)
	if [[ -n "${SGXLKL_ENABLE_DEBUGGER}" ]]; then \
		python3 sgx-lkl-fstests sgx-lkl-gdb --args ../../build/sgx-lkl-run $(DISK) /bin/$(PROG); \
	else \
		python3 sgx-lkl-fstests ../../build/sgx-lkl-run $(DISK) /bin/$(PROG); \
	fi

clean:
	rm -f $(PROG) $(DISK)

$(DISK): $(PROG)
	dd if=/dev/zero of="$@" count=$(IMAGE_SIZE_MB) bs=1M
	mkfs.ext4 "$@"
	$(ESCALATE_CMD) bash -c '\
		set -euxo pipefail; \
		mnt=`mktemp -d`; \
		trap "rm -rf $$mnt" EXIT; \
		mount -t ext4 -o loop "$@" $$mnt; \
		install -D $(PROG) $$mnt/bin/$(PROG); \
		mkdir -p $$mnt/dev; \
		umount $$mnt; \
	'
