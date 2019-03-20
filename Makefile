CC = c++

INCLUDES = `/homes/ahg13/wxWidgets-3.0.3/gtk-build/wx-config --version=3.0 --cxxflags` 

CCSW = -O3 -Wall -Wno-unused-variable

LIBS  =	libinterp.a libisosurf.a `/homes/ahg13/wxWidgets-3.0.3/gtk-build/wx-config --version=3.0 --libs --gl_libs` -lGL -lGLU 

vis:	vis.o libinterp.a
	$(CC) -o vis vis.o libinterp.a ${CCSW} ${LIBS} 

vis.o: vis.h

.cpp.o: 
	$(CC) ${CCSW} ${INCLUDES} -c $<


clean:
	echo cleaning up; /bin/rm -f core *.o vis
