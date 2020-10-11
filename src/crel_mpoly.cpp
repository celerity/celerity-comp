//
// Created by nmammeri on 04/09/2020.
//

#include "crel_mpoly.h"

using namespace celerity;

/**
 * Canonical representation of fmpq_mpoly.
 *   fmpq_mpoly = coeff1*term1 + coeff2*term2 + ....
 *   term1 = [3,2,1] : index of all variables => x^3+y^2+z
 *   coeff is rational number = nominator/denominator
 * fmpq_mpoly is made of terms and each term is made of exponent vector of all variables.
 * The length of an exponent vector is the number of variables in the polynomial,
 * and the element at index 0 corresponds to the most significant variable.
 *
 */

// Destuctor
crel_mpoly::~crel_mpoly() {

}

// Constructors
crel_mpoly::crel_mpoly() {
    // Initialise to 1 variable
    fmpq_mpoly_ctx_init(context, 1, ordering_t::ORD_DEGLEX);
    fmpq_mpoly_init(fmpq_mpoly, context);

    // Set degrees for variables to be  MAX_MPOLY_DEGREE
    slong degrees[] = {MAX_MPOLY_DEGREE};
    fmpq_mpoly_degrees_si(degrees, fmpq_mpoly, context);
}

crel_mpoly::crel_mpoly(const uint32_t numVars, const uint32_t degree) : num_vars(numVars) {
    fmpq_mpoly_ctx_init(context, numVars, ordering_t::ORD_DEGLEX);
    fmpq_mpoly_init(fmpq_mpoly, context);

    // Set degrees for variables to be MAX_MPOLY_DEGREE
    std::vector<slong> degrees(numVars); // make sure to reserve right amount of space
    for (auto item : degrees)
        degrees.push_back(MAX_MPOLY_DEGREE);

    fmpq_mpoly_degrees_si(&degrees[0], fmpq_mpoly, context);

}
std::string crel_mpoly::toString() const {
    return std::string(fmpq_mpoly_get_str_pretty(fmpq_mpoly, nullptr, context));
}

/**
 * Returns a string representation of the mpoly using the passed vector of names for variables
 * @param varNames length must be
 * @return
 */
std::string crel_mpoly::toString(const vector<string>& varNames) const {

    string result;

    if (!varNames.empty()) {
        std::vector<const char*> runtime_vars_names{};
        for(const auto& string : varNames)
            runtime_vars_names.push_back(string.c_str());

        result = std::string(fmpq_mpoly_get_str_pretty(fmpq_mpoly, runtime_vars_names.data(), context));
    } else {
        result = std::string(fmpq_mpoly_get_str_pretty(fmpq_mpoly, nullptr, context));
    }

    return result;
}

/**
 * Set to the passed poly
 * @param poly2
 */
void crel_mpoly::set(const crel_mpoly &poly2) {
    fmpq_mpoly_set(fmpq_mpoly, poly2.fmpq_mpoly, context);
}

/**
 * Returns if they are equal
 * @param poly2
 * @return
 */
bool crel_mpoly::isEqual(const crel_mpoly &poly2) {
    return (fmpq_mpoly_equal(fmpq_mpoly, poly2.fmpq_mpoly, context) == 1);
}

/**
 * Set poly to unisigned integer constant
 * @param constant
 */
void crel_mpoly::setConstant(const uint32_t constant) {
    fmpq_mpoly_set_ui(fmpq_mpoly, constant, context);
}
/**
 * Set poly to signed integer constant
 * @param constant
 */
void crel_mpoly::setConstant(const int constant) {
    fmpq_mpoly_set_si(fmpq_mpoly, constant, context);
}

void crel_mpoly::setConstant64bit(int64_t constant) {
    fmpq_mpoly_set_si(fmpq_mpoly, constant, context);
}


/**
 * Adds unisigned integer constant to our poly
 * @param constant
 */
void crel_mpoly::add( const uint32_t constant) {
    fmpq_mpoly_add_ui(fmpq_mpoly, fmpq_mpoly, constant, context);
}

/**
 * Adds signed integer constant to our poly
 * @param constant
 */
void crel_mpoly::add(const int constant) {
    fmpq_mpoly_add_si(fmpq_mpoly, fmpq_mpoly, constant, context);
}

/**
 * Add the poly2 to our poly
 * @param poly2
 */
void crel_mpoly::add(const crel_mpoly& poly2) {
    fmpq_mpoly_add(fmpq_mpoly, fmpq_mpoly, poly2.fmpq_mpoly, context);
}

void crel_mpoly::sub(const crel_mpoly &poly2) {
    fmpq_mpoly_sub(fmpq_mpoly, fmpq_mpoly, poly2.fmpq_mpoly, context);
}


void crel_mpoly::multiply(const crel_mpoly& poly2) {
    fmpq_mpoly_mul(fmpq_mpoly, fmpq_mpoly, poly2.fmpq_mpoly, context);

}

void crel_mpoly::multiply(const long constant) {
    fmpq_mpoly_scalar_mul_si(fmpq_mpoly, fmpq_mpoly, constant, context);
}

void crel_mpoly::divideby(const long constant) {
    fmpq_mpoly_scalar_div_si(fmpq_mpoly, fmpq_mpoly, constant, context);

}

void crel_mpoly::divideby(const crel_mpoly &poly2) {

    bool isConstant = fmpq_mpoly_is_fmpq(poly2.fmpq_mpoly, context);

    if (isConstant) {
        // GEt the constant
        fmpq_t constant;
        fmpq_mpoly_get_fmpq(constant, poly2.fmpq_mpoly, context);

        fmpq_mpoly_scalar_div_fmpq(fmpq_mpoly, fmpq_mpoly, constant, context);

    } else {
        // Check if the division is not exact. 3X2 / x2 isExact. 3x2 / x1 is not
        fmpq_mpoly_t fmpq_mpoly_result{}; fmpq_mpoly_init(fmpq_mpoly_result, context);
        int isExact = fmpq_mpoly_divides(fmpq_mpoly_result, fmpq_mpoly, poly2.fmpq_mpoly, context);

        if (isExact>0) {
            // Set the result to this polynomial
            fmpq_mpoly_set(fmpq_mpoly, fmpq_mpoly_result, context);

        } else {

            // Here we are sure that the qotient will be 0and the remainder will be fmpq_mpoly.
            // Because flintlib support only integer polynomials this is a limitation
            // we just don't do anything let the result equals to fmpq_mpoly
        }
    }

}

void crel_mpoly::setVarCoeff(const uint32_t varIndex, const int nominator) {
    setVarCoeff(varIndex, nominator, 1);
}

void crel_mpoly::setVarCoeff(const uint32_t varIndex, const int nominator, const int denominator) {

    // Create fmpq rational coefficient
    fmpq_t coefficient;
    fmpq_set_si(coefficient, nominator, denominator);

    // prepare the exp vector
    std::vector<ulong> expVector(num_vars);
    for (uint32_t i=0; i<num_vars; i++)
        if (i == varIndex)
            expVector[i] = 1;
        else
            expVector[i] = 0;

    // Set the coefficient of the monomial with exponent vector exp to c.
    fmpq_mpoly_set_coeff_fmpq_ui(fmpq_mpoly, coefficient, &expVector[0], context);

}

void crel_mpoly::maxjoin(const crel_mpoly &poly2) {

    // Loop through the polynomial terms
    auto numTerms1 = fmpq_mpoly_length(fmpq_mpoly, context);
    auto numTerms2 = fmpq_mpoly_length(poly2.fmpq_mpoly, context);

    // We pick a term of poly2 and then get its index vector and check if we have it in result poly
    for (int j=0; j<numTerms2; j++) {
        std::vector<ulong> expVector2(num_vars);
        fmpq_mpoly_get_term_exp_ui(&expVector2[0], poly2.fmpq_mpoly, j, context);
        fmpq_t coeff2;
        fmpq_mpoly_get_term_coeff_fmpq(coeff2, poly2.fmpq_mpoly, j, context);

        // Loop through all terms of poly1 and check if we have a match of the vector index
        bool term2NotFound = true;
        int term2ExpIndex = j;
        for (int i=0; i<numTerms1; i++) {

            std::vector<ulong> expVector1(num_vars);
            fmpq_mpoly_get_term_exp_ui(&expVector1[0], fmpq_mpoly, i, context);
            fmpq_t coeff1;
            fmpq_mpoly_get_term_coeff_fmpq(coeff1, fmpq_mpoly, i, context);

            // check if the exp vectors are equal
            if (std::equal(expVector2.begin(), expVector2.begin() + num_vars, expVector1.begin())) {

                // update the coeffcient if coeff2 is > than coeff1
                if (fmpq_cmp(coeff2, coeff1) > 0) {
                    fmpq_mpoly_set_term_coeff_fmpq(fmpq_mpoly, i, coeff2, context);
                }
                // Set that this term was found
                term2NotFound = false;
                break;
            }
        }

        // If we didn't find the term add it to the final result
        if (term2NotFound) {
            //std::vector<ulong> expVector1(num_vars);
            //fmpq_mpoly_get_term_exp_ui(&expVector1[0], fmpq_mpoly, j, context);
            fmpq_mpoly_push_term_fmpq_ui(fmpq_mpoly, coeff2, &expVector2[0], context);
        }
    }
}

bool crel_mpoly::isConstant() {
    return fmpq_mpoly_is_fmpq(fmpq_mpoly, context);
}

long crel_mpoly::getConstantNominator() {
    // Get the constant
    fmpq_t constant;
    fmpq_mpoly_get_fmpq(constant, fmpq_mpoly, context);

    return constant->num;
}

long crel_mpoly::getConstantDenominator() {
    // Get the constant
    fmpq_t constant;
    fmpq_mpoly_get_fmpq(constant, fmpq_mpoly, context);

    return constant->den;
}











