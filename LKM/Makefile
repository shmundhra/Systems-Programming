obj-m+=lkm_bst.o

all:
    make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
    @echo ""
    @echo "sudo insmod <name_of_module>.ko"
    @echo ""
clean:
    make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
