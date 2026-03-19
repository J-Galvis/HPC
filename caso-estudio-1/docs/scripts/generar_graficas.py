
import pandas as pd
import matplotlib.pyplot as plt
import os

# Create a directory to save the plots
if not os.path.exists('graficas'):
    os.makedirs('graficas')

# Path to the data
data_path = 'stats/Mac-mini-de-Daniel.local/20260318_215928.done/'
sizes = [500, 1000, 1300, 1600, 2000, 2300, 2600, 3000, 3300, 3600, 4000]

# --- Plot 1: Execution time vs. Matrix dimension ---
plt.figure(figsize=(10, 6))

# Secuential
df_secuential = pd.read_csv(data_path + 'secuential.csv', header=None, names=sizes)
avg_secuential = df_secuential.mean()
plt.plot(avg_secuential.index, avg_secuential.values, label='Secuencial', marker='o')

# Threads
for num_threads in [2, 4, 8, 16]:
    df = pd.read_csv(data_path + f'threads{num_threads}.csv', header=None, names=sizes)
    avg = df.mean()
    plt.plot(avg.index, avg.values, label=f'{num_threads} Hilos', marker='o')

# Multiprocessing
for num_processes in [2, 4, 8, 16]:
    df = pd.read_csv(data_path + f'mutiprocessing{num_processes}.csv', header=None, names=sizes)
    avg = df.mean()
    plt.plot(avg.index, avg.values, label=f'{num_processes} Procesos', marker='o')


plt.xlabel('Dimensión de la matriz cuadrada (NxN)')
plt.ylabel('Tiempo de ejecución (ms)')
plt.title('Ejecución secuencial vs concurrente')
plt.legend()
plt.grid(True)
plt.savefig('graficas/tiempo_ejecucion.png')
plt.close()


# --- Plot 2: SpeedUp vs. Matrix dimension ---
plt.figure(figsize=(10, 6))

# Threads
for num_threads in [2, 4, 8, 16]:
    df = pd.read_csv(data_path + f'threads{num_threads}.csv', header=None, names=sizes)
    avg = df.mean()
    speedup = avg_secuential / avg
    plt.plot(speedup.index, speedup.values, label=f'{num_threads} Hilos', marker='o')

# Multiprocessing
for num_processes in [2, 4, 8, 16]:
    df = pd.read_csv(data_path + f'mutiprocessing{num_processes}.csv', header=None, names=sizes)
    avg = df.mean()
    speedup = avg_secuential / avg
    plt.plot(speedup.index, speedup.values, label=f'{num_processes} Procesos', marker='o')


plt.xlabel('Dimensión de la matriz cuadrada (NxN)')
plt.ylabel('Speed Up')
plt.title('Valor de Speed Up en función de las dimensiones de la matriz')
plt.legend()
plt.grid(True)
plt.savefig('graficas/speedup.png')
plt.close()

print("Gráficas generadas en la carpeta 'graficas'")

