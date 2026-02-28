#!/bin/bash
# build_native.sh — Build Graph500 OpenSHMEM benchmark directly with oshcc
# (no MLIR/LLVM pipeline; straightforward C compilation)
#
# Usage:  ./build_native.sh [clean]

set -e

G500=/mnt/DISCL/home/jcowles/MLIR_testing/Graph500_shemem
SOS_DIR=/mnt/DISCL/home/jcowles/MLIR_testing/openshmem-mlir/openshmem-runtime/SOS-v1.5.2
OSHCC=${SOS_DIR}/bin/oshcc
ATOMIC_LIB_DIR=/opt/apps/nfs/spack-v0.23/opt/spack/linux-rocky9-zen4/gcc-11.4.1/gcc-runtime-11.4.1-7hex6dyh2ttbdeywfkq5vbsinmnhjoub/lib

OUT_DIR=${G500}/build-native
BIN=${OUT_DIR}/graph500_shmem_one_sided

# ---------------------------------------------------------------------------
if [[ "${1}" == "clean" ]]; then
  echo "Cleaning ${OUT_DIR}..."
  rm -rf "${OUT_DIR}"
  echo "Done."
  exit 0
fi

mkdir -p "${OUT_DIR}"

CFLAGS=(
  -g
  -O2
  -std=c99
  -Wall
  -DUSE_OPENSHMEM
  -DSIZE_MUST_BE_A_POWER_OF_TWO
  -D_BSD_SOURCE
  -I"${G500}"
  -I"${G500}/mpi"
  -I"${G500}/generator"
)

SOURCES=(
  "${G500}/mpi/bfs_shmem.c"
  "${G500}/mpi/main.c"
  "${G500}/mpi/oned_csr.c"
  "${G500}/mpi/oned_csc.c"
  "${G500}/mpi/utils.c"
  "${G500}/mpi/validate.c"
  "${G500}/mpi/onesided_shmem.c"
  "${G500}/mpi/shmem_lib.c"
  "${G500}/generator/graph_generator.c"
  "${G500}/generator/make_graph.c"
  "${G500}/generator/splittable_mrg.c"
  "${G500}/generator/utils.c"
)

echo "=== Graph500 Native OpenSHMEM Build ==="
echo "Compiler:  ${OSHCC}"
echo "Output:    ${BIN}"
echo ""

"${OSHCC}" \
  "${CFLAGS[@]}" \
  "${SOURCES[@]}" \
  -Wl,-rpath,"${ATOMIC_LIB_DIR}" \
  -lm -lrt \
  -o "${BIN}"

echo "  Built: ${BIN}"
echo ""
echo "=== Build complete ==="
echo "To run:"
echo "  ./run_native.sh"
echo "  ./run_native.sh -n 4 --scale 16"
