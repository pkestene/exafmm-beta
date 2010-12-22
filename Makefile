.SUFFIXES: .cxx .o

CXX     = g++ -I../include -I/usr/include/vtk-5.2 -L/usr/lib/vtk-5.2
OFLAGS  = -mpreferred-stack-boundary=4 -ggdb3 -Wall -Wextra -Winit-self -Wshadow -O2 -fPIC -fopenmp\
	-ffast-math -funroll-loops -fforce-addr -rdynamic -D_FILE_OFFSET_BITS=64
LFLAGS  = -ldl -lm -lvtkHybridTCL -lvtkWidgetsTCL

.cxx.o  :
	$(CXX) -c -o $@ $< $(OFLAGS)
cleanall:
	rm -rf *.o *.out
save    :
	make cleanall
	tar zcvf ../../exafmm.tgz ../../exafmm
