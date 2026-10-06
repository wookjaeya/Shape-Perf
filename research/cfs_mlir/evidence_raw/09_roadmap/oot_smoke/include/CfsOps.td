include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
def CFS_Dialect : Dialect { let name = "cfs"; let cppNamespace = "::cfs"; }
class CFS_Op<string m, list<Trait> t = []> : Op<CFS_Dialect, m, t>;
def CFS_SBSubscribeOp : CFS_Op<"sb.subscribe", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins I32:$mid, I32:$pipe);
  let results = (outs I32:$status);
  let assemblyFormat = "$mid `,` $pipe attr-dict `:` type($status)";
}
