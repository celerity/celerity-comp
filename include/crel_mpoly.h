//
// A Class representing a multivariate polynomial
// Created by nmammeri on 04/09/2020.
//

#ifndef COST_RELATION_CREL_MPOLY_H
#define COST_RELATION_CREL_MPOLY_H

#include <cstdint>
#include <vector>
#include <string>
#include "flint/fmpq_mpoly.h"
#include "flint/fmpz.h"

using namespace std;

namespace celerity {

    const uint32_t MAX_MPOLY_DEGREE = 5;

    // TODO: change to use multivariate polynomials fmpz_mpoly_t
    class crel_mpoly {

    public:
        // Constructor
        crel_mpoly();
        explicit crel_mpoly(uint32_t numVars, uint32_t degree=MAX_MPOLY_DEGREE);
        ~crel_mpoly();

        // Print
        string toString(const std::vector<std::string>& varNames) const;
        string toString() const;

        // Set to given mpoly
        void set(const crel_mpoly& poly2);
        bool isEqual(const crel_mpoly& poly2);

        // is Constant
        bool isConstant();
        long getConstantNominator();
        long getConstantDenominator();


        // Set to constant
        void setConstant(uint32_t constant);
        void setConstant(int constant);
        void setConstant64bit(int64_t constant);

        // Addition
        void add(const uint32_t constant);
        void add(const int constant);
        void add(const crel_mpoly& poly2);
        void sub(const crel_mpoly& poly2);


        // Multiplication
        void multiply(const long constant);
        void multiply(const crel_mpoly& poly2);
        void divideby(const long constant);
        void divideby(const crel_mpoly& poly2);

        // Max join
        void maxjoin(const crel_mpoly& poly2);

        // Set var coeff
        void setVarCoeff(const uint32_t varIndex, const int coeff);
        void setVarCoeff(const uint32_t varIndex, const int coeff, const int denom);



    private:
        // fmpz_mpoly_ctx_t is a structure holding information about the number of variables and the term ordering of an multivariate polynomial.
        uint32_t num_vars = 1;
        fmpq_mpoly_ctx_t context{};
        fmpq_mpoly_t fmpq_mpoly{};

    };

}


#endif //COST_RELATION_CREL_MPOLY_H
