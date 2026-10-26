// kzllvm.cpp -- compiler binding candidate "LLVM IR pass plugin" of the Kairos instrumentation family
// (stage C fitness prototype; kairos/docs/instr_candidate_llvm.md).
//
// A new-pass-manager plugin for the clang of the pinned LLVM build (1053047a), loaded with
//   clang -fpass-plugin=kzllvm.so -mllvm -kzllvm-manifest=<kzs-sites file> -mllvm -kzllvm-out=<dir> ...
// It reads a logical site manifest ("kzs-sites 1", kairos/docs/event_semantics.md section 3, parsed by the
// runtime's own host parser) and, in the translation units the manifest names, inserts calls of the
// runtime's observation hooks (kz_rt_obs, kz_rt_obs_str; hook ABI 1, kz_rt.h) and of the control hook
// kz_rt_ctl at
//   - a call of <callee> inside <func>        (phase=before: the call's argument operands; phase=after:
//                                              the call's own result value and out<k> = *arg<k> read right
//                                              after it by the same thread)
//   - the entry of <func> (phase=before, no callee: the function's arguments) and its returns
//     (phase=after: the return operand)
//   - a store through a pointer parameter in <func> (phase=effect, no callee: operand0 = stored value,
//     operand1 = the address; line=<n> restricts it to that line)
// Values are the IR operands the original instruction uses (SSA values), converted to 64 bits per the
// manifest type: nothing is evaluated twice, no shared variable is re-read (except the out<k> read the
// manifest declares), no store or call is changed, no value is replaced.  A string is copied only at a slot
// the manifest marks string=validated; the pass additionally reports what the IR says about the operand
// (an address of a [N x i8] array field or object) in the binding file.
//
// Pass positions: insertion at the PipelineStart extension point (the IR as clang emitted it, before every
// optimization, also at -O0), so the hooks follow inlining, cloning and unrolling of the code around them;
// a check at the OptimizerLast extension point counts, per binding, the hook calls left in each final
// function (bound / inlined / duplicated / removed) and writes one "kzs-sites 1" binding file per unit.
// Code generation can still duplicate a call (tail duplication); the linked file is checked separately
// (scripts/bincheck.sh of the GCC candidate).  A unit no site names is not touched at all.
//
// Value replacement: this pass has none.  (LLVM's Instrumentor offers Replace* configuration kinds; it is
// not used here and is not present in the pinned LLVM build.)
//
// Options (-mllvm):
//   -kzllvm-manifest=<file>      the logical site manifest (required when the plugin is loaded)
//   -kzllvm-out=<dir>            where the per-unit binding file goes (required)
//   -kzllvm-module=<id>:<prefix> units whose source path starts with <prefix> belong to module <id>
//   -kzllvm-default-module=<id>  module of the other units (default 1)
//   -kzllvm-skip-coverage        do not instrument units compiled with -fprofile-arcs (llvm.gcov metadata)
// The runtime itself is never instrumented: a unit that defines a kz_rt_* function is refused.
//
// LLVM API: the plugin is built against the exact LLVM build that loads it (LLVM_PLUGIN_API_VERSION and
// the C++ API of that revision); no compatibility across LLVM revisions is assumed.

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "kz/core/sha256.hpp"
#include "kz_rt_manifest.hpp"

using namespace llvm;

namespace {

const char* const kVersion = "kzllvm 0.1";
const char* const kBackend = "llvm-pass";

cl::opt<std::string> OptManifest("kzllvm-manifest", cl::desc("kzllvm: the logical site manifest (kzs-sites 1)"), cl::init(""));
cl::opt<std::string> OptOut("kzllvm-out", cl::desc("kzllvm: directory of the per-unit binding files"), cl::init(""));
cl::list<std::string> OptModule("kzllvm-module", cl::desc("kzllvm: <id>:<source path prefix> (repeatable)"));
cl::opt<unsigned> OptDefaultModule("kzllvm-default-module", cl::desc("kzllvm: module of the other units"), cl::init(1));
cl::opt<bool> OptSkipCoverage("kzllvm-skip-coverage", cl::desc("kzllvm: leave units compiled with -fprofile-arcs alone"),
                              cl::init(false));

// ---- per-unit state (one module per cc1 process) ---------------------------------------------------
struct BindRec {
  uint32_t id = 0, site = 0, module = 0, vals = 0;
  std::string kind, orig_func, at;
  std::string status = "bound", reason, notes;
  std::map<std::string, int> copies;  // final function -> hook calls (check)
};

struct State {
  bool loaded = false, load_failed = false;
  std::string manifest_path, out_dir, manifest_text;
  kz::Digest manifest_sha{};
  kzrt::Manifest m;
  std::vector<std::pair<uint32_t, std::string>> modules;
  // the unit
  bool active = false, refused = false;
  std::string tu, output;
  uint32_t module = 1;
  std::vector<const kzrt::Site*> sites;
  std::vector<BindRec> binds;
  std::map<uint32_t, uint32_t> next_n;
  std::set<uint32_t> matched;
  std::vector<std::string> notes;
  std::map<std::string, int> ctl_copies;
  std::string opt_level;
  bool has_debug_locs = false;
};
State kz;

bool ends_with_path(const std::string& path, const std::string& suffix) {
  if (suffix.empty() || path.size() < suffix.size()) return false;
  if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) return false;
  return path.size() == suffix.size() || path[path.size() - suffix.size() - 1] == '/';
}

bool read_file(const std::string& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

// The object file of this compilation, from the cc1 command line (/proc/self/cmdline, Linux host): it keeps
// the binding files of two compilations of one source apart (the role of GCC's dumpbase).
std::string output_name() {
  std::string cmd;
  if (!read_file("/proc/self/cmdline", &cmd)) return "";
  std::vector<std::string> args;
  size_t b = 0;
  for (size_t i = 0; i < cmd.size(); ++i)
    if (cmd[i] == '\0') {
      args.push_back(cmd.substr(b, i - b));
      b = i + 1;
    }
  for (size_t i = 0; i + 1 < args.size(); ++i)
    if (args[i] == "-o") return args[i + 1];
  return "";
}

bool load_manifest() {
  if (kz.loaded) return true;
  if (kz.load_failed) return false;
  kz.manifest_path = OptManifest;
  kz.out_dir = OptOut;
  if (kz.manifest_path.empty() || kz.out_dir.empty()) {
    errs() << "kzllvm: -mllvm -kzllvm-manifest=<file> and -mllvm -kzllvm-out=<dir> are required\n";
    kz.load_failed = true;
    return false;
  }
  std::string err;
  if (!read_file(kz.manifest_path, &kz.manifest_text)) {
    errs() << "kzllvm: cannot read " << kz.manifest_path << "\n";
    kz.load_failed = true;
    return false;
  }
  if (!kzrt::parse_manifest(kz.manifest_text, kz.manifest_path, &kz.m, &err)) {
    errs() << "kzllvm: " << err << "\n";
    kz.load_failed = true;
    return false;
  }
  kz.manifest_sha = kz::sha256(kz.manifest_text);
  for (const std::string& v : OptModule) {
    size_t c = v.find(':');
    if (c == std::string::npos) {
      errs() << "kzllvm: -kzllvm-module wants <id>:<prefix>, got " << v << "\n";
      kz.load_failed = true;
      return false;
    }
    kz.modules.push_back({(uint32_t)std::strtoul(v.substr(0, c).c_str(), nullptr, 10), v.substr(c + 1)});
  }
  kz.module = OptDefaultModule;
  kz.loaded = true;
  return true;
}

// ---- hooks -------------------------------------------------------------------------------------------
struct Hooks {
  FunctionCallee obs, obs_str, ctl, module;
};

Hooks declare_hooks(Module& M) {
  LLVMContext& C = M.getContext();
  Type* i32 = Type::getInt32Ty(C);
  Type* i64 = Type::getInt64Ty(C);
  Type* ptr = PointerType::getUnqual(C);
  Hooks h;
  // kz_rt_obs reads no memory of the target and never calls back into this module: nocallback (the
  // counterpart of GCC's leaf).  Its memory effects are left unknown, so the optimizer keeps it in order with
  // the surrounding loads, stores and calls.  kz_rt_obs_str reads the string it is given; kz_rt_ctl may block.
  AttributeList obs_attrs = AttributeList().addFnAttribute(C, Attribute::NoUnwind).addFnAttribute(C, Attribute::NoCallback);
  AttributeList plain = AttributeList().addFnAttribute(C, Attribute::NoUnwind);
  h.obs = M.getOrInsertFunction("kz_rt_obs", FunctionType::get(i32, {i32, i32, i32, i32, i64, i64, i64, i64}, false), obs_attrs);
  h.obs_str =
      M.getOrInsertFunction("kz_rt_obs_str", FunctionType::get(i32, {i32, i32, i32, i32, i64, i64, i64, i64, i32, i32}, false), plain);
  h.ctl = M.getOrInsertFunction("kz_rt_ctl", FunctionType::get(i32, {i32, i32, i32, i32}, false), plain);
  h.module = M.getOrInsertFunction("kz_rt_module", FunctionType::get(Type::getVoidTy(C), {i32, ptr}, false), plain);
  return h;
}

uint32_t bind_id(uint32_t module, uint32_t site, uint32_t n) { return (module << 16) | (site << 4) | n; }

unsigned dl_line(const Instruction* I) {
  const DebugLoc& d = I->getDebugLoc();
  return d ? d.getLine() : 0;
}

std::string where(const Instruction* I, const std::string& func) {
  std::string file = kz.tu;
  if (const DebugLoc& d = I->getDebugLoc()) {
    if (auto* s = dyn_cast<DIScope>(d.getScope())) file = s->getFilename().str();
  }
  size_t sl = file.rfind('/');
  return (sl == std::string::npos ? file : file.substr(sl + 1)) + ":" + std::to_string(dl_line(I)) + " " + func;
}

// The [N x i8] array that starts exactly at byte offset off inside an object of type t (descending through
// structs and arrays), or 0.
uint64_t char_array_at(Type* t, uint64_t off, const DataLayout& DL) {
  for (int depth = 0; depth < 16 && t != nullptr; ++depth) {
    if (auto* a = dyn_cast<ArrayType>(t)) {
      if (a->getElementType()->isIntegerTy(8)) return off == 0 ? a->getNumElements() : 0;
      uint64_t es = DL.getTypeAllocSize(a->getElementType());
      if (es == 0 || off >= es * a->getNumElements()) return 0;
      off %= es;
      t = a->getElementType();
    } else if (auto* st = dyn_cast<StructType>(t)) {
      if (st->isOpaque()) return 0;
      const StructLayout* sl = DL.getStructLayout(st);
      if (off >= sl->getSizeInBytes()) return 0;
      unsigned i = sl->getElementContainingOffset(off);
      off -= sl->getElementOffset(i);
      t = st->getElementType(i);
    } else {
      return 0;
    }
  }
  return 0;
}

// What the IR says about a string operand: the address of a [N x i8] array -- a field reached by a GEP, the
// decay of such an array, or a constant offset into a global/alloca whose type has such an array there (clang
// folds &g[0].name into @g).  "" when nothing is known (opaque pointers carry no pointee type).
std::string string_evidence(const Value* v, const DataLayout& DL) {
  auto arr = [](Type* t) -> uint64_t {
    auto* a = dyn_cast_or_null<ArrayType>(t);
    return (a != nullptr && a->getElementType()->isIntegerTy(8)) ? a->getNumElements() : 0;
  };
  if (auto* g = dyn_cast<GEPOperator>(v)) {
    if (uint64_t n = arr(g->getResultElementType())) return "array char[" + std::to_string(n) + "] (gep field address)";
    if (g->hasAllZeroIndices())
      if (uint64_t n = arr(g->getSourceElementType())) return "array char[" + std::to_string(n) + "] (array decay)";
  }
  APInt off(DL.getIndexTypeSizeInBits(v->getType()), 0);
  const Value* base = v->stripAndAccumulateConstantOffsets(DL, off, /*AllowNonInbounds=*/true);
  if (off.isNegative()) return "";
  Type* bt = nullptr;
  const char* what = "";
  if (auto* gv = dyn_cast<GlobalVariable>(base)) {
    bt = gv->getValueType();
    what = "global";
  } else if (auto* a = dyn_cast<AllocaInst>(base)) {
    bt = a->getAllocatedType();
    what = "local";
  }
  if (bt != nullptr)
    if (uint64_t n = char_array_at(bt, off.getZExtValue(), DL))
      return "array char[" + std::to_string(n) + "] (" + what + " at constant offset " + std::to_string(off.getZExtValue()) + ")";
  return "";
}

// The integer type the IR says a pointer operand points to (a GEP's result element type, a global's or an
// alloca's type), or nullptr.
Type* pointee_evidence(const Value* p) {
  if (auto* g = dyn_cast<GEPOperator>(p)) return g->getResultElementType();
  if (auto* gv = dyn_cast<GlobalVariable>(p)) return gv->getValueType();
  if (auto* a = dyn_cast<AllocaInst>(p)) return a->getAllocatedType();
  return nullptr;
}

struct Operands {
  std::vector<Value*> arg;   // call argument operands or function arguments
  Value* ret = nullptr;      // the call's result / the return operand
  Value* operand0 = nullptr; // store: value
  Value* operand1 = nullptr; // store: address
  CallBase* call = nullptr;  // for out<k>
  Function* fn = nullptr;    // the instrumented function (byval/sret checks of its own arguments)
};

void manifest_width(const std::string& t, unsigned* bits, bool* sign) {
  *bits = 64;
  *sign = false;
  if (t == "ptr") return;
  if (t.size() >= 2 && (t[0] == 'u' || t[0] == 's')) {
    *sign = t[0] == 's';
    *bits = (unsigned)std::atoi(t.c_str() + 1);
  }
}

// v -> i64 per the manifest type (unsigned zero-extends, signed sign-extends, ptr is the pointer value).
// nullptr and *why when the IR value cannot be passed.
Value* to_u64(IRBuilder<>& B, Value* v, const kzrt::Value& val, std::string* why) {
  unsigned bits;
  bool sign;
  manifest_width(val.type, &bits, &sign);
  Type* t = v->getType();
  Type* it = B.getIntNTy(bits);
  Value* x;
  if (t->isPointerTy()) {
    x = B.CreatePtrToInt(v, B.getInt64Ty());
    if (bits < 64) x = B.CreateTrunc(x, it);
  } else if (t->isIntegerTy()) {
    unsigned w = t->getIntegerBitWidth();
    if (w > bits) x = B.CreateTrunc(v, it);
    else if (w < bits) x = (sign && w > 1) ? B.CreateSExt(v, it) : B.CreateZExt(v, it);
    else x = v;
  } else {
    std::string ts;
    raw_string_ostream os(ts);
    t->print(os);
    *why = "IR value of type " + ts + " (only integers and pointers are passed; aggregates arrive ABI-lowered)";
    return nullptr;
  }
  if (bits == 64) return x;
  return sign ? B.CreateSExt(x, B.getInt64Ty()) : B.CreateZExt(x, B.getInt64Ty());
}

// Builds the observation call for site s at one binding at the builder's position; returns its aseq value
// (i32) or nullptr (then *why is set and nothing was inserted).
Value* build_obs(const Hooks& h, const kzrt::Site& s, uint32_t bind, Value* link, const Operands& ops, IRBuilder<>& B, uint32_t* vals,
                 std::string* notes, std::string* why) {
  // collect first, insert only when every value can be bound
  struct Src {
    const kzrt::Value* val;
    Value* v;
    bool out_load;
    Type* load_ty;
    Align align;
  };
  std::vector<Src> srcs;
  int str_slot = -1;
  uint32_t str_bound = 0;
  for (const kzrt::Value& val : s.values) {
    if (val.slot > 3) {
      *why = "slot > 3 (this prototype binds at most four values per event)";
      return nullptr;
    }
    const std::string& f = val.from;
    Src src{&val, nullptr, false, nullptr, Align(1)};
    if (f == "ret") {
      src.v = ops.ret;
      if (src.v == nullptr) {
        *why = "no result value at this binding";
        return nullptr;
      }
    } else if (f.compare(0, 3, "arg") == 0) {
      size_t k = (size_t)std::atoi(f.c_str() + 3);
      if (k >= ops.arg.size()) {
        *why = "argument " + std::to_string(k) + " does not exist here";
        return nullptr;
      }
      // the IR argument k is the C argument k only without ABI rewriting of earlier arguments
      if (ops.call != nullptr) {
        for (unsigned i = 0; i <= k && i < ops.call->arg_size(); ++i)
          if (ops.call->paramHasAttr(i, Attribute::StructRet) || ops.call->paramHasAttr(i, Attribute::ByVal)) {
            *why = "sret/byval argument at or before IR argument " + std::to_string(k) + " (ABI-lowered call)";
            return nullptr;
          }
      } else if (ops.fn != nullptr) {
        for (unsigned i = 0; i <= k && i < ops.fn->arg_size(); ++i)
          if (ops.fn->hasParamAttribute(i, Attribute::StructRet) || ops.fn->hasParamAttribute(i, Attribute::ByVal)) {
            *why = "sret/byval parameter at or before IR argument " + std::to_string(k) + " (ABI-lowered function)";
            return nullptr;
          }
      }
      src.v = ops.arg[k];
    } else if (f.compare(0, 3, "out") == 0) {
      size_t k = (size_t)std::atoi(f.c_str() + 3);
      if (ops.call == nullptr || k >= ops.call->arg_size()) {
        *why = f + " needs a call argument " + std::to_string(k);
        return nullptr;
      }
      Value* p = ops.call->getArgOperand(k);
      if (!p->getType()->isPointerTy()) {
        *why = f + ": the argument is not a pointer";
        return nullptr;
      }
      unsigned bits;
      bool sign;
      manifest_width(val.type, &bits, &sign);
      Type* lt = B.getIntNTy(bits);
      Type* ev = pointee_evidence(p);
      src.v = p;
      src.out_load = true;
      src.load_ty = lt;
      if (ev != nullptr && ev == lt) {
        src.align = B.GetInsertBlock()->getModule()->getDataLayout().getABITypeAlign(lt);
        *notes += " " + f + "=load-after-call(i" + std::to_string(bits) + ", pointee type confirmed by IR)";
      } else {
        *notes += " " + f + "=load-after-call(i" + std::to_string(bits) + " from the manifest type, align 1; pointee type not in IR)";
      }
    } else if (f == "operand0") {
      src.v = ops.operand0;
    } else if (f == "operand1") {
      src.v = ops.operand1;
    }
    if (src.v == nullptr) {
      *why = "value source " + f + " does not exist here";
      return nullptr;
    }
    if (val.type == "ptr" && val.string_mode == "validated") {
      std::string e = string_evidence(src.v, B.GetInsertBlock()->getModule()->getDataLayout());
      *notes += " slot" + std::to_string(val.slot) + ".string=" + (e.empty() ? "type-unverified" : "type:" + e);
      if (!e.empty()) {
        unsigned long n = std::strtoul(e.c_str() + e.find('[') + 1, nullptr, 10);
        if (n < val.bound) *notes += "(array smaller than bound " + std::to_string(val.bound) + ")";
      }
      str_slot = (int)val.slot;
      str_bound = val.bound;
    }
    // a type check that does not need instructions
    Type* t = src.out_load ? src.load_ty : src.v->getType();
    if (!t->isIntegerTy() && !t->isPointerTy()) {
      std::string ts;
      raw_string_ostream os(ts);
      t->print(os);
      *why = "IR value of type " + ts + " (only integers and pointers are passed; aggregates arrive ABI-lowered)";
      return nullptr;
    }
    srcs.push_back(src);
  }
  Value* v[4] = {B.getInt64(0), B.getInt64(0), B.getInt64(0), B.getInt64(0)};
  uint32_t valid = 0;
  for (const Src& src : srcs) {
    Value* x = src.v;
    if (src.out_load) x = B.CreateAlignedLoad(src.load_ty, src.v, src.align, "kz.out");
    Value* c = to_u64(B, x, *src.val, why);
    if (c == nullptr) return nullptr;  // not reached: checked above
    v[src.val->slot] = c;
    valid |= 1u << src.val->slot;
  }
  CallInst* call;
  if (str_slot >= 0)
    call = B.CreateCall(h.obs_str, {B.getInt32(s.id), B.getInt32(bind), link, B.getInt32(valid), v[0], v[1], v[2], v[3],
                                    B.getInt32((uint32_t)str_slot), B.getInt32(str_bound)});
  else
    call = B.CreateCall(h.obs, {B.getInt32(s.id), B.getInt32(bind), link, B.getInt32(valid), v[0], v[1], v[2], v[3]});
  call->setDoesNotThrow();
  *vals = valid;
  return call;
}

void add_ctl(const Hooks& h, const kzrt::Site& s, uint32_t bind, Value* aseq, IRBuilder<>& B) {
  CallInst* c = B.CreateCall(h.ctl, {B.getInt32(s.id), B.getInt32(bind), aseq, B.getInt32(kzrt::ctx_code(s.ctx))});
  c->setDoesNotThrow();
}

BindRec& new_bind(const kzrt::Site& s, const std::string& kind, const std::string& at) {
  uint32_t n = kz.next_n[s.id]++;
  BindRec b;
  b.id = bind_id(kz.module, s.id, n);
  b.site = s.id;
  b.module = kz.module;
  b.kind = kind;
  b.orig_func = s.func;
  b.at = at;
  if (n > 14) {
    b.status = "unsupported";
    b.reason = "more than 15 bindings of one site in one unit (bind id scheme)";
  }
  kz.binds.push_back(b);
  kz.matched.insert(s.id);
  return kz.binds.back();
}

// The builder at a position, with the debug location of the instrumented instruction (or of the function).
void position(IRBuilder<>& B, Instruction* before, const DebugLoc& dl) {
  B.SetInsertPoint(before);
  B.SetCurrentDebugLocation(dl);
}

DebugLoc fn_loc(Function& F) {
  if (DISubprogram* sp = F.getSubprogram()) return DILocation::get(F.getContext(), sp->getLine(), 0, sp);
  return DebugLoc();
}

// Stack slots of pointer parameters as clang emits them before mem2reg: an entry-block alloca whose only
// store is the parameter itself and whose other uses are loads.
std::map<const Value*, Argument*> param_slots(Function& F) {
  std::map<const Value*, Argument*> out;
  for (Instruction& I : F.getEntryBlock()) {
    auto* st = dyn_cast<StoreInst>(&I);
    if (st == nullptr) continue;
    auto* a = dyn_cast<Argument>(st->getValueOperand());
    auto* slot = dyn_cast<AllocaInst>(st->getPointerOperand());
    if (a == nullptr || slot == nullptr || !a->getType()->isPointerTy()) continue;
    bool ok = true;
    for (const User* u : slot->users()) {
      if (u == st) continue;
      if (!isa<LoadInst>(u)) ok = false;
    }
    if (ok) out[slot] = a;
  }
  return out;
}

// ---- insertion (PipelineStart) ---------------------------------------------------------------------------
bool instrument_function(Function& F, const Hooks& h) {
  const std::string fname = F.getName().str();
  std::vector<const kzrt::Site*> here;
  for (const kzrt::Site* s : kz.sites)
    if (s->func == fname) here.push_back(s);
  if (here.empty()) return false;
  bool changed = false;
  IRBuilder<> B(F.getContext());

  std::vector<CallBase*> calls;
  std::vector<StoreInst*> stores;
  std::vector<ReturnInst*> returns;
  for (Instruction& I : instructions(F)) {
    if (auto* cb = dyn_cast<CallBase>(&I)) calls.push_back(cb);
    else if (auto* st = dyn_cast<StoreInst>(&I)) stores.push_back(st);
    else if (auto* r = dyn_cast<ReturnInst>(&I)) returns.push_back(r);
  }

  // 1. calls: the candidates of each call site first (line= is checked against the debug location when the
  //    unit has one; without debug information a line= site binds only if it is the only call of its callee)
  std::map<const kzrt::Site*, std::vector<CallBase*>> cand;
  for (const kzrt::Site* s : here) {
    if (s->callee.empty()) continue;
    std::vector<CallBase*> exact, noloc;
    for (CallBase* cb : calls) {
      auto* callee = dyn_cast<Function>(cb->getCalledOperand()->stripPointerCasts());
      if (callee == nullptr || callee->getName() != s->callee) continue;
      if (s->src_line == 0) exact.push_back(cb);
      else if (dl_line(cb) == s->src_line) exact.push_back(cb);
      else if (dl_line(cb) == 0) noloc.push_back(cb);
    }
    if (!exact.empty()) cand[s] = exact;
    else if (noloc.size() == 1) cand[s] = noloc;
    else if (noloc.size() > 1) {
      BindRec& b = new_bind(*s, std::string("call-") + s->phase, fname + " calls " + s->callee);
      b.status = "unsupported";
      b.reason = "line=" + std::to_string(s->src_line) + " cannot be checked (no debug locations) and " + std::to_string(noloc.size()) +
                 " calls of the callee match";
    }
  }
  for (CallBase* cb : calls) {
    std::vector<const kzrt::Site*> before, after;
    for (const auto& c : cand)
      for (CallBase* x : c.second)
        if (x == cb) (c.first->phase == "before" ? before : after).push_back(c.first);
    if (before.empty() && after.empty()) continue;
    // stable order: the manifest's order
    auto by_id = [](const kzrt::Site* a, const kzrt::Site* b) { return a->id < b->id; };
    std::sort(before.begin(), before.end(), by_id);
    std::sort(after.begin(), after.end(), by_id);
    std::string callee = cb->getCalledOperand()->stripPointerCasts()->getName().str();
    const std::string at = where(cb, fname) + " call " + callee;
    auto lnote = [&](const kzrt::Site* s) -> std::string {
      if (s->src_line == 0 || dl_line(cb) != 0) return "";
      return " line=unverified(no debug location; the only call of " + callee + " in " + fname + ")";
    };
    Operands ops;
    for (unsigned i = 0; i < cb->arg_size(); ++i) ops.arg.push_back(cb->getArgOperand(i));
    ops.call = cb;
    Value* link = B.getInt32(0);
    for (const kzrt::Site* s : before) {
      BindRec& b = new_bind(*s, "call-before", at);
      b.notes += lnote(s);
      if (b.status != "bound") continue;
      position(B, cb, cb->getDebugLoc());
      std::string why;
      Value* aseq = build_obs(h, *s, b.id, B.getInt32(0), ops, B, &b.vals, &b.notes, &why);
      if (aseq == nullptr) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(h, *s, b.id, aseq, B);
      if (isa<ConstantInt>(link)) link = aseq;
      changed = true;
    }
    if (after.empty()) continue;
    if (!isa<CallInst>(cb) || cb->doesNotReturn() || cb->getNextNode() == nullptr) {
      for (const kzrt::Site* s : after) {
        BindRec& b = new_bind(*s, "call-after", at);
        b.status = "unsupported";
        b.reason = "the call does not return to the next instruction (noreturn or invoke)";
      }
      continue;
    }
    if (!cb->getType()->isVoidTy()) ops.ret = cb;  // the call's own result value, used or not
    Instruction* next = cb->getNextNode();
    for (const kzrt::Site* s : after) {
      BindRec& b = new_bind(*s, "call-after", at);
      b.notes += lnote(s);
      if (b.status != "bound") continue;
      position(B, next, cb->getDebugLoc());
      std::string why;
      Value* aseq = build_obs(h, *s, b.id, link, ops, B, &b.vals, &b.notes, &why);
      if (aseq == nullptr) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(h, *s, b.id, aseq, B);
      changed = true;
    }
  }

  // 2. stores through a pointer parameter (phase=effect, no callee): *p = v where p is the parameter (or its
  //    stack slot's value before mem2reg); field stores p->f = v are not this form
  std::map<const Value*, Argument*> slots = param_slots(F);
  for (StoreInst* st : stores) {
    Value* p = st->getPointerOperand();
    Argument* parm = dyn_cast<Argument>(p);
    if (parm == nullptr)
      if (auto* ld = dyn_cast<LoadInst>(p)) {
        auto it = slots.find(ld->getPointerOperand());
        if (it != slots.end()) parm = it->second;
      }
    if (parm == nullptr) continue;
    for (const kzrt::Site* s : here) {
      if (s->phase != "effect" || !s->callee.empty()) continue;
      if (s->src_line != 0 && dl_line(st) != s->src_line) continue;
      BindRec& b = new_bind(*s, "store", where(st, fname) + " store *" + parm->getName().str());
      if (b.status != "bound") continue;
      Operands ops;
      ops.operand0 = st->getValueOperand();
      ops.operand1 = p;
      position(B, st->getNextNode(), st->getDebugLoc());
      std::string why;
      Value* aseq = build_obs(h, *s, b.id, B.getInt32(0), ops, B, &b.vals, &b.notes, &why);
      if (aseq == nullptr) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(h, *s, b.id, aseq, B);
      changed = true;
    }
  }

  // 3. entry (phase=before, no callee) and returns (phase=after, no callee)
  Value* entry_aseq = nullptr;
  Instruction* ip = &*F.getEntryBlock().getFirstInsertionPt();
  while (isa<AllocaInst>(ip)) ip = ip->getNextNode();
  for (const kzrt::Site* s : here) {
    if (s->phase != "before" || !s->callee.empty()) continue;
    BindRec& b = new_bind(*s, "entry", std::string("entry of ") + fname);
    if (b.status != "bound") continue;
    Operands ops;
    for (Argument& a : F.args()) ops.arg.push_back(&a);
    ops.fn = &F;
    position(B, ip, fn_loc(F));
    std::string why;
    Value* aseq = build_obs(h, *s, b.id, B.getInt32(0), ops, B, &b.vals, &b.notes, &why);
    if (aseq == nullptr) {
      b.status = "unsupported";
      b.reason = why;
      continue;
    }
    if (s->has_control) add_ctl(h, *s, b.id, aseq, B);
    if (entry_aseq == nullptr) entry_aseq = aseq;
    changed = true;
  }
  for (const kzrt::Site* s : here) {
    if (s->phase != "after" || !s->callee.empty()) continue;
    if (returns.empty()) {
      BindRec& b = new_bind(*s, "return", std::string("returns of ") + fname);
      b.status = "unsupported";
      b.reason = "the function has no return instruction";
      continue;
    }
    for (ReturnInst* r : returns) {
      BindRec& b = new_bind(*s, "return", where(r, fname) + " return");
      if (b.status != "bound") continue;
      Operands ops;
      for (Argument& a : F.args()) ops.arg.push_back(&a);
      ops.fn = &F;
      ops.ret = r->getReturnValue();
      position(B, r, r->getDebugLoc());
      std::string why;
      Value* aseq = build_obs(h, *s, b.id, entry_aseq != nullptr ? entry_aseq : B.getInt32(0), ops, B, &b.vals, &b.notes, &why);
      if (aseq == nullptr) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(h, *s, b.id, aseq, B);
      changed = true;
    }
  }
  return changed;
}

// Module registration: a constructor kz_rt_module(module, manifest sha256) in llvm.global_ctors.
void add_module_ctor(Module& M, const Hooks& h) {
  LLVMContext& C = M.getContext();
  auto* arr = ConstantDataArray::get(C, ArrayRef<uint8_t>(kz.manifest_sha.data(), kz.manifest_sha.size()));
  auto* gv = new GlobalVariable(M, arr->getType(), true, GlobalValue::PrivateLinkage, arr, "__kzllvm_manifest_sha256");
  Function* f = Function::Create(FunctionType::get(Type::getVoidTy(C), false), GlobalValue::InternalLinkage, "__kzllvm_module_ctor", M);
  f->addFnAttr(Attribute::NoUnwind);
  IRBuilder<> B(BasicBlock::Create(C, "entry", f));
  B.CreateCall(h.module, {B.getInt32(kz.module), gv})->setDoesNotThrow();
  B.CreateRetVoid();
  appendToGlobalCtors(M, f, 65535);
  kz.notes.push_back("module constructor: kz_rt_module(" + std::to_string(kz.module) + ", manifest sha256) in llvm.global_ctors");
}

struct InstrPass : PassInfoMixin<InstrPass> {
  OptimizationLevel level;
  explicit InstrPass(OptimizationLevel l) : level(l) {}
  PreservedAnalyses run(Module& M, ModuleAnalysisManager&) {
    if (!load_manifest()) report_fatal_error("kzllvm: manifest not loaded");
    kz.tu = M.getSourceFileName();
    for (const kzrt::Site& s : kz.m.sites)
      if (ends_with_path(kz.tu, s.tu)) kz.sites.push_back(&s);
    if (kz.sites.empty()) return PreservedAnalyses::all();  // this unit is not touched
    for (const auto& md : kz.modules)
      if (kz.tu.compare(0, md.second.size(), md.second) == 0) {
        kz.module = md.first;
        break;
      }
    if (kz.m.module(kz.module) == nullptr) report_fatal_error(Twine("kzllvm: module ") + Twine(kz.module) + " is not declared in the manifest");
    kz.active = true;
    kz.output = output_name();
    kz.opt_level = "-O" + std::to_string(level.getSpeedupLevel()) + (level.getSizeLevel() ? " (size " + std::to_string(level.getSizeLevel()) + ")" : "");
    for (Function& F : M)
      if (!F.isDeclaration() && F.getName().starts_with("kz_rt_")) {
        kz.refused = true;
        kz.notes.push_back("refused: the unit defines runtime function " + F.getName().str() + " (the runtime is never instrumented)");
        break;
      }
    if (!kz.refused && OptSkipCoverage && M.getNamedMetadata("llvm.gcov") != nullptr) {
      kz.refused = true;
      kz.notes.push_back("not instrumented: unit compiled with -fprofile-arcs (skip-coverage)");
    }
    if (kz.refused) return PreservedAnalyses::all();
    for (Function& F : M)
      for (Instruction& I : instructions(F))
        if (I.getDebugLoc()) {
          kz.has_debug_locs = true;
          break;
        }
    Hooks h = declare_hooks(M);
    bool changed = false;
    std::vector<Function*> fns;
    for (Function& F : M)
      if (!F.isDeclaration()) fns.push_back(&F);
    for (Function* F : fns) changed |= instrument_function(*F, h);
    add_module_ctor(M, h);
    (void)changed;
    return PreservedAnalyses::none();
  }
  static bool isRequired() { return true; }
};

// ---- check (OptimizerLast) and the binding file ---------------------------------------------------------
std::string hex(const uint8_t* p, size_t n) {
  static const char* d = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; ++i) {
    s += d[p[i] >> 4];
    s += d[p[i] & 15];
  }
  return s;
}

void write_binding_file(Module& M, ThinOrFullLTOPhase phase) {
  bool lto_prelink = phase == ThinOrFullLTOPhase::ThinLTOPreLink || phase == ThinOrFullLTOPhase::FullLTOPreLink;
  for (BindRec& b : kz.binds) {
    if (b.status != "bound") continue;
    int total = 0;
    bool other = false;
    for (const auto& c : b.copies) {
      total += c.second;
      if (c.first != b.orig_func) other = true;
    }
    if (lto_prelink) {
      b.notes += " check=deferred-to-lto-link";
      continue;
    }
    if (total == 0) {
      b.status = "removed";
      b.reason = "no hook call left after optimization (the code around it was removed)";
    } else if (total > 1) {
      b.status = "duplicated";
    } else if (other) {
      b.status = "inlined";
    }
  }
  std::string base = kz.tu;
  for (char& c : base)
    if (c == '/') c = '_';
  kz::Digest dh = kz::sha256(kz.output + "|" + kz.tu + "|" + (kz.output.empty() ? std::to_string(getpid()) : ""));
  std::string path = kz.out_dir + "/" + base + "." + hex(dh.data(), 4) + ".kzs";
  std::ofstream o(path);
  if (!o) report_fatal_error(Twine("kzllvm: cannot write ") + path);
  o << "kzs-sites 1\n";
  o << "# " << kVersion << " backend=" << kBackend << " llvm=" << LLVM_VERSION_STRING << " plugin-api=" << LLVM_PLUGIN_API_VERSION
    << " unit=" << kz.tu << " output=" << (kz.output.empty() ? "-" : kz.output) << "\n";
  o << "# manifest=" << kz.manifest_path << " sha256=" << hex(kz.manifest_sha.data(), 32) << " module=" << kz.module << "\n";
  o << "# optimize=" << kz.opt_level << " lto-phase=" << (phase == ThinOrFullLTOPhase::None ? "none" : lto_prelink ? "prelink" : "other")
    << " pic=" << (int)M.getPICLevel() << " pie=" << (int)M.getPIELevel() << " profile_arcs=" << (M.getNamedMetadata("llvm.gcov") ? 1 : 0)
    << " debug_locs=" << (kz.has_debug_locs ? 1 : 0) << " triple=" << M.getTargetTriple().str() << "\n";
  o << "# passes: insertion at PipelineStart, check at OptimizerLast\n";
  for (const std::string& n : kz.notes) o << "# note " << n << "\n";
  for (const BindRec& b : kz.binds) {
    std::string copies;
    for (const auto& c : b.copies) copies += (copies.empty() ? "" : ",") + c.first + ":" + std::to_string(c.second);
    o << "# bind " << b.id << " kind=" << b.kind << " source_func=" << b.orig_func << " final=" << (copies.empty() ? "-" : copies)
      << (b.notes.empty() ? "" : " notes=" + b.notes) << "\n";
    o << "bind " << b.id << " site=" << b.site << " backend=" << kBackend << " module=" << b.module << " at=\"" << b.at
      << (copies.empty() ? "" : " -> " + copies) << "\" status=" << b.status;
    if (b.status == "bound" || b.status == "inlined" || b.status == "duplicated") {
      char vb[16];
      std::snprintf(vb, sizeof vb, "0x%x", b.vals);
      o << " vals=" << vb;
    }
    if (!b.reason.empty()) o << " reason=\"" << b.reason << "\"";
    o << "\n";
  }
  for (const auto& c : kz.ctl_copies) o << "# ctl " << c.first << ":" << c.second << "\n";
  for (const kzrt::Site* s : kz.sites) {
    if (kz.matched.count(s->id)) continue;
    uint32_t id = bind_id(kz.module, s->id, 15);
    o << "bind " << id << " site=" << s->id << " backend=" << kBackend << " module=" << kz.module << " at=\"" << kz.tu << " " << s->func
      << "\" status=unsupported reason=\"" << (kz.refused ? "unit refused" : "no matching function, call or store in the unit") << "\"\n";
  }
}

struct CheckPass : PassInfoMixin<CheckPass> {
  ThinOrFullLTOPhase phase;
  explicit CheckPass(ThinOrFullLTOPhase p) : phase(p) {}
  PreservedAnalyses run(Module& M, ModuleAnalysisManager&) {
    if (!kz.active) return PreservedAnalyses::all();
    for (Function& F : M)
      for (Instruction& I : instructions(F)) {
        auto* cb = dyn_cast<CallBase>(&I);
        if (cb == nullptr) continue;
        auto* callee = dyn_cast<Function>(cb->getCalledOperand()->stripPointerCasts());
        if (callee == nullptr) continue;
        StringRef n = callee->getName();
        if (n != "kz_rt_obs" && n != "kz_rt_obs_str" && n != "kz_rt_ctl") continue;
        auto* b = dyn_cast<ConstantInt>(cb->getArgOperand(1));
        if (b == nullptr) continue;
        uint32_t id = (uint32_t)b->getZExtValue();
        if (n == "kz_rt_ctl") {
          kz.ctl_copies[F.getName().str() + "#" + std::to_string(id)]++;
          continue;
        }
        for (BindRec& r : kz.binds)
          if (r.id == id) r.copies[F.getName().str()]++;
      }
    write_binding_file(M, phase);
    kz.active = false;  // once per unit
    return PreservedAnalyses::all();
  }
  static bool isRequired() { return true; }
};

}  // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "kzllvm", "0.1", [](PassBuilder& PB) {
            PB.registerPipelineStartEPCallback([](ModulePassManager& MPM, OptimizationLevel L) { MPM.addPass(InstrPass(L)); });
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager& MPM, OptimizationLevel, ThinOrFullLTOPhase P) { MPM.addPass(CheckPass(P)); });
          }};
}
