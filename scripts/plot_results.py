
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


# Configuration
CSV_PATH = Path("results/results.csv")
OUTPUT_DIR = Path("results/plots")

JOB_COUNT = 1000
CLIENT_LOAD = 0

ALGORITHMS = [
    "FIFO",
    "SJF",
    "Risk-aware",
    "Failure-aware",
]


def load_results():
    df = pd.read_csv(CSV_PATH)

    required = {
        "job_count",
        "seed",
        "algorithm",
        "concurrency",
        "client_load_percent",
        "avg_completion_hours",
        "risk_exposure",
        "expected_data_losses",
    }

    missing = required - set(df.columns)

    if missing:
        raise ValueError(f"Missing columns: {missing}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    return df


def plot_algorithm_comparison(df, metric, ylabel, filename):
    subset = df[
        (df["job_count"] == JOB_COUNT)
        & (df["client_load_percent"] == CLIENT_LOAD)
    ]

    grouped = (
        subset.groupby(["algorithm", "concurrency"])[metric]
        .mean()
        .reset_index()
    )

    plt.figure(figsize=(10, 6))

    for algorithm in ALGORITHMS:
        data = grouped[grouped["algorithm"] == algorithm]

        plt.plot(
            data["concurrency"],
            data[metric],
            marker="o",
            linewidth=2,
            label=algorithm,
        )

    plt.title(f"{ylabel} vs Concurrency ({JOB_COUNT:,} Jobs)")
    plt.xlabel("Concurrent Recovery Jobs")
    plt.ylabel(ylabel)
    plt.xticks(sorted(grouped["concurrency"].unique()))
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig(OUTPUT_DIR / filename, dpi=300)
    plt.close()


def plot_client_contention(df):
    subset = df[
        (df["job_count"] == JOB_COUNT)
        & (df["algorithm"] == "SJF")
    ]

    grouped = (
        subset.groupby(
            ["client_load_percent", "concurrency"]
        )["avg_completion_hours"]
        .mean()
        .reset_index()
    )

    plt.figure(figsize=(10, 6))

    for load in sorted(grouped["client_load_percent"].unique()):
        data = grouped[grouped["client_load_percent"] == load]

        plt.plot(
            data["concurrency"],
            data["avg_completion_hours"],
            marker="o",
            linewidth=2,
            label=f"{load:g}% Client Load",
        )

    plt.title("Client I/O Contention vs Recovery Concurrency")
    plt.xlabel("Concurrent Recovery Jobs")
    plt.ylabel("Average Completion Time (Hours)")
    plt.xticks(sorted(grouped["concurrency"].unique()))
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "client_contention.png",
        dpi=300,
    )
    plt.close()


def main():
    df = load_results()

    plot_algorithm_comparison(
        df,
        metric="avg_completion_hours",
        ylabel="Average Completion Time (Hours)",
        filename="completion_time_vs_concurrency.png",
    )

    plot_algorithm_comparison(
        df,
        metric="risk_exposure",
        ylabel="Risk Exposure",
        filename="risk_exposure_vs_concurrency.png",
    )

    plot_algorithm_comparison(
        df,
        metric="expected_data_losses",
        ylabel="Expected Data Losses",
        filename="expected_losses_vs_concurrency.png",
    )

    plot_client_contention(df)

    print(f"Graphs saved to: {OUTPUT_DIR.resolve()}")


if __name__ == "__main__":
    main()
