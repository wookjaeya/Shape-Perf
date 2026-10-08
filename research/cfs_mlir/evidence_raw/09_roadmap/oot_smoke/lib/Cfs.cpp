#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "CfsDialect.h.inc"
#define GET_OP_CLASSES
#include "CfsOps.h.inc"
#include "CfsDialect.cpp.inc"
#define GET_OP_CLASSES
#include "CfsOps.cpp.inc"
void cfs::CFSDialect::initialize() { addOperations<
#define GET_OP_LIST
#include "CfsOps.cpp.inc"
>(); }
