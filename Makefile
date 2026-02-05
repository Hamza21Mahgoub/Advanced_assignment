# ==============================================================================
# ELE430 Producer-Consumer Makefile
# ==============================================================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -g
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LOG_DIR = logs

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = producer_consumer

# ==============================================================================
# Build Rules
# ==============================================================================

# Default target
all: $(TARGET)

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	@echo "Creating object directory..."
	@mkdir -p $(OBJ_DIR)

# Create logs directory if it doesn't exist
$(LOG_DIR):
	@echo "Creating logs directory..."
	@mkdir -p $(LOG_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================================================================
# Utility Targets
# ==============================================================================

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete"

# Clean everything including logs
cleanall: clean
	@echo "Removing log files..."
	rm -rf $(LOG_DIR)
	@echo "All cleaned"

# Run with default parameters (quick test)
run: $(TARGET) | $(LOG_DIR)
	@echo "Running with default parameters..."
	./$(TARGET) 3 2 10 20

# Run with different test configurations
test1: $(TARGET) | $(LOG_DIR)
	@echo "Test 1: Balanced system (3 producers, 2 consumers, queue=10)"
	./$(TARGET) 3 2 10 30

test2: $(TARGET) | $(LOG_DIR)
	@echo "Test 2: Many producers (5 producers, 3 consumers, queue=10)"
	./$(TARGET) 5 3 10 30

test3: $(TARGET) | $(LOG_DIR)
	@echo "Test 3: Small queue (5 producers, 3 consumers, queue=5)"
	./$(TARGET) 5 3 5 30

test4: $(TARGET) | $(LOG_DIR)
	@echo "Test 4: Large queue (5 producers, 3 consumers, queue=20)"
	./$(TARGET) 5 3 20 30

# Generate required log files for submission
log1: $(TARGET) | $(LOG_DIR)
	@echo "Generating log file 1: 5 producers, 3 consumers, queue=10, 30 seconds"
	./$(TARGET) 5 3 10 30 > $(LOG_DIR)/run1.log 2>&1
	@echo "Log file created: $(LOG_DIR)/run1.log"

log2: $(TARGET) | $(LOG_DIR)
	@echo "Generating log file 2: 8 producers, 4 consumers, queue=15, 60 seconds"
	./$(TARGET) 8 4 15 60 > $(LOG_DIR)/run2.log 2>&1
	@echo "Log file created: $(LOG_DIR)/run2.log"

# Generate both required logs
logs: log1 log2
	@echo "Both log files generated successfully"

# Display help
help:
	@echo "ELE430 Producer-Consumer Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove object files and executable"
	@echo "  cleanall   - Remove object files, executable, and logs"
	@echo "  run        - Build and run with default parameters"
	@echo "  test1-4    - Run various test configurations"
	@echo "  log1       - Generate first required log file"
	@echo "  log2       - Generate second required log file"
	@echo "  logs       - Generate both required log files"
	@echo "  help       - Display this help message"

# ==============================================================================
# Dependencies
# ==============================================================================

# Ensure all object files depend on all headers (simple dependency)
$(OBJECTS): $(wildcard $(INC_DIR)/*.h)

.PHONY: all clean cleanall run test1 test2 test3 test4 log1 log2 logs help
