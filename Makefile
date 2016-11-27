HEADERS = 

OBJECTS = \
	md.o

CXXFLAGS = -O3 -o prog -g3 -std=c++11 -rdynamic -D_GNU_SOURCE \
	-Wall -Wno-write-strings -Wno-unused-result \
	-L/usr/local/lib -I/usr/local/include -I/usr/include/libxml2 \
        -L/lib -L/usr/lib 

LIBS = 	-lopencv_imgproc  -lplplotcxxd \
	-lpthread -lcurl \
	-lstdc++ -lxml2 \
	-lopencv_core -lopencv_contrib -lopencv_legacy \
	-lopencv_ml -lopencv_video -lopencv_highgui \
        -lopencv_calib3d -lopencv_flann -lopencv_objdetect -lopencv_features2d \
	-ludev -lm -llog4cxx

# -lopencv_imgcodecs

default: md 

%.o: %.cpp $(HEADERS)
	$(CXX)  $(CXXFLAGS) -c $< -o $@

md: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LIBS) $(CXXFLAGS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f md 


