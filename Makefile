
CXX = g++
LD = g++
WARNINGFLAGS = -Wall -Wextra
CXXFLAGS = -O3 -std=gnu++11 $(WARNINGFLAGS)
LDFLAGS = -lpthread

OBJFILES = client.o Command.o grep.o server.o utils.o
EXE = grep

$(EXE): $(OBJFILES)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	@rm $(OBJFILES) $(EXE)

