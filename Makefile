CXXFLAGS += --std=c++11
COORD_LOC = ${HOME}/sandbox/coordinates
CPPXML_LOC = ${HOME}/sandbox/cppxml
DATA_LOC = ${HOME}/sandbox/data
TIME_LOC = ${HOME}/sandbox/time
CXXFLAGS += -I${COORD_LOC} -I${CPPXML_LOC} -I${DATA_LOC} -I${TIME_LOC}
LDFLAGS += -lboost_system -lboost_program_options -lboost_regex -lexpat
OBJS = gpx2kml.o
EXE = gpx2kml

.cpp.o:
	g++ -c -g ${CXXFLAGS} $<

all: ${OBJS}
	g++ ${OBJS} ${LDFLAGS} -o ${EXE}

clean:
	rm -f ${OBJS} ${EXE} core *~
