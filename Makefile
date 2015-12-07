obj-m += top30.o
CFLAGS_top30.o += -DNDEBUG -O2

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
