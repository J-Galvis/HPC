# HPC - Multiplicación de Matrices

Proyecto en C para comparar varias implementaciones de multiplicación de matrices cuadradas (enteros aleatorios 0–99). Cada binario mide y escribe por stdout el tiempo de CPU de usuario (usando `getrusage`) en segundos con 6 decimales.

## Estructura

```
src/
├── SecuentialMatrixSolver.c        # versión secuencial (sin flags)
├── MemoryMatrixSolver.c            # mejora: B transpuesta para mejor localidad
├── ThreadsMatrixSolver.c           # divide filas entre threads (pthreads)
└── MultiprocessingMatrixSolver.c   # divide trabajo con fork() + mmap compartida
output/                             # binarios de salida (creado por Makefile)
scripts/
└── RunAll.sh                        # scripts de benchmark/automatización
```

## Compilación

Se incluye un `Makefile` para compilar todo rápidamente:

```bash
make        # compila todos los binarios en output/
make clean  # elimina output/
```

Si prefieres compilar manualmente (ejemplos):

```bash
# Secuencial (sin optimizaciones)
gcc src/SecuentialMatrixSolver.c -o output/secuential

# Secuencial con optimizaciones (medida separada en el repo)
gcc src/SecuentialMatrixSolver.c -O2 -o output/secuentialFlags

# Memory-optimized
gcc src/MemoryMatrixSolver.c -O3 -o output/memory

# Threads (pthreads)
gcc src/ThreadsMatrixSolver.c -o output/threads -pthread

# Multiprocessing
gcc src/MultiprocessingMatrixSolver.c -o output/multiprocessing
```

## Ejecutar

Formato general (según el binario):

```
./output/<variante> <filas> <columnas|num_workers>
```

Ejemplos:

```
./output/secuential 500 500         # filas=500 cols=500
./output/memory 500 500             # usa la versión optimizada en memoria
./output/threads 1000 4             # filas=1000, 4 threads
./output/multiprocessing 1000 8     # filas=1000, 8 procesos
```

Cada programa imprime únicamente el tiempo de CPU de usuario (float con 6 decimales) en stdout.

## Script de benchmarks

`scripts/RunAll.sh` ejecuta la batería de pruebas y guarda CSV de resultados en `stats/<hostname>/<timestamp>/`. Soporta reanudar ejecuciones interrumpidas mediante `checkpoint.log`.

También existe `scripts/testing.sh` para ejecuciones más cortas y rápidas (legado).

Uso rápido:

1) Compilar con `make`.
2) Dar permisos al script (si hace falta): `chmod +x scripts/RunAll.sh`.
3) Ejecutarlo: `./scripts/RunAll.sh`.

## Arquitectura y objetivos

Objetivo: comparar impacto de distintas estrategias en el rendimiento de CPU:

- `secuential`: implementación naïve O(n^3), compilada sin flags.
- `secuentialFlags`: misma fuente que `secuential` pero compilada con `-O2` para medir efecto del compilador.
- `memory`: transpone B para mejor localidad de caché.
- `threads`: usa POSIX threads y divide filas entre workers.
- `multiprocessing`: fork() + `mmap(MAP_SHARED|MAP_ANONYMOUS)` para compartir la matriz resultado; mide tiempo de procesos hijos y padre (`RUSAGE_SELF + RUSAGE_CHILDREN`).

## Salida de resultados

Los resultados del script se organizan en `stats/<hostname>/<timestamp>/` con CSV por variante. Al finalizar una ejecución el directorio de la corrida se marca con el sufijo `.done`.

## Notas

- Las implementaciones usan `getrusage(RUSAGE_SELF, ...)` y extraen `ru_utime`.
- En la versión de multiprocesos sólo la matriz resultado C se comparte explícitamente; A y B se copian por copy-on-write tras `fork()`.

Si deseas que actualice o traduzca algo más en el README, dime qué prefieres (más ejemplos, secciones traducidas al inglés, etc.).
