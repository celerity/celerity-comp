#include <string>
#include <iostream>
#include <sstream>
using namespace std;

#include "IMPoly.hpp"
#include "KernelInvariant.hpp"
using namespace celerity;


void IMPoly::initContext(){   
    //static bool first = true;
    //static fmpz_mpoly_ctx_t static_ctx;
    //ctx = static_ctx;
    //if(first){
    fmpz_mpoly_ctx_init(ctx, KernelInvariant::numInvariantType(), ordering_t::ORD_DEGLEX);
    //    first = false;
    //}    
}


IMPoly::IMPoly(){   
    //cout << "IMPoly()" << endl;
    initContext();
    fmpz_mpoly_init(mpoly, ctx);
    fmpz_mpoly_zero(mpoly, ctx);
}

IMPoly::IMPoly(unsigned coeff, unsigned invariant, unsigned exponent){
    //cout << "IMPoly(invariant,value)" << endl;
    initContext();
    fmpz_mpoly_init(mpoly, ctx);    
    stringstream pretty;
    pretty << coeff << "*x" << invariant << "^" << exponent;
    fmpz_mpoly_set_str_pretty(mpoly, pretty.str().c_str(), NULL, ctx);
    //cout << pretty.str() << endl;
}

IMPoly::IMPoly(const IMPoly &copy){
    //cout << "IMPoly(IMPoly) copy ctor" << endl;
    initContext();
    fmpz_mpoly_init(mpoly, ctx);  
    fmpz_mpoly_set(mpoly, copy.mpoly, copy.ctx);
}

IMPoly::~IMPoly(){
    //cout << "dtor" << endl;
    //fmpz_mpoly_clear(mpoly, ctx);
    //fmpz_mpoly_ctx_clear(ctx);
}   

IMPoly& IMPoly::operator += (const IMPoly &rhs){
    fmpz_mpoly_add(mpoly, mpoly, rhs.mpoly, ctx);
    return *this;    
}

IMPoly& IMPoly::operator -= (const IMPoly &rhs){
    fmpz_mpoly_sub(mpoly, mpoly, rhs.mpoly, ctx);
    return *this;    
}

IMPoly& IMPoly::operator *= (const IMPoly &rhs){
    fmpz_mpoly_mul(mpoly, mpoly, rhs.mpoly, ctx);
    return *this; 
}

IMPoly& IMPoly::operator = (const IMPoly &rhs){
    //cout << "operator="<< endl;
    if (this != &rhs) {
        fmpz_mpoly_set(this->mpoly, rhs.mpoly, rhs.ctx);
    }
    return *this; 
}


void IMPoly::abs(){
    //cout << "abs" << endl;
    for(unsigned i=0; i<fmpz_mpoly_length(mpoly, ctx); i++){ // for each term
        fmpz_t coef, abs_coef;
        fmpz_mpoly_get_term_coeff_fmpz(coef, mpoly, i, ctx);
        fmpz_abs(abs_coef, coef);
        //cout << "  " << i << ") coef " << coef[0] << "=>" << abs_coef[0] << endl;
        fmpz_mpoly_set_term_coeff_fmpz(mpoly, i, abs_coef, ctx);
    }
}

void IMPoly::divide_by_two(){
    //cout << "divide_by_two" << endl;
    for(unsigned i=0; i<fmpz_mpoly_length(mpoly, ctx); i++){
        ulong coef = fmpz_mpoly_get_term_coeff_ui(mpoly,i, ctx);
        ulong res = coef / 2;        
        //cout << "  " << i << ") coef " << coef << "=>" << res << endl;
        fmpz_mpoly_set_term_coeff_ui(mpoly, i, res, ctx);
    }
}

IMPoly IMPoly::max(const IMPoly &poly1, const IMPoly &poly2){
    // implementaiton not effiicnet, but avoid by term checks
    // a+b+|a-b| / 2
    IMPoly a_plus_b = poly1 + poly2;
    //cout << "    a+b : " << a_plus_b << endl;
    IMPoly a_minus_b = poly1 - poly2;
    //cout << "    a-b : " << a_minus_b << endl;
    a_minus_b.abs();
    //cout << "   |a-b|:"<< a_minus_b << endl;    
    IMPoly result = a_plus_b + a_minus_b;
    //cout << "    sum :"<< result << endl;
    result.divide_by_two();
    //cout << "    res :"<< result << endl;
    return result;
}

std::string IMPoly::str() const{
    std::string pretty = std::string(fmpz_mpoly_get_str_pretty(mpoly, InvariantTypeName, ctx));
    return pretty;
}

