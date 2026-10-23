CXX = g++

ON = ON
ON_SRC = ON.cpp

.PHONY: all run-on run-vn clean

all: $(ON)

$(ON): $(ON_SRC)
	$(CXX) $(ON_SRC) -o $(ON)

# Usage: make run-on CONFIG=my_config.txt
run-on: $(ON)
	./$(ON) $(CONFIG)

# Usage: make run-vn ARGS="10.53.2.185 5000 10.53.2.186 5001"
run-vn:
	python3 VN.py $(ARGS)

clean:
	rm -f $(ON)