//
// Created by nmammeri on 04/09/2020.
//

#include "crel_mpoly.h"

using namespace celerity;

/**
 * Canonical representation of fmpz_mpoly.
 *   fmpz_mpoly = coeff1*term1 + coeff2*term2 + ....
 *   term1 = [3,2,1] : index of all variables => x^3+y^2+z
 * fmpz_mpoly is made of terms and each term is made of exponent vector of all variables.
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
    fmpz_mpoly_ctx_init(context, 1, ordering_t::ORD_DEGLEX);
    fmpz_mpoly_init(fmpz_mpoly, context);

    // Set degrees for variables to be  MAX_MPOLY_DEGREE
    slong degrees[] = {MAX_MPOLY_DEGREE};
    fmpz_mpoly_degrees_si(degrees, fmpz_mpoly, context);
}

crel_mpoly::crel_mpoly(const uint32_t numVars, const uint32_t degree) : num_vars(numVars) {
    fmpz_mpoly_ctx_init(context, numVars, ordering_t::ORD_DEGLEX);
    fmpz_mpoly_init(fmpz_mpoly, context);

    // Set degrees for variables to be MAX_MPOLY_DEGREE
    std::vector<slong> degrees(numVars); // make sure to reserve right amount of space
    for (auto item : degrees)
        degrees.push_back(MAX_MPOLY_DEGREE);

    fmpz_mpoly_degrees_si(&degrees[0], fmpz_mpoly, context);

}
std::string crel_mpoly::toString() const {
    return std::string(fmpz_mpoly_get_str_pretty(fmpz_mpoly, nullptr, context));
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

        result = std::string(fmpz_mpoly_get_str_pretty(fmpz_mpoly, runtime_vars_names.data(), context));
    } else {
        result = std::string(fmpz_mpoly_get_str_pretty(fmpz_mpoly, nullptr, context));
    }

    return result;
}

/**
 * Set to the passed poly
 * @param poly2
 */
void crel_mpoly::set(const crel_mpoly &poly2) {
    fmpz_mpoly_set(fmpz_mpoly, poly2.fmpz_mpoly, context);
}

/**
 * Returns if they are equal
 * @param poly2
 * @return
 */
bool crel_mpoly::isEqual(const crel_mpoly &poly2) {
    return (fmpz_mpoly_equal(fmpz_mpoly, poly2.fmpz_mpoly, context) == 1);
}

/**
 * Set poly to unisigned integer constant
 * @param constant
 */
void crel_mpoly::setConstant(const uint32_t constant) {
    fmpz_mpoly_set_ui(fmpz_mpoly, constant, context);
}
/**
 * Set poly to signed integer constant
 * @param constant
 */
void crel_mpoly::setConstant(const int constant) {
    fmpz_mpoly_set_si(fmpz_mpoly, constant, context);
}

void crel_mpoly::setConstant64bit(int64_t constant) {
    fmpz_mpoly_set_si(fmpz_mpoly, constant, context);
}


/**
 * Adds unisigned integer constant to our poly
 * @param constant
 */
void crel_mpoly::add( const uint32_t constant) {
    fmpz_mpoly_add_ui(fmpz_mpoly, fmpz_mpoly, constant, context);
}

/**
 * Adds signed integer constant to our poly
 * @param constant
 */
void crel_mpoly::add(const int constant) {
    fmpz_mpoly_add_si(fmpz_mpoly, fmpz_mpoly, constant, context);
}

/**
 * Add the poly2 to our poly
 * @param poly2
 */
void crel_mpoly::add(const crel_mpoly& poly2) {
    // TODO: Check if we need to check which context to use by comparing degrees
    fmpz_mpoly_add(fmpz_mpoly, fmpz_mpoly, poly2.fmpz_mpoly, context);
}


void crel_mpoly::multiply(const crel_mpoly& poly2) {
    fmpz_mpoly_mul(fmpz_mpoly, fmpz_mpoly, poly2.fmpz_mpoly, context);

}

void crel_mpoly::divideby(const crel_mpoly &poly2) {
    fmpz_mpoly_div_monagan_pearce(fmpz_mpoly, fmpz_mpoly, poly2.fmpz_mpoly, context);
}


void crel_mpoly::setVarCoeff(const uint32_t varIndex, const int coeff) {
    // prepare the exp vector
    std::vector<ulong> expVector(num_vars);
    for (uint32_t i=0; i<num_vars; i++)
        if (i == varIndex)
            expVector[i] = 1;
        else
            expVector[i] = 0;

    // Set the coefficient of the monomial with exponent vector exp to c.
    fmpz_mpoly_set_coeff_si_ui(fmpz_mpoly, coeff, &expVector[0], context);
}

void crel_mpoly::maxjoin(const crel_mpoly &poly2) {

    // Loop through the polynomial terms
    auto numTerms1 = fmpz_mpoly_length(fmpz_mpoly, context);
    auto numTerms2 = fmpz_mpoly_length(poly2.fmpz_mpoly, context);

    // We pick a term of poly2 and then get its index vector and check if we have it in result poly
    for (int j=0; j<numTerms2; j++) {
        std::vector<ulong> expVector2(num_vars);
        fmpz_mpoly_get_term_exp_ui(&expVector2[0], poly2.fmpz_mpoly, j, context);
        auto coeff2 = fmpz_mpoly_get_term_coeff_si(poly2.fmpz_mpoly, j, context);

        // Loop through all terms of poly1 and check if we have a match of the vector index
        bool term2NotFound = true;
        for (int i=0; i<numTerms1; i++) {

            std::vector<ulong> expVector1(num_vars);
            fmpz_mpoly_get_term_exp_ui(&expVector1[0], fmpz_mpoly, i, context);
            auto coeff1 = fmpz_mpoly_get_term_coeff_si(fmpz_mpoly, i, context);

            // check if the exp vectors are equal
            if (std::equal(expVector2.begin(), expVector2.begin() + num_vars, expVector1.begin())) {

                // update the coeffcient if coeff2 is > than coeff1
                if (coeff2 > coeff1) {
                    fmpz_mpoly_set_term_coeff_si(fmpz_mpoly, i, coeff2, context);
                }
                // Set that this term was found
                term2NotFound = false;
                break;
            }
        }

        // If we didn't find the term add it to the final result
        if (term2NotFound) {
            fmpz_mpoly_push_term_si_ui(fmpz_mpoly, coeff2, &expVector2[0], context);
        }
    }
}






