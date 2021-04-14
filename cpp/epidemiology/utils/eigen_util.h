#pragma once

#include "epidemiology/utils/eigen.h"
#include <utility>
#include <iostream>

namespace epi
{

/**
 * @brief sequence of indices
 */
template <typename T>
struct Seq {
    Seq(T start_, T n_, T stride_ = 1)
        : start(start_)
        , n(n_)
        , stride(stride_)
    {
        assert(start >= 0);
        assert(n >= 0);
        assert(stride >= 1);
    }
    T start, n, stride = 1;
};

/**
 * @brief check if Eigen::Matrix type M is a dynamic vector type.
 */
template <class M>
struct is_dynamic_vector {
    static constexpr bool value =
        (std::remove_reference_t<M>::RowsAtCompileTime == Eigen::Dynamic && std::remove_reference_t<M>::ColsAtCompileTime == 1) ||
        (std::remove_reference_t<M>::RowsAtCompileTime == 1 && std::remove_reference_t<M>::ColsAtCompileTime == Eigen::Dynamic);
};

/**
 * @brief check if Eigen::Matrix type M is a dynamic matrix type.
 */
template <class M>
struct is_dynamic_matrix {
    static constexpr bool value =
        std::remove_reference_t<M>::RowsAtCompileTime == Eigen::Dynamic && std::remove_reference_t<M>::ColsAtCompileTime == Eigen::Dynamic;
};

/**
 * @brief number of rows (columns) of a row (column) major matrix.
 */
template <class M>
Eigen::Index major_size(M&& m)
{
    return std::remove_reference_t<M>::IsRowMajor ? m.rows() : m.cols();
}

/**
 * @brief number of columns (rows) of a row (column) major matrix.
 */
template <class M>
Eigen::Index minor_size(M&& m)
{
    return std::remove_reference_t<M>::IsRowMajor ? m.cols() : m.rows();
}

/**
 * helper to get the matrix type from an eigen expression 
 * with correct const volatile qualitfications.
 */
template<class M>
struct CVPlainMatrix
{
    using Type = typename M::PlainMatrix;
};
template<class M>
struct CVPlainMatrix<Eigen::Ref<const M>>
{
    using Type = const M;
};
template<class M>
using CVPlainMatrixT = typename CVPlainMatrix<M>::Type;

/**
 * @brief take a regular slice of a row or column vector.
 * The slices shares the same memory as the original vector, no copying is performed,
 * changes to the slice are also made to the original vector.
 * Assign to a different vector of compatible size if you need a copy.
 * @param v Row or column vector to take a slice of
 * @param elems sequence of row or column indices
 * @returns vector expression with selected entries from the input vector
 */
template <class V, std::enable_if_t<is_dynamic_vector<V>::value, int> = 0>
auto slice(V&& v, Seq<Eigen::Index> elems)
{
    return Eigen::Map<CVPlainMatrixT<std::decay_t<V>>, 0, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>(
        v.data() + elems.start, elems.n, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>{1, elems.stride});
}

/**
 * @brief take a regular slice of a matrix.
 * The slices shares the same memory as the original matrix, no copying is performed,
 * changes to the slice are also made to the original matrix.
 * Assign to a different matrix of compatible size if you need a copy.
 * @param m Matrix to take a slice of
 * @param rows sequence of row indices
 * @param cols sequence of column indices
 * @returns matrix expression with selected entries from the input matrix
 */
template <class M, std::enable_if_t<is_dynamic_matrix<M>::value, int> = 0>
auto slice(M&& m, Seq<Eigen::Index> rows, Seq<Eigen::Index> cols)
{
    assert(rows.start + rows.stride * rows.n <= m.rows());
    assert(cols.start + cols.stride * cols.n <= m.rows());

    auto majSpec   = std::remove_reference_t<M>::IsRowMajor ? rows : cols;
    auto minSpec   = std::remove_reference_t<M>::IsRowMajor ? cols : rows;
    auto majStride = majSpec.stride * minor_size(m);
    auto minStride = minSpec.stride;
    auto data      = m.data() + majSpec.start * minor_size(m) + minSpec.start;

    return Eigen::Map<CVPlainMatrixT<std::remove_reference_t<M>>, Eigen::Unaligned, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>(
        data, rows.n, cols.n, {majStride, minStride});
}

/**
 * @brief reshape the matrix.
 * Total number of entries before and after the reshape must be the same.
 * The new matrix shares memory with the input matrix, no copying is performed, changes to the new matrix
 * will be made to the input matrix as well.
 * Assign to another matrix of compatible size if you need a copy.
 * @param m matrix to reshape
 * @param rows number of rows of the new matrix
 * @param cols number of cols of the new matrix
 * @returns matrix expression with the same entries as the input matrix but new shape
 */
template <typename M>
auto reshape(M&& m, Eigen::Index rows, Eigen::Index cols)
{
    assert(rows * cols == m.rows() * m.cols());
    assert(rows >= 1);
    assert(cols >= 1);

    return Eigen::Map<CVPlainMatrixT<std::remove_reference_t<M>>>(m.data(), rows, cols);
}

/**
 * template utility.
 * Defines value = true if M is an Eigen matrix expression.
 * Defines value = false, otherwise.
 */
template<class M>
using is_matrix_expression = std::is_base_of<Eigen::EigenBase<M>, M>;

/**
 * coefficient wise maximum of two matrices.
 * @param a a matrix expression
 * @param b a matrix expression of the same shape as a
 * @return a matrix expression the shape of a with each coefficient the maximum of the coefficients of a and b.
 */
template<class A, class B>
auto max(const Eigen::MatrixBase<A>& a, B&& b)
{
    return a.binaryExpr(std::forward<B>(b), [](auto a_i, auto b_i) { return std::max(a_i, b_i); });
}

/**
 * Maps a random access range (i.e. anything with size() and operator[], e.g. std::vector) onto a 
 * Eigen array expression. Returns a column array expression ´a´ where a[i] = f(v[i]).
 * The returned expression stores a reference to the range, lifetime of the range must exceed
 * lifetime of the return.
 * @param v a random access range.
 * @param f a function that returns a numeric scalar for each element of v. 
 * @return an array expression ´a´ the same size as v where a[i] = f(v[i]).
 */
template<class Rng, class F>
auto map(const Rng& v, F f)
{
    using Result = std::result_of_t<F(const typename Rng::value_type&)>;
    using Scalar = std::decay_t<Result>;
    using Array  = Eigen::Array<Scalar, Eigen::Dynamic, 1>;
    return Array::NullaryExpr(v.size(), [f, &v] (Eigen::Index i) -> Scalar { 
        return f(v[size_t(i)]);
    });
}

} // namespace epi