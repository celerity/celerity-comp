#pragma once

#include <string>
#include <ostream>

#include <flint/fmpz_mpoly.h>

namespace celerity
{

  const unsigned IMPOLY_MAX_DEGREE = 3;

  /// Multivariate Polinomial with variable based on invariants
  class IMPoly
  {
  private:
    fmpz_mpoly_ctx_t ctx;
    fmpz_mpoly_t mpoly;

    void initContext();

  public:
    /// Zero polynomial
    IMPoly(); 
    /// Polynominal initialized with a single term: <coeff> * x_invariant ^ exponent
    IMPoly(unsigned coeff, unsigned invariant, unsigned exponent = 1); 
    /// Dtor
    ~IMPoly();

    
    std::string str() const;
    //unsigned evaluate(int []) const;

    static IMPoly abs(IMPoly const &poly);
    static IMPoly max(IMPoly const &poly1, IMPoly const &poly2);

    IMPoly& operator+=(IMPoly const &poly);
    IMPoly& operator-=(IMPoly const &poly);
    IMPoly& operator*=(IMPoly const &poly);
    IMPoly& operator=(const IMPoly& other);


    friend IMPoly operator+(IMPoly lhs, const IMPoly& rhs){
      lhs += rhs; // reuse compound assignment
      return lhs; // return the result by value (uses move constructor)
    }

    friend IMPoly operator-(IMPoly lhs, const IMPoly& rhs){
      lhs -= rhs; 
      return lhs; 
    }

    
    friend std::ostream &operator<<(std::ostream &output, const IMPoly &rhs)
    {
      output << rhs.str();
      return output;
    }

  };

} // namespace
