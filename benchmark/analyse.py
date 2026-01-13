import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
import argparse
import numpy as np

def analyse_latency_data(filenames: list[Path]) -> None:
    plt.figure(figsize=(10, 6))
    
    # Track min/max across all files to set consistent bins
    all_data = []
    
    for filename in filenames:
        if not filename.exists():
            print(f"Skipping {filename}: File not found")
            continue
            
        df = pd.read_csv(filename)
        # Assuming the first column is the latency value
        data = df.iloc[:, 0]
        label = filename.stem # Use filename as the legend label
        
        # Calculate stats
        p99 = data.quantile(0.99)
        print(f"[{label}] 99th: {p99:.2f} | Max: {data.max():.2f}")
        
        all_data.append((data, label))

    if not all_data:
        return

    # 1. Determine global min/max for the log-bins
    global_min = min(d.min() for d, _ in all_data)
    global_max = max(d.max() for d, _ in all_data)
    bins = np.logspace(np.log10(global_min), np.log10(global_max), 100)

    # 2. Plot all datasets on the same histogram
    for data, label in all_data:
        plt.hist(data, bins=bins, density=True, alpha=0.5, label=label, edgecolor='black' if len(all_data) == 1 else None)

    plt.xscale('log')
    plt.title("Latency Distribution Comparison")
    plt.xlabel("Latency (Log Scale)")
    plt.ylabel("Frequency")
    plt.legend(loc='upper right')
    plt.grid(True, which="both", ls="-", alpha=0.2)
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description="Analyse latency CSV data.")
    parser.add_argument("filenames", type=Path, nargs="+", help="Path(s) to CSV file")
    args = parser.parse_args()
    
    analyse_latency_data(args.filenames)

