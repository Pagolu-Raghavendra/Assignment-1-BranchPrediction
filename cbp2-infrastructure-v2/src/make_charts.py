import csv
import os
from collections import defaultdict
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
HERE = os.path.dirname(os.path.abspath(__file__))
BASELINE_LIGHT = "#b4b2a9"       # bimodal only
BASELINE_DARK = "#7a7870"        # gshare
PM_BLUE = "#86b6ef"              # Pentium M variants
PM_SUBMITTED = "#1c5cab"         # the submitted pm_predictor
EXTRA_ORANGE = "#eb6834"         # extra credit
TEXT = "#2b2b29"
TEXT_MUTED = "#6b6a63"
GRID = "#e4e3dd"
BACKGROUND = "#ffffff"
CONFIGS = {
    "bimodal_only":             ("Bimodal only (4K)",              BASELINE_LIGHT),
    "gshare_32K":               ("gshare 32K (given sample)",      BASELINE_DARK),
    "pm_2way_alloc_every_miss": ("PM 2-way, alloc every miss",     PM_BLUE),
    "pm_4way_alloc_every_miss": ("PM 4-way, alloc every miss",     PM_BLUE),
    "pm_2way":                  ("PM 2-way",                       PM_BLUE),
    "pm_4way":                  ("PM 4-way (submitted)",           PM_SUBMITTED),
    "pm_8way":                  ("PM 8-way",                       PM_BLUE),
    "cpm_diagram_priority":     ("Extra credit, diagram priority", EXTRA_ORANGE),
    "cpm":                      ("Extra credit (+loop predictor)", EXTRA_ORANGE),
}
def load_results(path):
    results = defaultdict(dict)
    with open(path) as f:
        for row in csv.DictReader(f):
            results[row["config"]][row["trace"]] = float(row["direction_mpki"])
    return results
def style_axes(ax):
    ax.grid(axis="x", color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right", "left"):
        ax.spines[side].set_visible(False)
    ax.spines["bottom"].set_color(GRID)
    ax.tick_params(colors=TEXT_MUTED, length=0)
    ax.xaxis.label.set_color(TEXT_MUTED)
def plot_averages(results, out_file):
    averages = {name: sum(v.values()) / len(v) for name, v in results.items()}
    names = sorted(averages, key=averages.get, reverse=True)
    values = [averages[n] for n in names]
    colours = [CONFIGS[n][1] for n in names]
    fig, ax = plt.subplots(figsize=(7.2, 4.3))
    ax.barh(range(len(names)), values, height=0.62, color=colours)
    for y, (name, value) in enumerate(zip(names, values)):
        weight = "bold" if name == "pm_4way" else "normal"
        ax.text(value + 0.12, y, f"{value:.3f}", va="center", fontsize=8,
                color=TEXT, fontweight=weight)
    ax.set_yticks(range(len(names)))
    ax.set_yticklabels([CONFIGS[n][0] for n in names], color=TEXT)
    ax.set_xlabel("Average MPKI over 20 traces (lower is better)")
    ax.set_xlim(0, max(values) * 1.12)
    style_axes(ax)
    legend = [Patch(color=BASELINE_DARK, label="Baselines"),
              Patch(color=PM_BLUE, label="Pentium M (pm_predictor)"),
              Patch(color=PM_SUBMITTED, label="Submitted configuration"),
              Patch(color=EXTRA_ORANGE, label="Extra credit (cpm_predictor)")]
    ax.legend(handles=legend, loc="lower center", bbox_to_anchor=(0.35, 1.0),
              ncol=2, fontsize=7.5, frameon=False)
    fig.tight_layout()
    fig.savefig(out_file, dpi=200, facecolor=BACKGROUND)
    plt.close(fig)
def plot_per_trace(results, out_file):
    series = [
        ("bimodal_only", "Bimodal only", BASELINE_LIGHT),
        ("gshare_32K", "gshare 32K", BASELINE_DARK),
        ("pm_4way", "Pentium M hybrid (submitted)", PM_SUBMITTED),
        ("cpm", "Extra credit (+loop predictor)", EXTRA_ORANGE),
    ]
    traces = sorted(results["pm_4way"])
    bar_height = 0.2
    fig, ax = plt.subplots(figsize=(7.5, 8.8))
    for i, (config, label, colour) in enumerate(series):
        positions = [t + (i - 1.5) * bar_height for t in range(len(traces))]
        values = [results[config][trace] for trace in traces]
        ax.barh(positions, values, height=bar_height * 0.85, color=colour, label=label)
    ax.set_yticks(range(len(traces)))
    ax.set_yticklabels(traces, color=TEXT)
    ax.invert_yaxis()
    ax.set_xlabel("MPKI (lower is better)")
    style_axes(ax)
    ax.legend(loc="lower center", bbox_to_anchor=(0.45, 1.0), ncol=2,
              fontsize=8, frameon=False)
    fig.tight_layout()
    fig.savefig(out_file, dpi=200, facecolor=BACKGROUND)
    plt.close(fig)
if __name__ == "__main__":
    plt.rcParams["font.family"] = "DejaVu Sans"
    plt.rcParams["font.size"] = 9
    data = load_results(os.path.join(HERE, "results.csv"))
    plot_averages(data, os.path.join(HERE, "avg_mpki.png"))
    plot_per_trace(data, os.path.join(HERE, "per_trace_mpki.png"))
    print("charts written to", HERE)