obj-m += indigo.o 
indigo-objs += indigo_utils.o \
               tcp_tron.o \
               math.sse.o \
               print_float.sse.o \
               nn_inference.sse.o \
               nn_inference.o \
               training_output.o \
               nn/libgraph_hack.o
               
ccflags-y := -g -O0
CFLAGS_math.sse.o := -mhard-float -msse
CFLAGS_print_float.sse.o := -mhard-float -msse
CFLAGS_nn_inference.sse.o := -mhard-float -msse

all: indigo_nn.generated.h
	# KBuild in Linux 5.0 seems to no longer accept .a files.
	# They must be provided as special '_shipped' binary blob.
	cp nn/libgraph.pic.a nn/libgraph_hack.o_shipped
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
indigo_nn.generated.h: nn/graph.h nn/libgraph.pic.a
	cp nn/* tf_converter/input
	cd tf_converter && make -B
	./tf_converter/main indigo_nn > indigo_nn.generated.h
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
 
