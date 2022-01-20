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

IMPoly::~IMPoly(){
    //cout << "dtor" << endl;
    //fmpz_mpoly_clear(mpoly, ctx);
    //fmpz_mpoly_ctx_clear(ctx);
}   

IMPoly& IMPoly::operator += (IMPoly const &rhs){
    fmpz_mpoly_add(mpoly, mpoly, rhs.mpoly, ctx);
    return *this;    
}

IMPoly& IMPoly::operator -= (IMPoly const &rhs){
    fmpz_mpoly_sub(mpoly, mpoly, rhs.mpoly, ctx);
    return *this;    
}

IMPoly& IMPoly::operator *= (IMPoly const &rhs){
    fmpz_mpoly_mul(mpoly, mpoly, rhs.mpoly, ctx);
    return *this; 
}

IMPoly& IMPoly::operator = (IMPoly const &rhs){
    cout << "Copy="<< endl;
    if (this != &rhs) {
        fmpz_mpoly_set(this->mpoly, rhs.mpoly, rhs.ctx);
    }
    return *this; 
}


IMPoly IMPoly::abs(IMPoly const &poly){
    IMPoly abs = poly;
    // for each term
    cout << "abs" << endl;
    for(unsigned i=0; i<fmpz_mpoly_length(abs.mpoly, abs.ctx); i++){
        fmpz_t coef, abs_coef;
        fmpz_mpoly_get_term_coeff_fmpz(coef, abs.mpoly, i, abs.ctx);
        fmpz_abs(abs_coef, coef);
        cout << "  " << i << ") coef " << coef << "->" << abs_coef << endl;
        fmpz_mpoly_set_term_coeff_fmpz(abs.mpoly, i, abs_coef, abs.ctx);
    }
}

IMPoly IMPoly::max(IMPoly const &poly1, IMPoly const &poly2){
    IMPoly res;
    /*
    cout << "poly1 terms" << endl;
    // for each term
    for(unsigned i=0; i<fmpz_mpoly_length(poly1.mpoly, poly1.ctx); i++){
        unsigned coeff = fmpz_mpoly_get_term_coeff_ui(poly1.mpoly, i, poly1.ctx);
        ulong exp[1];
        fmpz_mpoly_get_term_exp_ui(exp, poly1.mpoly, i, poly1.ctx);
        cout << "  " << i << ") coeff " << coeff  << " exp " << exp[1] << endl;
    }
    
    cout << "poly2 terms" << endl;
    for(unsigned i=0; i<fmpz_mpoly_length(poly2.mpoly, poly2.ctx); i++){
        unsigned coeff = fmpz_mpoly_get_term_coeff_ui(poly2.mpoly, i, poly2.ctx);
        ulong exp[1];
        fmpz_mpoly_get_term_exp_ui(exp, poly2.mpoly, i, poly2.ctx);
        cout << "  " << i << ") coeff " << coeff  << " exp " << exp[1] << endl;
    }
    */
    cout << "compute max" << endl;
    cout << "  a+b" << endl;
    IMPoly a_plus_b = poly1 + poly2;
    cout << "  a-b" << endl;
    IMPoly a_minus_b = poly1 + poly2;
    cout << "  |a-b|" << endl;
    IMPoly abs_amb = IMPoly::abs(a_minus_b);
    IMPoly result = a_plus_b - abs_amb;

    // divide by 2

    return result;
}

// Assignment operators
//IMPoly& IMPoly::operator=(IMPoly &poly){}

//IMPoly& IMPoly::operator=(unsigned constant){}   
// Output operators




std::string IMPoly::str() const{
    std::string pretty = std::string(fmpz_mpoly_get_str_pretty(mpoly, InvariantTypeName, ctx));
    return pretty;
}

