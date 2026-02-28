#!/bin/bash
# run_graph500.sh — Run the Graph500 OpenSHMEM benchmark
#
# Usage:
#   ./run_graph500.sh                  # 2 PEs, scale=10, edgefactor=16
#   ./run_graph500.sh -n 4             # 4 PEs
#   ./run_graph500.sh -n 4 --scale 14  # 4 PEs, scale=14
#   ./run_graph500.sh --scale 16 --ef 32

set -e

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
NP=2
SCALE=10
EDGEFACTOR=16

# ---------------------------------------------------------------------------
# Parse args
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    -n)       NP="$2";         shift 2 ;;
    --scale)  SCALE="$2";      shift 2 ;;
    --ef)     EDGEFACTOR="$2"; shift 2 ;;
    -h|--help)
      echo "Usage: $0 [-n <nprocs>] [--scale <scale>] [--ef <edgefactor>]"
      echo "  -n         Number of PEs (default: 2)"
      echo "  --scale    Graph scale (default: 10)"
      echo "  --ef       Edge factor (default: 16)"
      exit 0
      ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SOS_DIR=/mnt/DISCL/home/jcowles/MLIR_testing/openshmem-mlir/openshmem-runtime/SOS-v1.5.2
BIN=/mnt/DISCL/home/jcowles/MLIR_testing/Graph500_shemem/build-mlir/bin/graph500_shmem_one_sided

if [[ ! -f "${BIN}" ]]; then
  echo "ERROR: executable not found: ${BIN}" >&2
  echo "Run ./build_graph500.sh first." >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
echo "=== Graph500 OpenSHMEM Benchmark ==="
echo "  PEs:         ${NP}"
echo "  Scale:       ${SCALE}"
echo "  Edge factor: ${EDGEFACTOR}"
echo ""

export PATH="${SOS_DIR}/bin:${PATH}"
export LD_LIBRARY_PATH="${SOS_DIR}/lib:${LD_LIBRARY_PATH:-}"
export OSHRUN_LAUNCHER=srun
export SHMEM_SYMMETRIC_HEAP_SIZE=640000000

oshrun -n "${NP}" "${BIN}" "${SCALE}" "${EDGEFACTOR}"
