#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>

#include "lambda.h"

namespace needle {
namespace cpu {

#define ALIGNMENT 256
#define TILE 8
    typedef float scalar_t;
    const size_t ELEM_SIZE = sizeof(scalar_t);

    /**
     * This is a utility structure for maintaining an array aligned to ALIGNMENT
     * boundaries in memory.  This alignment should be at least TILE *
     * ELEM_SIZE, though we make it even larger here by default.
     */
    struct AlignedArray {
        AlignedArray(const size_t size) {
            ptr = (scalar_t *)_aligned_malloc(size * ELEM_SIZE, ALIGNMENT);
            if (!ptr) throw std::bad_alloc();
            this->size = size;
        }
        ~AlignedArray() { _aligned_free(ptr); }
        size_t ptr_as_int() { return (size_t)ptr; }
        scalar_t *ptr;
        size_t size;
    };

    void Fill(AlignedArray *out, scalar_t val) {
        /**
         * Fill the values of an aligned array with val
         */
        for (int i = 0; i < out->size; i++) {
            out->ptr[i] = val;
        }
    }

    void Compact(const AlignedArray &a,
                 AlignedArray *out,
                 std::vector<uint32_t> shape,
                 std::vector<uint32_t> strides,
                 size_t offset) {
        /**
         * Compact an array in memory
         *
         * Args:
         *   a: non-compact representation of the array, given as input
         *   out: compact version of the array to be written
         *   shape: shapes of each dimension for a and out
         *   strides: strides of the *a* array (not out, which has compact
         * strides) offset: offset of the *a* array (not out, which has zero
         * offset, being compact)
         *
         * Returns:
         *  void (you need to modify out directly, rather than returning
         * anything; this is true for all the function will implement here, so
         * we won't repeat this note.)
         */
        uint32_t n = 1;
        std::for_each(shape.begin(), shape.end(), [&](uint32_t x) { n *= x; });
        for (uint32_t i = 0; i < n; i++) {
            size_t src = offset;
            uint32_t rem = i;
            for (int j = shape.size() - 1; j >= 0; j--)
                src += rem % shape[j] * strides[j], rem /= shape[j];
            out->ptr[i] = a.ptr[src];
        }
    }

    void EwiseSetitem(const AlignedArray &a,
                      AlignedArray *out,
                      std::vector<uint32_t> shape,
                      std::vector<uint32_t> strides,
                      size_t offset) {
        /**
         * Set items in a (non-compact) array
         *
         * Args:
         *   a: _compact_ array whose items will be written to out
         *   out: non-compact array whose items are to be written
         *   shape: shapes of each dimension for a and out
         *   strides: strides of the *out* array (not a, which has compact
         * strides) offset: offset of the *out* array (not a, which has zero
         * offset, being compact)
         */
        uint32_t n = 1;
        std::for_each(shape.begin(), shape.end(), [&](uint32_t x) { n *= x; });
        for (uint32_t i = 0; i < n; i++) {
            size_t dst = offset;
            uint32_t rem = i;
            for (int j = shape.size() - 1; j >= 0; j--)
                dst += rem % shape[j] * strides[j], rem /= shape[j];
            out->ptr[dst] = a.ptr[i];
        }
    }

    void ScalarSetitem(const size_t size,
                       scalar_t val,
                       AlignedArray *out,
                       std::vector<uint32_t> shape,
                       std::vector<uint32_t> strides,
                       size_t offset) {
        /**
         * Set items is a (non-compact) array
         *
         * Args:
         *   size: number of elements to write in out array (note that this will
         * note be the same as out.size, because out is a non-compact subset
         * array);  it _will_ be the same as the product of items in shape, but
         * convenient to just pass it here. val: scalar value to write to out:
         * non-compact array whose items are to be written shape: shapes of each
         * dimension of out strides: strides of the out array offset: offset of
         * the out array
         */
        uint32_t n = 1;
        std::for_each(shape.begin(), shape.end(), [&](uint32_t x) { n *= x; });
        for (uint32_t i = 0; i < n; i++) {
            size_t dst = offset;
            uint32_t rem = i;
            for (int j = shape.size() - 1; j >= 0; j--)
                dst += rem % shape[j] * strides[j], rem /= shape[j];
            out->ptr[dst] = val;
        }
    }

#define FUNC_EWISE_UNARY(func, a, out, fn)                                     \
    void func(const AlignedArray &a, AlignedArray *out) {                      \
        for (size_t i = 0; i < a.size; i++) out->ptr[i] = fn(a.ptr[i]);        \
    }
#define FUNC_EWISE_BINARY(func, a, b, out, fn)                                 \
    void func(                                                                 \
        const AlignedArray &a, const AlignedArray &b, AlignedArray *out) {     \
        for (size_t i = 0; i < a.size; i++)                                    \
            out->ptr[i] = fn(a.ptr[i], b.ptr[i]);                              \
    }
#define FUNC_SCALAR(func, a, val, out, fn)                                     \
    void func(const AlignedArray &a, scalar_t val, AlignedArray *out) {        \
        for (size_t i = 0; i < a.size; i++) out->ptr[i] = fn(a.ptr[i], val);   \
    }

    FUNC_EWISE_BINARY(EwiseAdd, a, b, out, LAMBDA_ADD)
    FUNC_SCALAR(ScalarAdd, a, val, out, LAMBDA_ADD)

    FUNC_EWISE_BINARY(EwiseMul, a, b, out, LAMBDA_MUL)
    FUNC_SCALAR(ScalarMul, a, val, out, LAMBDA_MUL)

    FUNC_EWISE_BINARY(EwiseDiv, a, b, out, LAMBDA_DIV)
    FUNC_SCALAR(ScalarDiv, a, val, out, LAMBDA_DIV)

    FUNC_SCALAR(ScalarPower, a, val, out, LAMBDA_POW)

    FUNC_EWISE_BINARY(EwiseMaximum, a, b, out, LAMBDA_MAX)
    FUNC_SCALAR(ScalarMaximum, a, val, out, LAMBDA_MAX)

    FUNC_EWISE_BINARY(EwiseEq, a, b, out, LAMBDA_EQ)
    FUNC_SCALAR(ScalarEq, a, val, out, LAMBDA_EQ)

    FUNC_EWISE_BINARY(EwiseGe, a, b, out, LAMBDA_GE)
    FUNC_SCALAR(ScalarGe, a, val, out, LAMBDA_GE)

    FUNC_EWISE_UNARY(EwiseLog, a, out, LAMBDA_LOG)

    FUNC_EWISE_UNARY(EwiseExp, a, out, LAMBDA_EXP)

    FUNC_EWISE_UNARY(EwiseTanh, a, out, LAMBDA_TANH)

    void Matmul(const AlignedArray &a,
                const AlignedArray &b,
                AlignedArray *out,
                uint32_t m,
                uint32_t n,
                uint32_t p) {
        /**
         * Multiply two (compact) matrices into an output (also compact) matrix.
         * For this implementation you can use the "naive" three-loop algorithm.
         *
         * Args:
         *   a: compact 2D array of size m x n
         *   b: compact 2D array of size n x p
         *   out: compact 2D array of size m x p to write the output to
         *   m: rows of a / out
         *   n: columns of a / rows of b
         *   p: columns of b / out
         */

        for (size_t i = 0; i < m; i++)
            for (size_t k = 0; k < p; k++) {
                scalar_t sum = 0;
                for (size_t j = 0; j < n; j++)
                    sum += a.ptr[i * n + j] * b.ptr[j * p + k];
                out->ptr[i * p + k] = sum;
            }
    }

    inline void AlignedDot(const float *__restrict a,
                           const float *__restrict b,
                           float *__restrict out) {
        /**
         * Multiply together two TILE x TILE matrices, and _add _the result to
         * out (it is important to add the result to the existing out, which you
         * should not set to zero beforehand).  We are including the compiler
         * flags here that enable the compile to properly use vector operators
         * to implement this function.  Specifically, the __restrict__ keyword
         * indicates to the compile that a, b, and out don't have any
         * overlapping memory (which is necessary in order for vector operations
         * to be equivalent to their non-vectorized counterparts (imagine what
         * could happen otherwise if a, b, and out had overlapping memory).
         * Similarly the __builtin_assume_aligned keyword tells the compiler
         * that the input array will be aligned to the appropriate blocks in
         * memory, which also helps the compiler vectorize the code.
         *
         * Args:
         *   a: compact 2D array of size TILE x TILE
         *   b: compact 2D array of size TILE x TILE
         *   out: compact 2D array of size TILE x TILE to write to
         */

        a = (const float *)__builtin_assume_aligned(a, TILE * ELEM_SIZE);
        b = (const float *)__builtin_assume_aligned(b, TILE * ELEM_SIZE);
        out = (float *)__builtin_assume_aligned(out, TILE * ELEM_SIZE);

        for (size_t i = 0; i < TILE; i++)
            for (size_t k = 0; k < TILE; k++)
                for (size_t j = 0; j < TILE; j++)
                    out[i * TILE + k] += a[i * TILE + j] * b[j * TILE + k];
    }

    void MatmulTiled(const AlignedArray &a,
                     const AlignedArray &b,
                     AlignedArray *out,
                     uint32_t m,
                     uint32_t n,
                     uint32_t p) {
        /**
         * Matrix multiplication on tiled representations of array.  In this
         * setting, a, b, and out are all *4D* compact arrays of the appropriate
         * size, e.g. a is an array of size a[m/TILE][n/TILE][TILE][TILE] You
         * should do the multiplication tile-by-tile to improve performance of
         * the array (i.e., this function should call `AlignedDot()` implemented
         * above).
         *
         * Note that this function will only be called when m, n, p are all
         * multiples of TILE, so you can assume that this division happens
         * without any remainder.
         *
         * Args:
         *   a: compact 4D array of size m/TILE x n/TILE x TILE x TILE
         *   b: compact 4D array of size n/TILE x p/TILE x TILE x TILE
         *   out: compact 4D array of size m/TILE x p/TILE x TILE x TILE to
         * write to m: rows of a / out n: columns of a / rows of b p: columns of
         * b / out
         *
         */

#define TILE_BASE_ADDR(b, r, c, i, j) (b + (i * c + j * TILE) * TILE)

        for (size_t i = 0; i < m / TILE; i++)
            for (size_t k = 0; k < p / TILE; k++) {
                memset(TILE_BASE_ADDR(out->ptr, m, p, i, k),
                       0,
                       TILE * TILE * ELEM_SIZE);
                for (size_t j = 0; j < n / TILE; j++)
                    AlignedDot(TILE_BASE_ADDR(a.ptr, m, n, i, j),
                               TILE_BASE_ADDR(b.ptr, n, p, j, k),
                               TILE_BASE_ADDR(out->ptr, m, p, i, k));
            }
    }

    void
    ReduceMax(const AlignedArray &a, AlignedArray *out, size_t reduce_size) {
        /**
         * Reduce by taking maximum over `reduce_size` contiguous blocks.
         *
         * Args:
         *   a: compact array of size a.size = out.size * reduce_size to reduce
         * over out: compact array to write into reduce_size: size of the
         * dimension to reduce over
         */

        for (size_t i = 0; i < out->size; i++)
            out->ptr[i] = *std::max_element(a.ptr + i * reduce_size,
                                            a.ptr + (i + 1) * reduce_size);
    }

    void
    ReduceSum(const AlignedArray &a, AlignedArray *out, size_t reduce_size) {
        /**
         * Reduce by taking sum over `reduce_size` contiguous blocks.
         *
         * Args:
         *   a: compact array of size a.size = out.size * reduce_size to reduce
         * over out: compact array to write into reduce_size: size of the
         * dimension to reduce over
         */

        for (size_t i = 0; i < out->size; i++)
            out->ptr[i] = std::accumulate(a.ptr + i * reduce_size,
                                          a.ptr + (i + 1) * reduce_size,
                                          (scalar_t)0);
    }

}  // namespace cpu
}  // namespace needle

PYBIND11_MODULE(ndarray_backend_cpu, m) {
    namespace py = pybind11;
    using namespace needle;
    using namespace cpu;

    m.attr("__device_name__") = "cpu";
    m.attr("__tile_size__") = TILE;

    py::class_<AlignedArray>(m, "Array")
        .def(py::init<size_t>(), py::return_value_policy::take_ownership)
        .def("ptr", &AlignedArray::ptr_as_int)
        .def_readonly("size", &AlignedArray::size);

    // return numpy array (with copying for simplicity, otherwise garbage
    // collection is a pain)
    m.def("to_numpy",
          [](const AlignedArray &a,
             std::vector<size_t> shape,
             std::vector<size_t> strides,
             size_t offset) {
              std::vector<size_t> numpy_strides = strides;
              std::transform(numpy_strides.begin(),
                             numpy_strides.end(),
                             numpy_strides.begin(),
                             [](size_t &c) { return c * ELEM_SIZE; });
              return py::array_t<scalar_t>(
                  shape, numpy_strides, a.ptr + offset);
          });

    // convert from numpy (with copying)
    m.def("from_numpy", [](py::array_t<scalar_t> a, AlignedArray *out) {
        std::memcpy(out->ptr, a.request().ptr, out->size * ELEM_SIZE);
    });

    m.def("fill", Fill);
    m.def("compact", Compact);
    m.def("ewise_setitem", EwiseSetitem);
    m.def("scalar_setitem", ScalarSetitem);
    m.def("ewise_add", EwiseAdd);
    m.def("scalar_add", ScalarAdd);

    m.def("ewise_mul", EwiseMul);
    m.def("scalar_mul", ScalarMul);
    m.def("ewise_div", EwiseDiv);
    m.def("scalar_div", ScalarDiv);
    m.def("scalar_power", ScalarPower);

    m.def("ewise_maximum", EwiseMaximum);
    m.def("scalar_maximum", ScalarMaximum);
    m.def("ewise_eq", EwiseEq);
    m.def("scalar_eq", ScalarEq);
    m.def("ewise_ge", EwiseGe);
    m.def("scalar_ge", ScalarGe);

    m.def("ewise_log", EwiseLog);
    m.def("ewise_exp", EwiseExp);
    m.def("ewise_tanh", EwiseTanh);

    m.def("matmul", Matmul);
    m.def("matmul_tiled", MatmulTiled);

    m.def("reduce_max", ReduceMax);
    m.def("reduce_sum", ReduceSum);
}
