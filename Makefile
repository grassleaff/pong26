CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
LDFLAGS = -lncursesw

TARGET = pong26
SRC = src/main.cpp

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(BINDIR)..."
	sudo install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Done."

uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)..."
	sudo rm -f $(BINDIR)/$(TARGET)
	@echo "Done."

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all install uninstall clean run

