CC = gcc
CFLAGS = -O2
SRC = src
OUT = output

# macOS (Apple clang) requires libomp from Homebrew for OpenMP support
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
	OMP_FLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include -L$(LIBOMP_PREFIX)/lib -lomp
else
	OMP_FLAGS = -fopenmp
endif

all: $(OUT)/secuential $(OUT)/parallel $(OUT)/memory

$(OUT):
	mkdir -p $(OUT)

$(OUT)/secuential: $(SRC)/SecuentialCellularAutomaton.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $< -lm

$(OUT)/parallel: $(SRC)/ParallelCellularAutomaton.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $< -lm $(OMP_FLAGS)

$(OUT)/memory: $(SRC)/MemoryCellularAutomaton.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $< -lm $(OMP_FLAGS)


clean:
	rm -rf $(OUT)
