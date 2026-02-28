#!/bin/bash
# build_graph500.sh — Compile Graph500 OpenSHMEM benchmark through the
# ClangIR → OpenSHMEM MLIR → LLVM IR pipeline.
#
# Usage:
#   ./build_graph500.sh              # build bin/graph500_shmem_one_sided
#   ./build_graph500.sh --run        # build and run with srun -n 2, scale=10
#   ./build_graph500.sh --run -n 4 --scale 14
#
# Output: bin/graph500_shmem_one_sided

set -e

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
CLANG=/mnt/DISCL/home/jcowles/MLIR_testing/clangir/build-new/bin/clang
CIR_OPT=/mnt/DISCL/home/jcowles/MLIR_testing/clangir/build-new/bin/cir-opt
SHMEM_OPT=/mnt/DISCL/home/jcowles/MLIR_testing/openshmem-mlir/build-incubator/bin/shmem-mlir-opt
MLIR_TRANSLATE=/mnt/DISCL/home/jcowles/MLIR_testing/clangir/build-new/bin/mlir-translate
LLC=/mnt/DISCL/home/jcowles/MLIR_testing/clangir/build-new/bin/llc

SOS_DIR=/mnt/DISCL/home/jcowles/MLIR_testing/openshmem-mlir/openshmem-runtime/SOS-v1.5.2
OSHCC=${SOS_DIR}/bin/oshcc
SHMEM_INC=${SOS_DIR}/include

ATOMIC_LIB_DIR=/opt/apps/nfs/spack-v0.23/opt/spack/linux-rocky9-zen4/gcc-11.4.1/gcc-runtime-11.4.1-7hex6dyh2ttbdeywfkq5vbsinmnhjoub/lib

# ---------------------------------------------------------------------------
# Graph500 source layout
# ---------------------------------------------------------------------------
G500=/mnt/DISCL/home/jcowles/MLIR_testing/Graph500_shemem

# Sources for graph500_shmem_one_sided (mirroring mpi/Makefile target)
SHMEM_SOURCES=(
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

# ---------------------------------------------------------------------------
# Args
# ---------------------------------------------------------------------------
DO_RUN=0
RUN_NP=2
RUN_SCALE=10
while [[ $# -gt 0 ]]; do
  case "$1" in
    --run)    DO_RUN=1; shift ;;
    -n)       RUN_NP="$2";    shift 2 ;;
    --scale)  RUN_SCALE="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ---------------------------------------------------------------------------
# Build directories
# ---------------------------------------------------------------------------
BUILD_DIR=/mnt/DISCL/home/jcowles/MLIR_testing/Graph500_shemem/build-mlir
mkdir -p \
  "${BUILD_DIR}/cir" \
  "${BUILD_DIR}/flat_cir" \
  "${BUILD_DIR}/openshmem_mlir" \
  "${BUILD_DIR}/partial_llvm" \
  "${BUILD_DIR}/llvm_with_casts" \
  "${BUILD_DIR}/llvm_mlir" \
  "${BUILD_DIR}/llvm_ir" \
  "${BUILD_DIR}/obj" \
  "${BUILD_DIR}/bin"

BIN="${BUILD_DIR}/bin/graph500_shmem_one_sided"

# ---------------------------------------------------------------------------
# Common compiler flags (matches mpi/Makefile CFLAGS + make.inc.oshmem.gcc)
# ---------------------------------------------------------------------------
CFLAGS=(
  -std=c99
  -g
  -Wall
  -DUSE_OPENSHMEM
  -D_BSD_SOURCE
  -I"${G500}"
  -I"${G500}/mpi"
  -I"${G500}/generator"
  -I"${SHMEM_INC}"
)

# ---------------------------------------------------------------------------
# Per-file pipeline
# ---------------------------------------------------------------------------
OBJ_FILES=()

compile_file() {
  local src="$1"
  # Use parent_dir + basename to produce unique object-file names
  # (e.g. mpi/utils.c → mpi_utils, generator/utils.c → generator_utils)
  local parent
  parent="$(basename "$(dirname "${src}")")"
  local base
  base="${parent}_$(basename "${src}" .c)"

  echo "  [${base}]"

  # Step 1: C → CIR
  ${CLANG} "${CFLAGS[@]}" -fclangir -emit-cir "${src}" \
    -o "${BUILD_DIR}/cir/${base}.cir" 2>&1

  # Step 2: CIR → flattened CIR (removes cir.scope/cir.if)
  ${CIR_OPT} \
    --cir-flatten-cfg \
    --cir-abi-lowering \
    "${BUILD_DIR}/cir/${base}.cir" \
    -o "${BUILD_DIR}/flat_cir/${base}.flat.cir" 2>&1

  # Step 3: Flat CIR → OpenSHMEM MLIR
  ${SHMEM_OPT} \
    --convert-cir-to-openshmem \
    "${BUILD_DIR}/flat_cir/${base}.flat.cir" \
    -o "${BUILD_DIR}/openshmem_mlir/${base}.openshmem.mlir" 2>&1

  # Step 4: Residual CIR → LLVM MLIR (OpenSHMEM ops left in place)
  ${SHMEM_OPT} \
    --cir-to-llvm \
    "${BUILD_DIR}/openshmem_mlir/${base}.openshmem.mlir" \
    -o "${BUILD_DIR}/partial_llvm/${base}.partial.mlir" 2>&1

  # Step 5: OpenSHMEM → LLVM
  ${SHMEM_OPT} \
    --convert-openshmem-to-llvm \
    "${BUILD_DIR}/partial_llvm/${base}.partial.mlir" \
    -o "${BUILD_DIR}/llvm_with_casts/${base}.casts.mlir" 2>&1

  # Step 6: Reconcile unrealized casts
  ${SHMEM_OPT} \
    --reconcile-unrealized-casts \
    "${BUILD_DIR}/llvm_with_casts/${base}.casts.mlir" \
    -o "${BUILD_DIR}/llvm_mlir/${base}.llvm.mlir" 2>&1

  # Step 7: LLVM MLIR → LLVM IR
  ${MLIR_TRANSLATE} \
    --allow-unregistered-dialect \
    --mlir-to-llvmir \
    "${BUILD_DIR}/llvm_mlir/${base}.llvm.mlir" \
    -o "${BUILD_DIR}/llvm_ir/${base}.ll" 2>&1

  # Step 8: LLVM IR → object file
  ${LLC} \
    -filetype=obj \
    -relocation-model=pic \
    "${BUILD_DIR}/llvm_ir/${base}.ll" \
    -o "${BUILD_DIR}/obj/${base}.o" 2>&1

  OBJ_FILES+=("${BUILD_DIR}/obj/${base}.o")
}

# ---------------------------------------------------------------------------
# Compile all sources
# ---------------------------------------------------------------------------
echo "=== Graph500 OpenSHMEM MLIR Build ==="
echo "Sources: ${#SHMEM_SOURCES[@]} files"
echo ""
echo "Compiling..."

FAIL=0
for src in "${SHMEM_SOURCES[@]}"; do
  compile_file "${src}" || { echo "  FAILED: ${src}"; FAIL=1; }
done

if [[ ${FAIL} -eq 1 ]]; then
  echo ""
  echo "ERROR: One or more files failed to compile." >&2
  exit 1
fi

echo ""

# ---------------------------------------------------------------------------
# Link
# ---------------------------------------------------------------------------
echo "Linking ${BIN}..."
SHMEM_CC="${CLANG}" \
${OSHCC} \
  "${OBJ_FILES[@]}" \
  -Wl,-rpath,"${ATOMIC_LIB_DIR}" \
  -lm -lrt \
  -o "${BIN}"

echo "  Generated: ${BIN}"
echo ""
echo "=== Build complete ==="
echo "To run:"
echo "  OSHRUN_LAUNCHER=srun SHMEM_SYMMETRIC_HEAP_SIZE=640000000 \\"
echo "    ${SOS_DIR}/bin/oshrun -n <N> ${BIN} <scale> <edgefactor>"
echo "  Example: OSHRUN_LAUNCHER=srun ... oshrun -n 2 ${BIN} 10 16"
echo ""

# ---------------------------------------------------------------------------
# Optional run
# ---------------------------------------------------------------------------
if [[ ${DO_RUN} -eq 1 ]]; then
  echo "Running: scale=${RUN_SCALE} edgefactor=16, ${RUN_NP} PEs..."
  PATH="${SOS_DIR}/bin:${PATH}" \
  LD_LIBRARY_PATH="${SOS_DIR}/lib:${LD_LIBRARY_PATH:-}" \
  OSHRUN_LAUNCHER=srun \
  SHMEM_SYMMETRIC_HEAP_SIZE=640000000 \
  oshrun -n "${RUN_NP}" "${BIN}" "${RUN_SCALE}" 16
fi
