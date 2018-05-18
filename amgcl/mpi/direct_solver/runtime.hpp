#ifndef AMGCL_MPI_DIRECT_SOLVER_RUNTIME_HPP
#define AMGCL_MPI_DIRECT_SOLVER_RUNTIME_HPP

/*
The MIT License

Copyright (c) 2012-2018 Denis Demidov <dennis.demidov@gmail.com>

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
 * \file   amgcl/mpi/direct_solver/runtime.hpp
 * \author Denis Demidov <dennis.demidov@gmail.com>
 * \brief  Runtime wrapper for distributed direct solvers.
 */

#include <amgcl/util.hpp>
#include <amgcl/mpi/direct_solver/skyline_lu.hpp>
#ifdef AMGCL_HAVE_EIGEN
#  include <amgcl/mpi/direct_solver/eigen_splu.hpp>
#endif
#ifdef AMGCL_HAVE_PASTIX
#  include <amgcl/mpi/direct_solver/pastix.hpp>
#endif

namespace amgcl {
namespace runtime {
namespace mpi {
namespace direct {

enum type {
    skyline_lu
#ifdef AMGCL_HAVE_EIGEN
  , eigen_splu
#endif
#ifdef AMGCL_HAVE_PASTIX
  , dpastix
  , spastix
#endif
};

std::ostream& operator<<(std::ostream &os, type s)
{
    switch (s) {
        case skyline_lu:
            return os << "skyline_lu";
#ifdef AMGCL_HAVE_EIGEN
        case eigen_splu:
            return os << "eigen_splu";
#endif
#ifdef AMGCL_HAVE_PASTIX
        case dpastix:
            return os << "dpastix";
        case spastix:
            return os << "spastix";
#endif
        default:
            return os << "???";
    }
}

std::istream& operator>>(std::istream &in, type &s)
{
    std::string val;
    in >> val;

    if (val == "skyline_lu")
        s = skyline_lu;
#ifdef AMGCL_HAVE_EIGEN
    else if (val == "eigen_splu")
        s = eigen_splu;
#endif
#ifdef AMGCL_HAVE_PASTIX
    else if (val == "dpastix")
        s = dpastix;
    else if (val == "spastix")
        s = spastix;
#endif
    else
        throw std::invalid_argument("Invalid direct solver value. Valid choices are: "
                "skyline_lu"
#ifdef AMGCL_HAVE_EIGEN
                ", eigen_splu"
#endif
#ifdef AMGCL_HAVE_PASTIX
                ", dpastix"
                ", spastix"
#endif
                ".");

    return in;
}

template <class value_type>
class solver {
    public:
        typedef boost::property_tree::ptree params;

        template <class Matrix>
        solver(MPI_Comm mpi_comm, const Matrix &A, params prm = params())
            : s(prm.get("type", skyline_lu))
        {
            if (!prm.erase("type")) AMGCL_PARAM_MISSING("type");

            switch (s) {
                case skyline_lu:
                    {
                        typedef amgcl::mpi::direct::skyline_lu<value_type> S;
                        handle = static_cast<void*>(new S(mpi_comm, A, prm));
                    }
                    break;
#ifdef AMGCL_HAVE_EIGEN
                case eigen_splu:
                    {
                        typedef amgcl::mpi::direct::eigen_splu<value_type> S;
                        handle = static_cast<void*>(new S(mpi_comm, A, prm));
                    }
                    break;
#endif
#ifdef AMGCL_HAVE_PASTIX
                case dpastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type,true> S;
                        handle = static_cast<void*>(new S(mpi_comm, A, prm));
                    }
                    break;
                case spastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type,false> S;
                        handle = static_cast<void*>(new S(mpi_comm, A, prm));
                    }
                    break;
#endif
                default:
                    throw std::invalid_argument("Unsupported direct solver type");
            }
        }

        template <class Vec1, class Vec2>
        void operator()(const Vec1 &rhs, Vec2 &x) const {
            switch (s) {
                case skyline_lu:
                    {
                        typedef amgcl::mpi::direct::skyline_lu<value_type> S;
                        static_cast<const S*>(handle)->operator()(rhs, x);
                    }
                    break;
#ifdef AMGCL_HAVE_EIGEN
                case eigen_splu:
                    {
                        typedef amgcl::mpi::direct::eigen_splu<value_type> S;
                        static_cast<const S*>(handle)->operator()(rhs, x);
                    }
                    break;
#endif
#ifdef AMGCL_HAVE_PASTIX
                case dpastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type, true> S;
                        static_cast<const S*>(handle)->operator()(rhs, x);
                    }
                    break;
                case spastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type, false> S;
                        static_cast<const S*>(handle)->operator()(rhs, x);
                    }
                    break;
#endif
                default:
                    throw std::invalid_argument("Unsupported direct solver type");
            }
        }

        ~solver() {
            switch (s) {
                case skyline_lu:
                    {
                        typedef amgcl::mpi::direct::skyline_lu<value_type> S;
                        delete static_cast<S*>(handle);
                    }
                    break;
#ifdef AMGCL_HAVE_EIGEN
                case eigen_splu:
                    {
                        typedef amgcl::mpi::direct::eigen_splu<value_type> S;
                        delete static_cast<S*>(handle);
                    }
                    break;
#endif
#ifdef AMGCL_HAVE_PASTIX
                case dpastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type, true> S;
                        delete static_cast<S*>(handle);
                    }
                    break;
                case spastix:
                    {
                        typedef amgcl::mpi::direct::pastix<value_type, false> S;
                        delete static_cast<S*>(handle);
                    }
                    break;
#endif
                default:
                    break;
            }
        }
    private:
        direct::type s;
        void *handle;
};

} // namespace direct
} // namespace mpi
} // namespace runtime
} // namespace amgcl

#endif