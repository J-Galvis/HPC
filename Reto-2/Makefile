CC = gcc
CFLAGS = -O2
SRC = src
OUT = output

all: $(OUT)/secuential $(OUT)/parallel

$(OUT):
	mkdir -p $(OUT)

$(OUT)/secuential: $(SRC)/SecuentialCellularAutomaton.c | $(OUT)
	$(CC) -o $@ $< -lm

$(OUT)/parallel: $(SRC)/ParallelCellularAutomaton.c | $(OUT)
	$(CC) -o $@ $< -lm -fopenmp


clean:
	rm -rf $(OUT)
