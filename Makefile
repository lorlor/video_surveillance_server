LDLIBS:= -lSDL2 -lx264 -lm -lpthread -ljrtp -ldl
OBJS:= main.o capture.o encoder.o sender.o
main: $(OBJS) 
	@clang++ -o main $(OBJS) $(LDLIBS) 

main.o: main.cpp
	@clang++ -c main.cpp

capture.o: capture.cpp capture.hpp
	@clang++ -c capture.cpp capture.hpp 

encoder.o: encoder.hpp encoder.cpp
	@clang++ -c encoder.cpp encoder.hpp -I/usr/local/include

sender.o: sender.hpp sender.cpp
	@clang++ -c sender.cpp sender.hpp

.PHONY: clean

clean:
	@echo "Clean middleware ..."
	@rm *.o *.hpp.gch
	@echo "Done"
