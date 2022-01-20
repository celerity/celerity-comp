#include <iostream>
using namespace std;

#include "KernelInvariant.hpp"
#include "IMPoly.hpp"

using namespace celerity;

/// Simple test application for IMPoly
int main(){
    cout << " * poly empty\n";
    IMPoly test1;
    cout << test1 << endl;

    cout << " * poly a0\n";
    IMPoly test2(10, KernelInvariant::enumerate(celerity::InvariantType::a0), 1);
    cout << test2 << endl;

    cout << " * poly gs0\n";
    IMPoly test3(5, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    cout << test3 << endl;

    test2 += test3;
    cout << " * poly a0+gs0\n";
    cout << test2  << endl;

    cout << " * poly a0gs0+gs0^2\n";
    test3 *= test2;
    cout << test3 << endl;

    cout << " * poly copy operator=\n";
    IMPoly test3_copy = test3;
    cout << test3_copy << endl;

    cout << " * negative poly\n";
    test2 = test2 - test2 - test2;
    cout << test2 << endl;

    cout << " * abs poly\n";
    cout << IMPoly::abs(test2);

    cout << " * poly gs0\n";
    IMPoly test4(70, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    //test4 *= test4;
    //test4 += test4;
    cout << test4 << endl;

    cout << " * max(poly2+poly3,poly4)\n";
    IMPoly test5 = IMPoly::max(test3, test4);  
    cout << "FINAL" << endl;
    cout << test5 << endl;

    return 0;
}
