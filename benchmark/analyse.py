import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
import argparse
import numpy as np

def analyse_latency_data(filename: Path) -> None:
    df = pd.read_csv(filename)
    original_unit = df.columns[0]
    df.columns = ['value']
    data = df.iloc[:, 0]

    # 1. Identify the "Tail" (e.g., the 99th percentile)
    p99 = data.quantile(0.99)
    max_val = data.max()
    
    print(f"99% of data is below: {p99}")
    print(f"Max outlier is: {max_val}")

    # 2. Plot log scale distribution
    bins = np.logspace(np.log10(df['value'].min()), np.log10(df['value'].max()), 100)

    plt.hist(df['value'], bins=bins, color='skyblue', edgecolor='black')
    plt.xscale('log') # This is the magic line
    plt.title("Latency Distribution (Log Scale)")
    plt.xlabel(f"{original_unit} (Log Scale)")
    plt.ylabel("Frequency")
    plt.grid(True, which="both", ls="-", alpha=0.2)
    plt.show()

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description="Analyse latency CSV data.")
    parser.add_argument("filename", type=Path, help="Path to CSV file")
    args = parser.parse_args()
    
    analyse_latency_data(args.filename)

