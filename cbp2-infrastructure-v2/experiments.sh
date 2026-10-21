set -e
cd "$(dirname "$0")"
mkdir -p results/bin
sed 's/new pm_predictor ()/new gshare_predictor ()/' src/predict.cc > results/bin/predict_gshare.cc
CONFIGS=(
  "bimodal_only|src/predict.cc|-DPM_Bimodal_Only=1"
  "gshare_32K|results/bin/predict_gshare.cc|"
  "pm_2way_alloc_every_miss|src/predict.cc|-DPM_Ways=2 -DPM_Mispredict=0"
  "pm_4way_alloc_every_miss|src/predict.cc|-DPM_Mispredict=0"
  "pm_2way|src/predict.cc|-DPM_Ways=2"
  "pm_4way|src/predict.cc|"
  "pm_8way|src/predict.cc|-DPM_Ways=8"
  "cpm_diagram_priority|src/predict_extra_credit.cc|-DLPB_Over_Global=0"
  "cpm|src/predict_extra_credit.cc|"
)
echo "trace,config,direction_mpki,target_mpki" > results/results.csv
for entry in "${CONFIGS[@]}"; do
  IFS='|' read -r name driver flags <<< "$entry"
  g++ -O3 -w -Isrc $flags -o results/bin/$name $driver src/trace.cc
  for t in $(find traces -name '*.trace.*' | sort); do
    nums=($(results/bin/$name "$t" | tail -1 | grep -oE '[0-9]+\.[0-9]+'))
    echo "$(basename $(dirname $t)),$name,${nums[0]},${nums[1]:-}" >> results/results.csv
  done
  echo "done: $name"
done
rm -rf results/bin
