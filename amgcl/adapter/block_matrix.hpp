#ifndef AMGCL_ADAPTER_BLOCK_MATRIX_HPP
#define AMGCL_ADAPTER_BLOCK_MATRIX_HPP

/*
The MIT License

Copyright (c) 2012-2015 Denis Demidov <dennis.demidov@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/**
\file    amgcl/adapter/block_matrix.hpp
\author  Denis Demidov <dennis.demidov@gmail.com>
\brief   On-the-fly conversion to block valued matrix.
\ingroup adapters
*/

#include <amgcl/util.hpp>
#include <amgcl/backend/detail/matrix_ops.hpp>

namespace amgcl {
namespace adapter {

template <class Matrix, int BlockSize, class BlockType>
struct block_matrix_adapter {
    typedef BlockType val_type;

    const Matrix &A;

    block_matrix_adapter(const Matrix &A) : A(A) {
        precondition(
                backend::rows(A) % BlockSize == 0 &&
                backend::cols(A) % BlockSize == 0,
                "Matrix size is not divisible by block size!"
                );
    }

    size_t rows() const {
        return backend::rows(A) / BlockSize;
    }

    size_t cols() const {
        return backend::cols(A) / BlockSize;
    }

    size_t nonzeros() const {
        // Just an estimate:
        return backend::nonzeros(A) / (BlockSize * BlockSize);
    }

    struct row_iterator {
        typedef typename backend::row_iterator<Matrix>::type Base;
        typedef ptrdiff_t col_type;
        typedef BlockType val_type;

        boost::array<boost::shared_ptr<Base>, BlockSize> base;

        row_iterator(const Matrix &A, col_type row) {
            for(int i = 0; i < BlockSize; ++i)
                base[i] = boost::make_shared<Base>(backend::row_begin(A, row * BlockSize + i));
        }

        operator bool() const {
            for(int i = 0; i < BlockSize; ++i)
                if (*base[i]) return true;
            return false;
        }

        row_iterator& operator++() {
            col_type cur_col = col();

            for(int i = 0; i < BlockSize; ++i) {
                while (*base[i] && base[i]->col() / BlockSize == cur_col) {
                    ++(*base[i]);
                }
            }

            return *this;
        }

        col_type col() const {
            col_type cur_col = 0;
            bool first = true;
            for(int i = 0; i < BlockSize; ++i) {
                if (*base[i]) {
                    if (first) {
                        cur_col = base[i]->col() / BlockSize;
                        first = false;
                    } else {
                        cur_col = std::min<col_type>(cur_col, base[i]->col() / BlockSize);
                    }
                }
            }
            return cur_col;
        }

        val_type value() const {
            col_type cur_col = col();
            val_type val = math::zero<val_type>();

            for(int i = 0; i < BlockSize; ++i) {
                for(Base a = *base[i]; a && a.col() / BlockSize == cur_col; ++a) {
                    val(i,a.col() % BlockSize) = a.value();
                }
            }

            return val;
        }
    };

    row_iterator row_begin(size_t i) const {
        return row_iterator(A, i);
    }
};

/// Convert scalar-valued matrix to a block-valued one.
template <int BlockSize, class BlockType, class Matrix>
block_matrix_adapter<Matrix, BlockSize, BlockType> block_matrix(const Matrix &A) {
    return block_matrix_adapter<Matrix, BlockSize, BlockType>(A);
}

} // namespace adapter

namespace backend {

//---------------------------------------------------------------------------
// Specialization of matrix interface
//---------------------------------------------------------------------------
template <class Matrix, int BlockSize, class BlockType>
struct value_type< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    typedef BlockType type;
};

template <class Matrix, int BlockSize, class BlockType>
struct rows_impl< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    static size_t get(const adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> &A) {
        return A.rows();
    }
};

template <class Matrix, int BlockSize, class BlockType>
struct cols_impl< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    static size_t get(const adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> &A) {
        return A.cols();
    }
};

template <class Matrix, int BlockSize, class BlockType>
struct nonzeros_impl< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    static size_t get(const adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> &A) {
        return A.nonzeros();
    }
};

template <class Matrix, int BlockSize, class BlockType>
struct row_iterator< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    typedef
        typename adapter::block_matrix_adapter<Matrix, BlockSize, BlockType>::row_iterator
        type;
};

template <class Matrix, int BlockSize, class BlockType>
struct row_begin_impl< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
{
    typedef adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> BM;
    static typename row_iterator<BM>::type
    get(const BM &matrix, size_t row) {
        return matrix.row_begin(row);
    }
};

namespace detail {

template <class Matrix, int BlockSize, class BlockType>
struct use_builtin_matrix_ops< adapter::block_matrix_adapter<Matrix, BlockSize, BlockType> >
    : boost::true_type
{};

} // namespace detail
} // namespace backend
} // namespace amgcl

#endif
