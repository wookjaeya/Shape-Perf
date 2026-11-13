// The entry-reached target (KZ_TARGET_KIND_ENTRY_REACHED, enter=) on real compiler output: tailjump_prog.c
// built at -O2 without a line table by the build's compiler and by the other one (gcc <-> clang).  The
// resolver is checked against what the instructions are (objdump, independent code, where present), the
// refusals (two transfers, a PLT stub, no transfer) are checked, and a profile with enter= and fetch slots
// goes through kz anchor's basis and the plan.  Where the kernel lets the test attach uprobes and a tracefs
// is available (KZ_TRACEFS, else /sys/kernel/tracing), the program runs under kz observe: the search's
// events are kept only for the calls through the lookup, with the name read as a string, and the finishing
// call's token gives the id.  Unit-test values only.
//   argv: as every test_probe_* test (tests/cfs-free/CMakeLists.txt); 7 kz, 8 the binary directory
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "anchor_resolve.hpp"
#include "basis.hpp"
#include "dwarf_line.hpp"
#include "elf_image.hpp"
#include "kz/core/container.hpp"
#include "profile.hpp"
#include "support/kz_test.hpp"

using namespace kz::anchor;

namespace {

int g_skipped_parts = 0;
std::string g_source_root, g_kz, g_bindir;

bool exists(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0;
}

std::string slurp(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

std::string run_text(const std::string& cmd) {
  std::string out;
  FILE* p = popen(cmd.c_str(), "r");
  if (p == nullptr) return out;
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
  pclose(p);
  return out;
}

// objdump's view of a function: the address of the instruction that is a call or jump to <callee>, and the
// address of the instruction after it.  0 if none (or no objdump).
bool objdump_transfer(const std::string& bin, const std::string& func, const std::string& callee, const char* mnemonic, uint64_t* at, uint64_t* next) {
  const std::string text = run_text("objdump -d -w --no-show-raw-insn --disassemble=" + func + " '" + bin + "' 2>/dev/null");
  std::istringstream in(text);
  std::string line;
  bool found = false;
  *at = *next = 0;
  while (std::getline(in, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos || line.find('<') == 0) continue;
    char* end = nullptr;
    const uint64_t a = std::strtoull(line.c_str(), &end, 16);
    if (end == line.c_str() || *end != ':') continue;
    if (found && *next == 0) *next = a;
    if (!found && line.find(std::string(mnemonic)) != std::string::npos && line.find("<" + callee + ">") != std::string::npos) {
      found = true;
      *at = a;
    }
  }
  return found;
}

void check_binary(const std::string& bin, const char* what) {
  if (!exists(bin)) {
    std::printf("%s: %s not built, skipped\n", what, bin.c_str());
    ++g_skipped_parts;
    return;
  }
  kz::Bytes bytes;
  KZ_CHECK_OK(kz::read_whole_file(bin, &bytes));
  ElfImage elf;
  KZ_CHECK_OK(ElfImage::parse(bytes, &elf));
  LineTable lines;
  KZ_CHECK_OK(LineTable::parse(elf, &lines));
  KZ_CHECK(lines.rows().empty());  // a release-like build: no line table
  const std::vector<const ElfFunction*> search = elf.find_functions("find_by_name");
  const std::vector<const ElfFunction*> fin = elf.find_functions("finish");
  KZ_CHECK(search.size() == 1 && fin.size() == 1);
  if (search.size() != 1 || fin.size() != 1) return;
  const bool have_objdump = system("command -v objdump >/dev/null 2>&1") == 0;

  // the lookup reaches the search by a tail jump: an after-call point would find no call ...
  ResolvedTarget t;
  KZ_CHECK_ERR(resolve_call_in_function(elf, "lookup_by_name", "find_by_name", &t), kz::Err::Invalid);
  // ... the entry-reached target is the search's entry, with the lookup's own return address
  ReachedPath path;
  KZ_CHECK_OK(resolve_entry_reached(elf, lines, "lookup_by_name", {"find_by_name"}, &t, &path));
  KZ_CHECK_EQ(t.kind, KZ_TARGET_KIND_ENTRY_REACHED);
  KZ_CHECK_EQ(t.func, std::string("find_by_name"));
  KZ_CHECK_EQ(t.func_vaddr, search[0]->vaddr);
  KZ_CHECK_EQ(t.delta, 4u);  // after endbr64 (-fcf-protection), as every entry target
  KZ_CHECK(path.ret_dynamic && path.hops == 1 && path.hop_is_call.size() == 1 && !path.hop_is_call[0]);
  KZ_CHECK_EQ(path.chain, std::string("lookup_by_name>find_by_name"));
  if (have_objdump) {
    uint64_t at = 0, next = 0;
    KZ_CHECK(objdump_transfer(bin, "lookup_by_name", "find_by_name", "jmp", &at, &next));
  }
  // through the finishing call: the return address is the one after that call, the same for every caller
  KZ_CHECK_OK(resolve_entry_reached(elf, lines, "lookup_by_name", {"find_by_name", "finish"}, &t, &path));
  const uint64_t finish_ret = path.expect_ret;
  KZ_CHECK_EQ(t.func, std::string("finish"));
  KZ_CHECK(!path.ret_dynamic && path.hops == 2 && !path.hop_is_call[0] && path.hop_is_call[1]);
  if (have_objdump) {
    uint64_t at = 0, next = 0;
    KZ_CHECK(objdump_transfer(bin, "find_by_name", "finish", "call", &at, &next));
    KZ_CHECK_EQ(path.expect_ret, next);
  }
  uint8_t call[5] = {0};
  KZ_CHECK(elf.read_vaddr(path.expect_ret - 5, 5, call));
  int32_t rel = 0;
  std::memcpy(&rel, call + 1, 4);
  KZ_CHECK(call[0] == 0xe8 && path.expect_ret + static_cast<uint64_t>(static_cast<int64_t>(rel)) == fin[0]->vaddr);
  // refusals: two transfers in one function, a PLT stub (another file), no transfer, an unknown function
  KZ_CHECK_ERR(resolve_entry_reached(elf, lines, "two_calls", {"find_by_name"}, &t, &path), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_entry_reached(elf, lines, "via_libc", {"puts"}, &t, &path), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_entry_reached(elf, lines, "lookup_by_name", {"finish"}, &t, &path), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_entry_reached(elf, lines, "lookup_by_name", {"no_such_function"}, &t, &path), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_entry_reached(elf, lines, "lookup_by_name", {}, &t, &path), kz::Err::Invalid);

  // the profile with enter= and fetch slots: basis, then plan
  kz::plan::Profile prof;
  KZ_CHECK_OK(kz::plan::parse_profile(slurp(g_source_root + "/tests/cfs-free/target/tailjump.kzp"), &prof));
  kz::plan::BasisOptions o;
  o.binary_path = bin;
  kz::Bytes basis_bytes;
  KZ_CHECK_OK(kz::plan::build_basis(bytes, prof, o, &basis_bytes, nullptr));
  kz::plan::LoadedBasis lb;
  KZ_CHECK_OK(kz::plan::load_basis_bytes(basis_bytes, &lb));
  std::vector<kz::trace::SiteSpec> sites;
  KZ_CHECK_OK(kz::plan::make_plan(prof, lb, &sites));
  KZ_CHECK_EQ(sites.size(), 4u);
  if (sites.size() == 4) {
    KZ_CHECK_EQ(sites[2].via_symbol, std::string("lookup_by_name"));
    KZ_CHECK(sites[2].via_ret_dynamic && sites[2].symbol == "find_by_name" && sites[2].vaddr == search[0]->vaddr + 4);
    KZ_CHECK(!sites[3].via_ret_dynamic && sites[3].via_expect_ret == finish_ret && sites[3].symbol == "finish");
    KZ_CHECK(sites[2].slots.size() == 2 && sites[2].slots[0].src == KZ_SLOT_SRC_STR && sites[2].slots[0].index == KZ_REG_RSI);
    KZ_CHECK(sites[3].slots[0].src == KZ_SLOT_SRC_MEM && sites[3].slots[0].offset == 16 && sites[3].slots[0].width == 4);
  }
  // a plan without the lookup's return site cannot attribute the search's events: refused
  kz::plan::Profile bad;
  std::string text = slurp(g_source_root + "/tests/cfs-free/target/tailjump.kzp");
  const size_t at = text.find("site lookup.ret");
  text.erase(at, text.find('\n', at) - at + 1);
  KZ_CHECK_ERR(kz::plan::parse_profile(text, &bad), kz::Err::Invalid);
}

// The run: kz anchor, kz observe, then the run container's events of the two reached sites.
void observe(const std::string& bin) {
  std::string tracefs;
  if (const char* e = std::getenv("KZ_TRACEFS")) tracefs = e;
  if (tracefs.empty() && access("/sys/kernel/tracing/uprobe_events", W_OK) == 0) tracefs = "/sys/kernel/tracing";
  if (geteuid() != 0 || tracefs.empty() || !exists(bin) || access(g_kz.c_str(), X_OK) != 0) {
    std::printf("observe: not root, no tracefs (KZ_TRACEFS) or no binary: the run part skipped\n");
    ++g_skipped_parts;
    return;
  }
  const std::string dir = g_bindir + "/anchor_reached_run";
  KZ_CHECK_EQ(system(("rm -rf '" + dir + "' && mkdir -p '" + dir + "' && cp '" + bin + "' '" + dir + "/tailjump_prog'").c_str()), 0);
  const std::string plan = g_source_root + "/tests/cfs-free/target/tailjump.kzp";
  const std::string anchor = "'" + g_kz + "' anchor '" + dir + "/tailjump_prog' '" + plan + "' -o '" + dir + "/basis.kz' >'" + dir + "/anchor.txt' 2>&1";
  KZ_CHECK_EQ(system(anchor.c_str()), 0);
  const std::string obs = "'" + g_kz + "' observe '" + dir + "/basis.kz' '" + plan + "' -o '" + dir + "/run.kzt' --tracefs '" + tracefs + "' --proc-capture status -- '" + dir +
                          "/tailjump_prog' >'" + dir + "/observe.txt' 2>&1";
  const int rc = system(obs.c_str());
  if (WEXITSTATUS(rc) == 3) {
    std::printf("observe: kz observe could not attach (see %s/observe.txt): the run part skipped\n", dir.c_str());
    ++g_skipped_parts;
    return;
  }
  KZ_CHECK_EQ(WEXITSTATUS(rc), 0);
  kz::Container c;
  KZ_CHECK_OK(kz::Container::read_file(dir + "/run.kzt", &c));
  std::vector<kz_raw_ev> ev;
  std::vector<kz_ev_str> strs;
  std::vector<kz_via> vias;
  std::vector<kz_slot_fetch> fetches;
  KZ_CHECK_OK(c.read_section(KZ_SEC_RAW_EVENTS, &ev));
  KZ_CHECK_OK(c.read_section(KZ_SEC_EVENT_STRINGS, &strs));
  KZ_CHECK_OK(c.read_section(KZ_SEC_VIA, &vias));
  KZ_CHECK_OK(c.read_section(KZ_SEC_SLOT_FETCHES, &fetches));
  KZ_CHECK_EQ(vias.size(), 2u);
  KZ_CHECK_EQ(fetches.size(), 2u);
  std::vector<std::string> names;
  for (const kz_ev_str& s : strs) {
    std::string t;
    c.string(s.text, &t);
    names.push_back(t);
  }
  size_t search = 0, found = 0, lookups = 0;
  uint64_t id = 0;
  for (const kz_raw_ev& e : ev) {
    if (e.site == 0) ++lookups;
    if (e.site == 2) ++search;
    if (e.site == 3) {
      ++found;
      id = e.a[0];
    }
  }
  // two lookups (alpha, gamma) reach the search; other_lookup and two_calls reach it too, and are not kept
  KZ_CHECK_EQ(lookups, 2u);
  KZ_CHECK_EQ(search, 2u);
  KZ_CHECK(names.size() == 2 && names[0] == "alpha" && names[1] == "gamma");
  // the finishing call is made for alpha only (gamma is not found); two_calls' alpha is not through the lookup
  KZ_CHECK_EQ(found, 1u);
  KZ_CHECK_EQ(id, 0x30004u);  // 0x30001 + type 3 (the lookup's type), stored in the token's id field
  const std::string record = slurp(dir + "/run.kzt.record.txt");
  KZ_CHECK(record.find("decode.via.site2.lookup.search\t2 kept") != std::string::npos || record.find("2 kept (attributed") != std::string::npos);
  KZ_CHECK(record.find("observer.fetch") != std::string::npos && record.find("removed 2 probe event(s)") != std::string::npos);
  // nothing left behind: no probe event of the observer's group (kz<pid>/s<site>)
  const std::string events = slurp(tracefs + "/uprobe_events");
  bool left = false;
  for (size_t p = events.find(":kz"); p != std::string::npos; p = events.find(":kz", p + 1))
    left = left || (p + 3 < events.size() && events[p + 3] >= '0' && events[p + 3] <= '9');
  KZ_CHECK(!left);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 9) {
    std::fprintf(stderr, "usage: test_probe_anchor_reached <as every test_probe_* test>\n");
    return 2;
  }
  g_source_root = argv[6];
  g_kz = argv[7];
  g_bindir = argv[8];
  check_binary(g_bindir + "/tailjump_prog_cc", "build compiler");
  check_binary(g_bindir + "/tailjump_prog_alt", "other compiler");
  observe(g_bindir + "/tailjump_prog_cc");
  const int rc = kztest::finish("test_probe_anchor_reached");
  if (rc == 0 && g_skipped_parts != 0) return 77;
  return rc;
}
