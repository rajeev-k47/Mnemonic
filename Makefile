CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Iinclude

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = .

TARGET = $(BIN_DIR)/mnemonic

SOURCES = $(SRC_DIR)/main.cpp \
					$(SRC_DIR)/cli/command_parser.cpp \
          $(SRC_DIR)/allocator/memory_manager.cpp \
          $(SRC_DIR)/allocator/first_fit.cpp \
          $(SRC_DIR)/allocator/best_fit.cpp \
          $(SRC_DIR)/allocator/worst_fit.cpp \
					$(SRC_DIR)/cli/handlers.cpp


OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(OBJ_DIR): 
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJ_DIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
