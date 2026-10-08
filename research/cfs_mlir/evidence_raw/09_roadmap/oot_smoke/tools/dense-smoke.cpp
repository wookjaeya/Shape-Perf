// API smoke test only (not the proposed analysis): checks that a DenseForwardDataFlowAnalysis
// subclass with an ExternalCallee hook compiles and runs at llvm-project 1053047a.
#include "mlir/Analysis/DataFlow/DenseAnalysis.h"
#include "mlir/Analysis/DataFlow/Utils.h"
#include "mlir/Analysis/DataFlowFramework.h"
#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <string>
using namespace mlir; using namespace mlir::dataflow;
static unsigned hookExternalDefined = 0, hookExternalDecl = 0, hookOther = 0;
class WrittenGlobals : public AbstractDenseLattice {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(WrittenGlobals)
  using AbstractDenseLattice::AbstractDenseLattice;
  std::set<std::string> s;
  ChangeResult join(const AbstractDenseLattice &o) override {
    size_t n = s.size(); for (auto &x : static_cast<const WrittenGlobals &>(o).s) s.insert(x);
    return n == s.size() ? ChangeResult::NoChange : ChangeResult::Change; }
  ChangeResult add(const std::string &x) { return s.insert(x).second ? ChangeResult::Change : ChangeResult::NoChange; }
  void print(raw_ostream &os) const override { os << "{"; for (auto &x : s) os << x << " "; os << "}"; }
};
class WrittenGlobalsAnalysis : public DenseForwardDataFlowAnalysis<WrittenGlobals> {
public:
  using DenseForwardDataFlowAnalysis::DenseForwardDataFlowAnalysis;
  LogicalResult visitOperation(Operation *op, const WrittenGlobals &before, WrittenGlobals *after) override {
    ChangeResult c = after->join(before);
    if (auto st = dyn_cast<LLVM::StoreOp>(op)) {
      Value a = st.getAddr();
      while (auto g = a.getDefiningOp<LLVM::GEPOp>()) a = g.getBase();
      if (auto ao = a.getDefiningOp<LLVM::AddressOfOp>()) c |= after->add(ao.getGlobalName().str());
    }
    propagateIfChanged(after, c); return success();
  }
  void visitCallControlFlowTransfer(CallOpInterface call, CallControlFlowAction action,
                                    const WrittenGlobals &before, WrittenGlobals *after) override {
    if (action != CallControlFlowAction::ExternalCallee) { ++hookOther;
      return DenseForwardDataFlowAnalysis::visitCallControlFlowTransfer(call, action, before, after); }
    auto sym = dyn_cast_if_present<SymbolRefAttr>(call.getCallableForCallee().dyn_cast<SymbolRefAttr>());
    bool defined = false; std::string name = sym ? sym.getLeafReference().str() : "<indirect>";
    if (sym) if (auto fn = SymbolTable::lookupNearestSymbolFrom<LLVM::LLVMFuncOp>(call, sym)) defined = !fn.isExternal();
    defined ? ++hookExternalDefined : ++hookExternalDecl;
    ChangeResult c = after->join(before);
    if (name == "CFE_SB_ReceiveBuffer") c |= after->add("#rcv");
    propagateIfChanged(after, c);
  }
  void setToEntryState(WrittenGlobals *l) override { propagateIfChanged(l, ChangeResult::NoChange); }
};
int main(int argc, char **argv) {
  DialectRegistry r; r.insert<LLVM::LLVMDialect, DLTIDialect>();
  MLIRContext ctx(r); ctx.loadAllAvailableDialects(); ctx.allowUnregisteredDialects();
  auto m = parseSourceFile<ModuleOp>(argv[1], &ctx);
  if (!m) { llvm::errs() << "parse failed\n"; return 1; }
  DataFlowSolver solver(DataFlowConfig().setInterprocedural(false));
  loadBaselineAnalyses(solver);
  solver.load<WrittenGlobalsAnalysis>();
  if (failed(solver.initializeAndRun(*m))) { llvm::errs() << "solver failed\n"; return 2; }
  m->walk([&](LLVM::CallOp c) {
    auto cal = c.getCallee(); if (!cal || !cal->starts_with("CFE_SB_")) return;
    auto *st = solver.lookupState<WrittenGlobals>(solver.getProgramPointBefore(c));
    llvm::outs() << *cal << " @ " << c.getLoc() << " before=";
    if (st) st->print(llvm::outs()); else llvm::outs() << "<none>";
    llvm::outs() << "\n"; });
  llvm::outs() << "hook ExternalCallee: defined-callee=" << hookExternalDefined
               << " decl-callee=" << hookExternalDecl << " other-actions=" << hookOther << "\n";
  return 0;
}
