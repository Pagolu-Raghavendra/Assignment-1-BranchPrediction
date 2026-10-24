# Assignment 1: Branch Predictor
Raghavendra Pagolu

This repo implements the branch predictor.

## Repository layout

| Path | What it is |
|---|---|
| `cbp2-infrastructure-v2/src/my_predictor.h` | **The implementation**: `pm_predictor` (assignment) and `cpm_predictor` (extra credit); the sample `gshare_predictor` is kept as a baseline |
| `cbp2-infrastructure-v2/src/predict.cc`, `predict_extra_credit.cc` | Drivers (instantiate `pm_predictor` / `cpm_predictor`) |
| `cbp2-infrastructure-v2/src/predictor.h` | Base classes. Fixed: the getter `target_prediction()` returned `bool` |
| `cbp2-infrastructure-v2/run`, `run_extra` | Course csh run scripts |
| `cbp2-infrastructure-v2/experiments.sh` | Builds and runs every configuration in the report → `results/results.csv` |
| `cbp2-infrastructure-v2/results_pm.txt` | Output of `./run traces` |
| `cbp2-infrastructure-v2/results_extra_credit.txt` | Extra-credit output (direction + target MPKI per trace) |
| `cbp2-infrastructure-v2/traces/` | The 20 CBP-2 traces |
| `report.pdf` | Report |
| `programming-branch-predictor.pdf`, `milenkovic_WDDD02.pdf` | Assignment handout and reference paper |

## Build and run

Requires Linux, WSL or GitHub Codespaces, with `g++`, `make` and `bzip2`. The course `run` script also needs `csh` and `dc`:

```bash
sudo apt-get install -y g++ make bzip2 csh dc
```

Then, from the infrastructure folder:

```bash
cd cbp2-infrastructure-v2
cd src && make && cd ..

./run traces                                           # pm_predictor on all 20 traces
./src/predict traces/164.gzip/gzip.trace.bz2           # one trace -> 12.436 MPKI

# extra credit (the course run_extra script cannot parse the two-value output line)
for t in $(find traces -name '*.trace.bz2' | sort); do
  printf "%-40s %s\n" "$t" "$(./src/predict_extra_credit $t)"
done
```

`./run traces` ends with `average MPKI: 5.847`.

## References

- V. Uzelac, A. Milenkovic, "Experiment Flows and Microbenchmarks for Reverse Engineering of Branch Predictor Structures," ISPASS 2009.
- CBP-2 infrastructure by D. A. Jimenez. No external cache code was used; the set-associative LRU logic was written for this assignment.
