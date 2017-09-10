#include <set>
#include <string>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifndef HAVE_LLVM
#error "This code needs LLVM enabled"
#endif

#include <llvm/Config/llvm-config.h>

#if (LLVM_VERSION_MAJOR < 3)
#error "Unsupported version of LLVM"
#endif

// ignore unused parameters in LLVM libraries
#if (__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if ((LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR < 5))
 #include <llvm/Assembly/AssemblyAnnotationWriter.h>
 #include <llvm/Analysis/Verifier.h>
#else // >= 3.5
 #include <llvm/IR/AssemblyAnnotationWriter.h>
 #include <llvm/IR/Verifier.h>
#endif

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/CommandLine.h>

#if (__clang__)
#pragma clang diagnostic pop // ignore -Wunused-parameter
#else
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <fstream>

#include "llvm/LLVMDependenceGraph.h"
#include "llvm/Slicer.h"
#include "llvm/LLVMDG2Dot.h"
#include "TimeMeasure.h"

#include "llvm/analysis/old/PointsTo.h"
#include "llvm/analysis/old/ReachingDefs.h"
#include "llvm/analysis/old/DefUse.h"

#include "llvm/analysis/DefUse.h"
#include "llvm/analysis/PointsTo/PointsTo.h"
#include "llvm/analysis/ReachingDefinitions/ReachingDefinitions.h"

#include "analysis/PointsTo/PointsToFlowInsensitive.h"
#include "analysis/PointsTo/PointsToFlowSensitive.h"
#include "analysis/PointsTo/Pointer.h"

#include "SVFPointerAnalysis.h"

#include "klee/Internal/Analysis/Cloner.h"
#include "klee/Internal/Analysis/Slicer.h"

using namespace dg;
using llvm::errs;

enum {
    // annotate
    ANNOTATE                    = 1,
    // data dependencies
    ANNOTATE_DD                 = 1 << 1,
    // forward data dependencies
    ANNOTATE_FORWARD_DD         = 1 << 2,
    // control dependencies
    ANNOTATE_CD                 = 1 << 3,
    // points-to information
    ANNOTATE_PTR                = 1 << 4,
    // reaching definitions
    ANNOTATE_RD                 = 1 << 5,
    // post-dominators
    ANNOTATE_POSTDOM            = 1 << 6,
    // comment out nodes that will be sliced
    ANNOTATE_SLICE              = 1 << 7,
};

enum PtaType {
    old, fs, fi
};

llvm::cl::OptionCategory SlicingOpts("Slicer options", "");

llvm::cl::opt<std::string> output("o",
    llvm::cl::desc("Save the output to given file. If not specified,\n"
                   "a .sliced suffix is used with the original module name."),
    llvm::cl::value_desc("filename"), llvm::cl::init(""), llvm::cl::cat(SlicingOpts));

//llvm::cl::opt<std::string> llvmfile(llvm::cl::Positional, llvm::cl::Required,
//    llvm::cl::desc("<input file>"), llvm::cl::init("test"), llvm::cl::cat(SlicingOpts));
std::string llvmfile = "test";

//llvm::cl::opt<std::string> slicing_criterion("c", llvm::cl::Required,
//    llvm::cl::desc("Slice with respect to the call-sites of a given function\n"
//                   "i. e.: '-c foo' or '-c __assert_fail'. Special value is a 'ret'\n"
//                   "in which case the slice is taken with respect to the return value\n"
//                   "of the main() function. You can use comma separated list of more\n"
//                   "function calls, e.g. -c foo,bar\n"), llvm::cl::value_desc("func"),
//                   llvm::cl::init(""), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<uint64_t> pta_field_sensitivie("pta-field-sensitive",
    llvm::cl::desc("Make PTA field sensitive/insensitive. The offset in a pointer\n"
                   "is cropped to UNKNOWN_OFFSET when it is greater than N bytes.\n"
                   "Default is full field-sensitivity (N = UNKNOWN_OFFSET).\n"),
                   llvm::cl::value_desc("N"), llvm::cl::init(UNKNOWN_OFFSET),
                   llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> rd_strong_update_unknown("rd-strong-update-unknown",
    llvm::cl::desc("Let reaching defintions analysis do strong updates on memory defined\n"
                   "with uknown offset in the case, that new definition overwrites\n"
                   "the whole memory. May be unsound for out-of-bound access\n"),
                   llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> undefined_are_pure("undefined-are-pure",
    llvm::cl::desc("Assume that undefined functions have no side-effects\n"),
                   llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<PtaType> pta("pta",
    llvm::cl::desc("Choose pointer analysis to use:"),
    llvm::cl::values(
        clEnumVal(old , "Old pointer analysis (flow-insensitive, deprecated)"),
        clEnumVal(fi, "Flow-insensitive PTA (default)"),
        clEnumVal(fs, "Flow-sensitive PTA")
#if LLVM_VERSION_MAJOR < 4
        , nullptr
#endif
        ),
    llvm::cl::init(fi), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<CD_ALG> CdAlgorithm("cd-alg",
    llvm::cl::desc("Choose control dependencies algorithm to use:"),
    llvm::cl::values(
        clEnumValN(CLASSIC , "classic", "Ferrante's algorithm (default)"),
        clEnumValN(CONTROL_EXPRESSION, "ce", "Control expression based (experimental)")
#if LLVM_VERSION_MAJOR < 4
        , nullptr
#endif
         ),
    llvm::cl::init(CLASSIC), llvm::cl::cat(SlicingOpts));

static bool createEmptyMain(llvm::Module *M)
{
    llvm::Function *main_func = M->getFunction("main");
    if (!main_func) {
        errs() << "No main function found in module. This seems like bug since\n"
                  "here we should have the graph build from main\n";
        return false;
    }

    // delete old function body
    main_func->deleteBody();

    // create new function body that just returns
    llvm::LLVMContext& ctx = M->getContext();
    llvm::BasicBlock* blk = llvm::BasicBlock::Create(ctx, "entry", main_func);
    llvm::Type *Ty = main_func->getReturnType();
    llvm::Value *retval = nullptr;
    if (Ty->isIntegerTy())
        retval = llvm::ConstantInt::get(Ty, 0);
    llvm::ReturnInst::Create(ctx, retval, blk);

    return true;
}

static std::vector<std::string> splitList(const std::string& opt)
{
    std::vector<std::string> ret;
    if (opt.empty())
        return ret;

    size_t old_pos = 0;
    size_t pos = 0;
    while (true) {
        old_pos = pos;

        pos = opt.find(',', pos);
        ret.push_back(opt.substr(old_pos, pos - old_pos));

        if (pos == std::string::npos)
            break;
        else
            ++pos;
    }

    return ret;
}

static bool array_match(llvm::StringRef name, const char *names[])
{
    unsigned idx = 0;
    while(names[idx]) {
        if (name.equals(names[idx]))
            return true;
        ++idx;
    }

    return false;
}

static bool verify_module(llvm::Module *M)
{
    // the verifyModule function returns false if there
    // are no errors

#if ((LLVM_VERSION_MAJOR >= 4) || (LLVM_VERSION_MINOR >= 5))
    return !llvm::verifyModule(*M, &llvm::errs());
#else
    return !llvm::verifyModule(*M, llvm::PrintMessageAction);
#endif
}

static void replace_suffix(std::string& fl, const std::string& with)
{
    if (fl.size() > 2) {
        if (fl.compare(fl.size() - 2, 2, ".o") == 0)
            fl.replace(fl.end() - 2, fl.end(), with);
        else if (fl.compare(fl.size() - 3, 3, ".bc") == 0)
            fl.replace(fl.end() - 3, fl.end(), with);
        else
            fl += with;
    } else {
        fl += with;
    }
}
static bool write_module(llvm::Module *M)
{
    // compose name if not given
    std::string fl;
    if (!output.empty()) {
        fl = output;
    } else {
        fl = llvmfile;
        replace_suffix(fl, ".sliced");
    }

    // open stream to write to
    std::ofstream ofs(fl);
    llvm::raw_os_ostream ostream(ofs);

    // write the module
    errs() << "INFO: saving sliced module to: " << fl.c_str() << "\n";
    llvm::WriteBitcodeToFile(M, ostream);

    return true;
}

static int verify_and_write_module(llvm::Module *M)
{
    int code = 1;

    if (!verify_module(M)) {
        errs() << "ERR: Verifying module failed, the IR is not valid\n";
        errs() << "INFO: Saving anyway so that you can check it\n";
    }

    if (!write_module(M)) {
        errs() << "Saving sliced module failed\n";
        goto cleanup;
    }

    code = 0;

cleanup:
    // exit code
    return code;
}

static int save_module(llvm::Module *M,
                       bool should_verify_module = true)
{
    if (should_verify_module)
        return verify_and_write_module(M);
    else
        return write_module(M);
}

/// --------------------------------------------------------------------
//   - Slicer class -
//
//  The main class that represents slicer and covers the elementary
//  functionality
/// --------------------------------------------------------------------
Slicer::Slicer(
    llvm::Module *mod,
    uint32_t o,
    std::string entryFunction,
    std::vector<std::string> criterions,
    LLVMPointerAnalysis *llvmpta,
    Cloner *cloner
) :
    M(mod), 
    opts(o),
    entryFunction(entryFunction),
    criterions(criterions),
    PTA(llvmpta),
    RD(
        new LLVMReachingDefinitions(
            mod,
            PTA,
            rd_strong_update_unknown,
            undefined_are_pure,
            ~((uint32_t)(0)),
            entryFunction
        )
    )
{
    assert(mod && "Need module");
    slicer.setCloner(cloner);
    slice_id = 0xdead;
}

int Slicer::run()
{
    if (!M) {
        llvm::errs() << "Failed parsing '" << llvmfile << "' file:\n";
        return 1;
    }

    // remove unused from module, we don't need that
    //remove_unused_from_module_rec();

    // build the dependence graph, so that we can dump it if desired
    if (!buildDG()) {
        errs() << "ERROR: Failed building DG\n";
        return 1;
    }

    // mark nodes that are going to be in the slice
    mark();

    // slice the graph
    if (!slice()) {
        errs() << "ERROR: Slicing failed\n";
        return 1;
    }

    // remove unused from module again, since slicing
    // could and probably did make some other parts unused
    //remove_unused_from_module_rec();

    // fix linkage of declared functions (if needs to be fixed)
    make_declarations_external();

    return save_module(M, false);
}

bool Slicer::buildDG()
{
    debug::TimeMeasure tm;

    tm.start();

    //PTA->PS->setRoot(PTA->builder->buildLLVMPointerSubgraph());
    //SVFPointerAnalysis *pa = new SVFPointerAnalysis(M, PTA.get(), svfaa);
    //pa->run();

    tm.stop();
    tm.report("INFO: Points-to analysis took");

    dg.build(M, PTA, M->getFunction(entryFunction));

    // verify if the graph is built correctly
    // FIXME - do it optionally (command line argument)
    if (!dg.verify()) {
        errs() << "ERR: verifying failed\n";
        return false;
    }

    return true;
}

bool Slicer::mark()
{
    debug::TimeMeasure tm;
    std::set<LLVMNode *> callsites;

    assert(!criterions.empty() && "Do not have the slicing criterion");

    for (std::string c : criterions) {
        if (c == "ret") {
            callsites.insert(dg.getExit());
        }
    }

    // check for slicing criterion here, because
    // we might have built new subgraphs that contain
    // it during points-to analysis
    bool ret = dg.getCallSites(criterions, &callsites);
    got_slicing_criterion = true;
    if (!ret) {
        errs() << "Did not find slicing criterion:\n";
        for (std::string c : criterions) {
          errs() << "\tmissing criterion: " << c << "\n";
        }
        got_slicing_criterion = false;
    }

    // if we found slicing criterion, compute the rest
    // of the graph. Otherwise just slice away the whole graph
    // Also compute the edges when the user wants to annotate
    // the file - due to debugging.
    if (got_slicing_criterion || (opts & ANNOTATE))
        computeEdges();

    // don't go through the graph when we know the result:
    // only empty main will stay there. Just delete the body
    // of main and keep the return value
    if (!got_slicing_criterion) {
        /* TODO: decide what to do... */
        return 0; //createEmptyMain(M);
    }

    /* TODO: check what happens with the slicing... */
    // we also do not want to remove any assumptions
    // about the code
    // FIXME: make it configurable and add control dependencies
    // for these functions, so that we slice away the
    // unneeded one
    const char *sc[] = {
        "__VERIFIER_assume",
        "__VERIFIER_exit",
        //"klee_assume",
        "exit",
        /* these are needed, otherwise vprintf crashes... */
        "llvm.va_start",
        "llvm.va_end",
        NULL // termination
    };

    dg.getCallSites(sc, &callsites);

    // FIXME: do this optional
    /* TODO: add klee_* functions */
    slicer.keepFunctionUntouched("__VERIFIER_assume");
    slicer.keepFunctionUntouched("__VERIFIER_exit");
    //slice_id = 1; //0xdead;

    tm.start();
    for (LLVMNode *start : callsites) {
        slice_id = slicer.mark(start, slice_id);
    }

    tm.stop();
    tm.report("INFO: Finding dependent nodes took");

    return true;
}

void Slicer::computeEdges()
{
    debug::TimeMeasure tm;
    assert(PTA && "BUG: No PTA");
    assert(RD && "BUG: No RD");

    tm.start();
    RD->run();
    tm.stop();
    tm.report("INFO: Reaching defs analysis took");

    LLVMDefUseAnalysis DUA(&dg, RD.get(), PTA, undefined_are_pure);
    tm.start();
    DUA.run(); // add def-use edges according that
    tm.stop();
    tm.report("INFO: Adding Def-Use edges took");

    tm.start();
    // add post-dominator frontiers
    dg.computeControlDependencies(CdAlgorithm);
    tm.stop();
    tm.report("INFO: Computing control dependencies took");
}

bool Slicer::slice()
{
    // we created an empty main in this case
    if (!got_slicing_criterion)
        return true;

    if (slice_id == 0) {
        if (!mark())
            return false;
    }

    debug::TimeMeasure tm;

    tm.start();
    slicer.slice(&dg, nullptr, slice_id);

    tm.stop();
    tm.report("INFO: Slicing dependence graph took");

    analysis::SlicerStatistics& st = slicer.getStatistics();
    errs() << "INFO: Sliced away " << st.nodesRemoved
        << " from " << st.nodesTotal << " nodes in DG\n";

    return true;
}

/* TODO: may collide with the reachability analysis? */
void Slicer::remove_unused_from_module_rec()
{
    bool fixpoint;

    do {
        fixpoint = remove_unused_from_module();
    } while (fixpoint);
}

bool Slicer::remove_unused_from_module()
{
    using namespace llvm;
    // do not slice away these functions no matter what
    // FIXME do it a vector and fill it dynamically according
    // to what is the setup (like for sv-comp or general..)
    const char *keep[] = {"main", "klee_assume", NULL};

    // when erasing while iterating the slicer crashes
    // so set the to be erased values into container
    // and then erase them
    std::set<Function *> funs;
    std::set<GlobalVariable *> globals;
    std::set<GlobalAlias *> aliases;
    auto cf = getConstructedFunctions();

    for (auto I = M->begin(), E = M->end(); I != E; ++I) {
        Function *func = &*I;
        if (array_match(func->getName(), keep))
            continue;

        // if the function is unused or we haven't constructed it
        // at all in dependence graph, we can remove it
        // (it may have some uses though - like when one
        // unused func calls the other unused func
        if (func->hasNUses(0))
            funs.insert(func);
    }

    for (auto I = M->global_begin(), E = M->global_end(); I != E; ++I) {
        GlobalVariable *gv = &*I;
        if (gv->hasNUses(0))
            globals.insert(gv);
    }

    for (GlobalAlias& ga : M->getAliasList()) {
        if (ga.hasNUses(0))
            aliases.insert(&ga);
    }

    for (Function *f : funs)
        f->eraseFromParent();
    for (GlobalVariable *gv : globals)
        gv->eraseFromParent();
    for (GlobalAlias *ga : aliases)
        ga->eraseFromParent();

    return (!funs.empty() || !globals.empty() || !aliases.empty());
}

// after we slice the LLVM, we somethimes have troubles
// with function declarations:
//
//   Global is external, but doesn't have external or dllimport or weak linkage!
//   i32 (%struct.usbnet*)* @always_connected
//   invalid linkage type for function declaration
//
// This function makes the declarations external
void Slicer::make_declarations_external()
{
    using namespace llvm;

    // iterate over all functions in module
    for (auto I = M->begin(), E = M->end(); I != E; ++I) {
        Function *func = &*I;
        if (func->size() == 0) {
            // this will make sure that the linkage has right type
            func->deleteBody();
        }
    }
}

Slicer::~Slicer() {
    clearConstructedFunctions();
}
