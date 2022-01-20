#include <iostream>
using namespace std;

#include "KernelInvariant.hpp"
#include "IMPoly.hpp"

using namespace celerity;

/// Simple test application for IMPoly
int main(){
    IMPoly test1;
    cout << " * poly empty: " << test1 << endl;

    
    IMPoly test2(10, KernelInvariant::enumerate(celerity::InvariantType::a0), 1);
    cout << " * poly a0: " << test2 << endl;

    IMPoly test3(5, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    cout << " * poly gs0: " << test3 << endl;

    test2 += test3;
    cout << " * poly a0+gs0: " << test2  << endl;

    test3 *= test2;
    cout << " * poly a0gs0+gs0^2: " << test3 << endl;

    IMPoly test3_copy = test3;
    cout << " * poly copy operator= : " << test3_copy << endl;

    test2 = test1 - test2;
    cout << " * negative poly: " << test2 << endl;
    test2.abs();
    cout << " * abs poly: " << test2 << endl;

    IMPoly test4(70, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    cout << " * poly gs0:" << test4 << endl;

    cout << " * max("<< test3 << "," << test4 << ") = ";
    IMPoly test5 = IMPoly::max(test3, test4);  
    cout << test5 << endl;


    IMPoly test6(100, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    cout << " * max("<< test5 << "," << test6 << ") = ";
    IMPoly test7 = IMPoly::max(test5, test6);  
    cout << test7 << endl;

    return 0;
}
