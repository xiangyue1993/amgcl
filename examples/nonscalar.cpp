#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/SparseExtra>

#include <amgcl/amgcl.hpp>
#include <amgcl/make_solver.hpp>

#include <amgcl/backend/builtin.hpp>
#include <amgcl/backend/eigen.hpp>
#include <amgcl/adapter/crs_tuple.hpp>
#include <amgcl/adapter/block_matrix.hpp>
#include <amgcl/value_type/eigen.hpp>
#include <amgcl/value_type/static_matrix.hpp>

#include <amgcl/coarsening/smoothed_aggregation.hpp>
#include <amgcl/relaxation/gauss_seidel.hpp>
#include <amgcl/solver/bicgstabl.hpp>

#include <amgcl/profiler.hpp>

namespace amgcl {
    profiler<> prof;
}

template <int B>
void solve(const std::string &matrix_file, const std::string &rhs_file) {
#if 1
    // Use Eigen static matrices for value types.
    typedef Eigen::Matrix<double, B, B> value_type;
    typedef Eigen::Matrix<double, B, 1> rhs_type;
#else
    // Use builtin static matrices for value types.
    typedef amgcl::static_matrix<double, B, B> value_type;
    typedef amgcl::static_matrix<double, B, 1> rhs_type;
#endif

    typedef Eigen::SparseMatrix<double, Eigen::RowMajor, int> EigenMatrix;
    typedef EigenMatrix::InnerIterator row_iterator;

    using amgcl::precondition;
    using amgcl::prof;

    prof.tic("read problem");
    // Read scalar matrix
    EigenMatrix A;
    precondition(Eigen::loadMarket(A, matrix_file),
            "Failed to load matrix file (" + matrix_file + ")");
    precondition(A.rows() % B == 0,
            "System size is not divisible by block size");

    const int n = A.rows() / B;

    // Read RHS (if any).
    std::vector<rhs_type> rhs(n, amgcl::math::constant<rhs_type>(1.0));

    if (!rhs_file.empty()) {
        Eigen::Matrix<double, Eigen::Dynamic, 1> b;
        precondition(Eigen::loadMarketVector(b, rhs_file),
                "Failed to load RHS file (" + rhs_file + ")");

        for(int ip = 0, ia = 0; ip < n; ++ip) {
            for(int k = 0; k < B; ++k, ++ia) {
                rhs[ip](k) = b(ia);
            }
        }
    }
    prof.toc("read problem");

    typedef amgcl::backend::builtin<value_type> Backend;

    boost::property_tree::ptree prm;
    prm.put("solver.maxiter", 1000);

    prof.tic("setup");
    amgcl::make_solver<
        amgcl::amg<
            Backend,
            amgcl::coarsening::smoothed_aggregation,
            amgcl::relaxation::gauss_seidel
            >,
        amgcl::solver::bicgstabl<Backend>
        > solve(amgcl::adapter::block_matrix<B, value_type>(A), prm);
    prof.toc("setup");

    std::cout << solve.precond() << std::endl;

    std::vector<rhs_type> x(n, amgcl::math::zero<rhs_type>());

    int    iters;
    double resid;

    prof.tic("solve");
    boost::tie(iters, resid) = solve(rhs, x);
    prof.toc("solve");

    std::cout << "Iterations: " << iters << std::endl
              << "Error:      " << resid << std::endl
              ;

    std::cout << prof << std::endl;
}

int main(int argc, char *argv[]) {
    int block_size = 3;
    std::string matrix_file;
    std::string rhs_file;

    namespace po = boost::program_options;
    po::options_description desc("Options");

    desc.add_options()
        ("help,h", "show help")
        (
         "block_size,b",
         po::value<int>(&block_size)->default_value(block_size)->required(),
         "Block size. Supported values: 2-6"
        )
        (
         "matrix,A",
         po::value<std::string>(&matrix_file)->required(),
         "The system matrix in MatrixMarket format"
        )
        (
         "rhs,f",
         po::value<std::string>(&rhs_file),
         "The right-hand side in MatrixMarket format"
        )
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    po::notify(vm);

    switch (block_size) {
        case 2:
            solve<2>(matrix_file, rhs_file);
            break;
        case 3:
            solve<3>(matrix_file, rhs_file);
            break;
        case 4:
            solve<4>(matrix_file, rhs_file);
            break;
        case 5:
            solve<5>(matrix_file, rhs_file);
            break;
        case 6:
            solve<6>(matrix_file, rhs_file);
            break;
        default:
            std::cerr << "Unsupported block size" << std::endl;
            return 1;
    }
}