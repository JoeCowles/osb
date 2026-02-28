#!/bin/bash
# run_native.sh — Run the natively-built Graph500 OpenSHMEM benchmark
#
# Usage:
#   ./run_native.sh                  # 2 PEs, scale=10, edgefactor=16
#   ./run_native.sh -n 4             # 4 PEs
#   ./run_native.sh -n 4 --scale 14 --ef 32

set -e

NP=2
SCALE=10
EDGEFACTOR=16

while [[ $# -gt 0 ]]; do
  case "$1" in
    -n)       NP="$2";         shift 2 ;;
    --scale)  SCALE="$2";      shift 2 ;;
    --ef)     EDGEFACTOR="$2"; shift 2 ;;
    -h|--help)
      echo "Usage: $0 [-n <nprocs>] [--scale <scale>] [--ef <edgefactor>]"
      exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

SOS_DIR=/mnt/DISCL/home/jcowles/MLIR_testing/openshmem-mlir/openshmem-runtime/SOS-v1.5.2
BIN=/mnt/DISCL/home/jcowles/MLIR_testing/Graph500_shemem/build-native/graph500_shmem_one_sided

if [[ ! -f "${BIN}" ]]; then
  echo "ERROR: ${BIN} not found — run ./build_native.sh first" >&2
  exit 1
fi

# SIZE_MUST_BE_A_POWER_OF_TWO requires PEs to be a power of 2
if (( NP & (NP - 1) )); then
  echo "ERROR: -n ${NP} is not a power of two. Use 2, 4, 8, 16, ..." >&2
  exit 1
fi

echo "=== Graph500 OpenSHMEM (native) ==="
echo "  PEs: ${NP}  Scale: ${SCALE}  EF: ${EDGEFACTOR}"
echo ""

export PATH="${SOS_DIR}/bin:${PATH}"
export LD_LIBRARY_PATH="${SOS_DIR}/lib:${LD_LIBRARY_PATH:-}"
export OSHRUN_LAUNCHER=srun
export SHMEM_SYMMETRIC_HEAP_SIZE=640000000

oshrun -n "${NP}" "${BIN}" "${SCALE}" "${EDGEFACTOR}"
