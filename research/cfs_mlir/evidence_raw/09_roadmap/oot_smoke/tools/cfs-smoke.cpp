#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "CfsDialect.h.inc"
#define GET_OP_CLASSES
#include "CfsOps.h.inc"
#include "llvm/Support/raw_ostream.h"
int main() {
  mlir::DialectRegistry r; r.insert<cfs::CFSDialect, mlir::arith::ArithDialect>();
  mlir::MLIRContext ctx(r); ctx.loadAllAvailableDialects();
  const char *src = "func_like_region : %0 = arith.constant 6274 : i32\n";
  (void)src;
  auto m = mlir::parseSourceString<mlir::ModuleOp>(
    "%m = arith.constant 6274 : i32\n%p = arith.constant 1 : i32\n%s = cfs.sb.subscribe %m, %p : i32\n", &ctx);
  if (!m) { llvm::errs() << "parse failed\n"; return 1; }
  if (mlir::failed(mlir::verify(*m))) { llvm::errs() << "verify failed\n"; return 2; }
  m->print(llvm::outs()); llvm::outs() << "\n";
  return 0;
}
