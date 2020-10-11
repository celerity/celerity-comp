#include <unordered_map>
#include <sstream>

#include "crel_feature_pass.h"
#include "crel_llvm_helper.h"

#include <llvm/IR/Instructions.h>


using namespace celerity;
using namespace llvm;
using namespace std;

bool debug = false;

// Initialization of a static member
char crel_feature_pass::ID = 0;
char poly_crel_pass::ID = 1;

// Pass registration.
// Old-style pass registration for <opt> (registering dynamically loaded passes).
static RegisterPass<crel_feature_pass> feature_eval_pass("poly-feature-pass", "Poly Feature evaluation");
static RegisterPass<poly_crel_pass> cr_eval_pass("poly-crel-pass", "Poly Cost relation feature evaluation");


// -------------------------------------------------------------------------
// mpoly_feature_pass
// -------------------------------------------------------------------------

void crel_feature_pass::eval_BB(BasicBlock &bb) {
    for (Instruction &i : bb) {
        featureSet->eval(i, 1);
    }
}

void crel_feature_pass::eval_function(Function &func) {

    // Check if we don't have an entry of this kernel in our map
    // If not create the map entry by calling the kernerl explicit constructor
    if (featureSet->kernels.find(func.getGlobalIdentifier()) == featureSet->kernels.end()) {
        featureSet->kernels[func.getGlobalIdentifier()] = crel_kernel(&func);
    }

    // Basic implementation: evaluate all function blocks
    for (llvm::BasicBlock &bb : func)
        eval_BB(bb);
}

void crel_feature_pass::finalize() {
    // no normalisation
}

bool crel_feature_pass::runOnModule(Module &m) {

    // Iterate through all kernel-functions of this module
    for (auto & func : m) {
        eval_function(func);
        finalize();
    }
    return false;
}


// -------------------------------------------------------------------------
// poly_crel_pass
// -------------------------------------------------------------------------

void poly_crel_pass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
}

/*
 * Runs on kernel functions only. Assumes that the all functions not representing kernels are inlined.
 *  Implements cost relation extraction and requires that the LoopInfoWrapperPass pass calculates the loop information,
 *  thus it should be ran before of this pass.
 *
 * Current limitations:
 *  -
 */
void poly_crel_pass::eval_function(Function &func) {

    auto name = func.getGlobalIdentifier();

    // Run only on kernels
    // We assume that always-inline pass is run and all non-kernel functions are inlined inside kernel functions.
    // We found out that definitions for non-kernel functions are kept inside the bitcode/ll but they are
    // all inlined.
    // We emit a warning when a function call to non-kernel function is performed. (check eval instruction implementation)
    if (!is_cl_kernel_function(func)) {
        return;
    } else {
        if (debug) cout << "processing kernel function: " << name << "\n";
    }

    // Create the kernel object
    crel_kernel kernel(&func);

    // Check if we don't have an entry of this kernel in our map
    // If not create the map entry by calling the kernerl explicit constructor
    if (featureSet->kernels.find(name) == featureSet->kernels.end()) {
        featureSet->kernels[name] = kernel;
    }


    // 1. for each BB, we initialize it's the mpoly for each feature to 0
    for (BasicBlock &bb : func.getBasicBlockList()) {
        // Initialise the features polynomials
        for (const auto& feat_name : featureSet->feat_names) {
            featureSet->kernels[name].bbFeatures[&bb][feat_name] = crel_mpoly(kernel.runtime_vars.size());
        }
        // Go through all instructions of thi BasicBlock
        for (Instruction &inst : bb) {
            featureSet->eval(inst, 1);
        }
    }

    // 2. We identify all loops and for each loop, we initialize it's the mpoly multiplier to 1
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(func).getLoopInfo();
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(func).getSE();

    for (const Loop *toplevelLoop : LI) {
        // This call gets all nested loops within this toplevel loop
        for (const Loop *nestedLoop : toplevelLoop->getLoopsInPreorder()) {

            // We support a maximum depth of 5 nested loops. Constraint imposed by the degree of the polynomials
            // Generate warning error if loop depth is larger than 5
            // toplevel depth = 1
            //      nestedlopp depth = 2 etc...
            if (nestedLoop->getLoopDepth() > 5) {
                std::cerr << "WARNINIG: found a loop nest depth larger > 5 " << endl;
            }

            // Get loop bounds
            auto loopTripCount = SE.getSmallConstantTripCount(nestedLoop);

            // 1. if loop bound is a constant
            if (loopTripCount != 0) {
                // initialise this loop multiplier to this constant
                crel_mpoly newPoly(kernel.runtime_vars.size());
                newPoly.setConstant(loopTripCount);
                featureSet->kernels[name].loopMultipliers[nestedLoop] = newPoly;

            } else {
                // Get the backedge taken count SCEV
                auto backedgeCountExpr = SE.getBackedgeTakenCount(nestedLoop);

                // Recursively evaluate the SCEV to create the multi polynomial
                crel_mpoly newPoly(kernel.runtime_vars.size());
                featureSet->kernels[name].loopMultipliers[nestedLoop] = evaluateSCEV(SE, kernel, *nestedLoop, newPoly, backedgeCountExpr);
            }
        }
    }

    // 3. Propagate all loop multipliers to their basic blocks
    for (const auto& entry : featureSet->kernels[name].loopMultipliers) {
        const Loop *loop = entry.first;
        const crel_mpoly multiplier = entry.second;

        for (const BasicBlock *bb : loop->getBlocks()) {
            // Make sure that the loop block has been initialised
            if (featureSet->kernels[name].bbFeatures.count(bb) > 0) {

                // Apply the multiplier on all feature mpolys
                for (const auto& feat_name : featureSet->feat_names) {
                    featureSet->kernels[name].bbFeatures[bb][feat_name].multiply(multiplier);
                }

            } else {
                cerr << "ERROR: loop block non-initialised" << endl;
            }
        }
    }

    // 4. Backward data-flow feature evaluation for the whole kernel

    // We run data flow algorithm for each feature.
    for (const auto& feat_name : featureSet->feat_names) {

        // 4.1 Initialise input and output sets for each basic block
        bool inputSetHasChanged = true;
        std::unordered_map<const llvm::BasicBlock*, crel_mpoly> inputSet;
        std::unordered_map<const llvm::BasicBlock*, crel_mpoly> outputSet;

        for (const BasicBlock &bb : func.getBasicBlockList()) {
            // Create mpoly= 0
            inputSet[&bb]  = crel_mpoly(kernel.runtime_vars.size());
            outputSet[&bb] = crel_mpoly(kernel.runtime_vars.size());
            // Initialise inputSet[&bb] mpoly to the previously calculated BB mpoly
            inputSet[&bb].set(featureSet->kernels[name].bbFeatures[&bb][feat_name]);
        }

        // 4.2 Backward data-flow
        while (inputSetHasChanged) {

            // On every iteration initialise inputSetHasChanged to false.
            // This should be set whenever we update the Input of any block
            inputSetHasChanged = false;
            // We loop through blocks found in this function
            for (const BasicBlock &bb : func.getBasicBlockList()) {

                // Ignore the exit blocks that don't have successors
                if (!llvm::successors(&bb).empty()) {

                    // Join operation for all sucessor blocks
                    for (const BasicBlock *succ : llvm::successors(&bb)) {
                        outputSet[&bb].maxjoin(inputSet[succ]);
                    }

                    // Check if the polynomial has changed before assigning new output to input
                    if (!inputSet[&bb].isEqual(outputSet[&bb])) {
                        inputSetHasChanged = true;
                    }

                    // Propagate output to input by assigning  inputSet[&bb] = outputSet[&bb]
                    inputSet[&bb].set(outputSet[&bb]);
                }
            }
        }

        // The final mpoly of this featue is the inputSet[&bb] of the basic block
        featureSet->kernels[name].features[feat_name] = inputSet[&func.getEntryBlock()];

    }



    // Debug printing
    if (debug) {
        for (const auto &entry : featureSet->kernels[name].loopMultipliers) {
            const Loop *loop = entry.first;
            const crel_mpoly poly = entry.second;
            auto backedgeExpr = SE.getBackedgeTakenCount(loop);
            //SE.getMinusSCEV()

            cout << "    loop " << loop->getLoopID() <<
                 " tripCount: " << SE.getSmallConstantTripCount(loop) <<
                 " mpoly: " << poly.toString(kernel.getVarNames()) <<
                 " subloops: " << loop->getSubLoops().size() <<
                 " depth: " << loop->getLoopDepth() <<
                 " backedgeExpr: " << backedgeExpr <<
                 " backedge SCEV type: " << backedgeExpr->getExpressionSize() <<
                 " isCanonical: " << loop->isCanonical(SE) <<
                 " isLoopSimplifyForm: " << loop->isLoopSimplifyForm() <<
                 " " << endl;
        }
        //cout << endl;

        for (const BasicBlock &bb : func.getBasicBlockList()) {
            for (const auto &feat_name : featureSet->feat_names) {
                cout << "   BB " << &bb <<
                     " feature: " << feat_name <<
                     " mpoly: " << featureSet->kernels[name].bbFeatures[&bb][feat_name].toString(kernel.getVarNames())
                     <<
                     " " << endl;
            }
            cout << endl;
        }
    }

}

/**
 * This creates a scev
 * @param value
 * @return
 */
crel_mpoly poly_crel_pass::evaluateValue(const crel_kernel &kernel, llvm::Value *scevValue) {

    // Check which variable is dependent on this passed variable
    for (uint32_t i=0; i<kernel.runtime_vars.size(); i++) {
        auto var = kernel.runtime_vars[i];

        if (scevValue == var.value) {
            crel_mpoly varPoly(kernel.runtime_vars.size());
            varPoly.setVarCoeff(i, 1);
            return varPoly;
        }
    }

    // If the variable was not found in kernel runtime variables -> means that it is an intermediate one
    // that is dependent on one of the kernel runtime variables
    // This case is handeled by just recursively evaluating the Value of this variable

    auto scevInstruction = dyn_cast<llvm::Instruction>(scevValue);

    if (const auto *constExpr = dyn_cast<llvm::ConstantInt>(scevValue) ) {

        long constant;
        if (constExpr->getType()->isSingleValueType()) {
            constant = constExpr->getSExtValue();
        } else {
            constant = constExpr->getZExtValue();
        }

        crel_mpoly constPoly(kernel.runtime_vars.size());
        constPoly.setConstant64bit(constant);
        return constPoly;

    } else if (auto *addExpr = dyn_cast<llvm::AddOperator>(scevInstruction)) {
        crel_mpoly addPoly(kernel.runtime_vars.size());
        addPoly.set(evaluateValue(kernel, addExpr->getOperand(0)));
        addPoly.add(evaluateValue(kernel, addExpr->getOperand(1)));
        return addPoly;

    } else if (auto *subExpr = dyn_cast<llvm::SubOperator>(scevInstruction)) {
        crel_mpoly subPoly(kernel.runtime_vars.size());
        subPoly.set(evaluateValue(kernel, subExpr->getOperand(0)));
        subPoly.sub(evaluateValue(kernel, subExpr->getOperand(1)));
        return subPoly;

    } else if (auto *divExpr = dyn_cast<llvm::SDivOperator>(scevInstruction)) {
        crel_mpoly divPoly(kernel.runtime_vars.size());
        divPoly.set(evaluateValue(kernel, divExpr->getOperand(0)));
        divPoly.divideby(evaluateValue(kernel, divExpr->getOperand(1)));
        return divPoly;

    } else if (auto *divExpr = dyn_cast<llvm::UDivOperator>(scevInstruction)) {
        crel_mpoly divPoly(kernel.runtime_vars.size());
        divPoly.set(evaluateValue(kernel, divExpr->getOperand(0)));
        divPoly.divideby(evaluateValue(kernel, divExpr->getOperand(1)));
        return divPoly;

    } else if (auto *shlExpr = dyn_cast<llvm::ShlOperator>(scevInstruction)) {

        // We first check is this value is constant otherwise we do not support this shift
        auto polyOp2 = evaluateValue(kernel, shlExpr->getOperand(1));

        if (polyOp2.isConstant()) {
            auto polyOp1 = evaluateValue(kernel, shlExpr->getOperand(0));

            // Get the value for this shift
            ulong op2 = (ulong) polyOp2.getConstantNominator();
            ulong op2Value = 1 << op2;

            // Return the corresponding mpoly
            crel_mpoly shlPoly(kernel.runtime_vars.size());
            shlPoly.set(polyOp1);
            shlPoly.multiply(op2Value);
            return shlPoly;
        }
    } else if (auto *lshrExpr = dyn_cast<llvm::LShrOperator>(scevInstruction)) {

        // We first check is this value is constant otherwise we do not support this shift
        auto polyOp2 = evaluateValue(kernel, lshrExpr->getOperand(1));

        if (polyOp2.isConstant()) {
            auto polyOp1 = evaluateValue(kernel, lshrExpr->getOperand(0));

            // Get the value for this shift
            ulong op2 = (ulong) polyOp2.getConstantNominator();
            ulong op2Value = 1 << op2;

            // Return the corresponding mpoly
            crel_mpoly lshrPoly(kernel.runtime_vars.size());
            lshrPoly.set(polyOp1);
            lshrPoly.divideby(op2Value);
            return lshrPoly;
        }

    } else if ( scevInstruction->isBitwiseLogicOp()) {

        auto *bitwiseExpr = dyn_cast<llvm::BinaryOperator>(scevInstruction);
        auto opCodeName = scevInstruction->getOpcodeName();

        if (string("and").compare(opCodeName) == 0) {
            auto polyOp1 = evaluateValue(kernel, bitwiseExpr->getOperand(0));
            auto polyOp2 = evaluateValue(kernel, bitwiseExpr->getOperand(1));

            if (polyOp1.isConstant() && polyOp2.isConstant()) {
                // Bitwise AND
                auto result = polyOp1.getConstantNominator() & polyOp2.getConstantNominator();

                crel_mpoly andPoly(kernel.runtime_vars.size());
                andPoly.setConstant64bit(result);
                return andPoly;
            }

        } else if (string("xor").compare(opCodeName) == 0) {
            auto polyOp1 = evaluateValue(kernel, bitwiseExpr->getOperand(0));
            auto polyOp2 = evaluateValue(kernel, bitwiseExpr->getOperand(1));
            auto poly1String = polyOp1.toString();
            auto poly2String = polyOp2.toString();

            if (polyOp1.isConstant() && polyOp2.isConstant()) {
                // Bitwise AND
                auto result = polyOp1.getConstantNominator() ^ polyOp2.getConstantNominator();

                crel_mpoly andPoly(kernel.runtime_vars.size());
                andPoly.setConstant64bit(result);
                return andPoly;
            }
        }

    }
//    else if (scevInstruction->getOpcode()==Instruction::PHI) {
//
//        auto *phiExpr = dyn_cast<llvm::PHINode>(scevInstruction);
//
//        crel_mpoly phiPoly(kernel.runtime_vars.size());
//
//        int num = phiExpr->getNumIncomingValues();
//
//        for ( int i=0; i<phiExpr->getNumIncomingValues(); i++) {
//            auto polyOp1 = evaluateValue(kernel, phiExpr->getIncomingValue(i));
//            auto poly1String = polyOp1.toString();
//            phiPoly.maxjoin(polyOp1);
//        }
//        return phiPoly;
//    }

    // If we reach here, means that SCEV variable was not part of the kernel variables
    cerr << "ERROR: in kernel: "<< kernel.name << " encountered a SCEV that uses an un-supoorted variable: ";
    scevValue->print(llvm::errs());
    cerr << endl;

    // If we fail to evalute return a constant
    crel_mpoly constPoly(kernel.runtime_vars.size());
    constPoly.setConstant(1);
    return constPoly;

}

/**
 * This class uses recursion to generate the multivariate polynomial from SCEV the scalar evolution expression.
 * @param scev
 * @param poly
 * @return
 */
crel_mpoly poly_crel_pass::evaluateSCEV(ScalarEvolution &SE, const crel_kernel &kernel, const Loop &loop, crel_mpoly &poly, const SCEV *scev) {

    if (debug) cout << " SCEV type: " << scev->getSCEVType() << " poly " << poly.toString() << endl;

    if (scev->getSCEVType() == scUnknown) {

        // Get the value related to this scevUnknown
        auto *scevUnknown = dyn_cast<llvm::SCEVUnknown>(scev);
        auto scevValue = scevUnknown->getValue();

        // Now try to get the mpoly of this value
        auto polyVal = evaluateValue(kernel, scevValue);
        string polyString = polyVal.toString();
        return polyVal;

    } else if (const auto *constExpr = dyn_cast<SCEVConstant>(scev) ) {

        crel_mpoly constPoly(kernel.runtime_vars.size());
        constPoly.setConstant64bit(constExpr->getValue()->getValue().getSExtValue());
        return constPoly;

    } else if (const auto *addExpr = dyn_cast<SCEVAddExpr>(scev) ) {

        crel_mpoly addPoly(kernel.runtime_vars.size());
        for (auto operand: addExpr->operands()) {
            addPoly.add(evaluateSCEV(SE, kernel, loop, poly, operand));
        }
        return addPoly;

    } else if (const auto *mulExpr = dyn_cast<SCEVMulExpr>(scev) ) {

        crel_mpoly multPoly(kernel.runtime_vars.size());
        multPoly.setConstant(1);
        for (auto operand: mulExpr->operands()) {
            multPoly.multiply(evaluateSCEV(SE, kernel, loop, poly, operand));
        }
        return multPoly;

    } else if (const auto *udivExpr = dyn_cast<SCEVUDivExpr>(scev) ) {

        auto lhs = udivExpr->getLHS();
        auto rhs = udivExpr->getRHS();

        crel_mpoly divPoly(kernel.runtime_vars.size());
        divPoly.setConstant(1);
        poly.multiply(evaluateSCEV(SE, kernel, loop, poly, lhs));
        poly.divideby(evaluateSCEV(SE, kernel, loop, poly, rhs));
        return divPoly;

    } else if (const auto *truncExpr = dyn_cast<SCEVTruncateExpr>(scev) ) {
        return evaluateSCEV(SE, kernel, loop, poly, truncExpr->getOperand());

    } else if (const auto *zexExpr = dyn_cast<SCEVZeroExtendExpr>(scev) ) {
        return evaluateSCEV(SE, kernel, loop, poly, zexExpr->getOperand());

    } else if (const auto *sextExpr = dyn_cast<SCEVSignExtendExpr>(scev) ) {
        return evaluateSCEV(SE, kernel, loop, poly, sextExpr->getOperand());

    // Min max operations
    } else if (scev->getSCEVType() == scUMaxExpr || scev->getSCEVType() == scSMaxExpr ||
               scev->getSCEVType() == scUMinExpr || scev->getSCEVType() == scSMinExpr)  {

        auto *minMaxExpr =  dyn_cast<SCEVMinMaxExpr>(scev);

        // If number of operands is 1, just evaluate
        // If number of operands is > 1 Handle max and min expressions by evaluating the non-constant SCEV
        if (minMaxExpr->getNumOperands() > 1) {
            for (auto operand: minMaxExpr->operands()) {
//                if (debug) cout << " minmax scev: " << operand << " num:" << minMaxExpr->getNumOperands() << " type: " << operand->getSCEVType() << " value: ";
//                operand->print(llvm::outs());
//                if (debug) cout << endl;

                if (operand->getSCEVType() != scConstant) {
                    return evaluateSCEV(SE, kernel, loop, poly, operand);
                }
            }
        } else {
            return evaluateSCEV(SE, kernel, loop, poly, minMaxExpr->getOperand(0));
        }

    } else if (scev->getSCEVType() == scCouldNotCompute)  {
        cerr << "WARNINIG: could not compute SCEV for loop " << loop.getLoopID() << " ";
        loop.print(llvm::errs());
        cerr << "in kernel: " << kernel.name << ". --> Returning a UNIT multiplier for this loop" << endl;

        // Return unit mpoly for this loop
        crel_mpoly constPoly(kernel.runtime_vars.size());
        constPoly.setConstant(1);
        return constPoly;

    } else if (scev->getSCEVType() == scAddRecExpr) {
        cerr << "WARNINIG: could not compute AddRecExpr SCEV for kernel " << kernel.name << endl;
    }

    // Always return poly
    return poly;
}

