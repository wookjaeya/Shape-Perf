// The ELF reader, the line-table reader and the resolver on real compiler output: the test
// program built by the build's own compiler (DWARF 5), the same source with DWARF 4, and with
// the other compiler (gcc <-> clang).  The line table is cross-checked against the system's
// addr2line, which is independent code; the resolved targets are checked against the source text
// and against what the instructions are.
//   argv: 1 target_prog, 2 dwarf4 build, 3 other-compiler build, 4 target.kzp, 5 target_prog.c, 6 source root
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include <sys/stat.h>
#include <unistd.h>

#include "anchor_resolve.hpp"
#include "dwarf_line.hpp"
#include "elf_image.hpp"
#include "kz/core/container.hpp"
#include "support/kz_test.hpp"

using namespace kz::anchor;

namespace {

bool exists(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0;
}

std::vector<std::string> source_lines(const std::string& path) {
  std::ifstream f(path);
  std::vector<std::string> v;
  std::string l;
  while (std::getline(f, l)) v.push_back(l);
  return v;
}

bool have_addr2line() { return system("command -v addr2line >/dev/null 2>&1") == 0; }

// addr -> "file:line" from addr2line for a batch of addresses.
std::map<uint64_t, std::pair<std::string, unsigned>> addr2line(const std::string& bin, const std::vector<uint64_t>& addrs) {
  std::map<uint64_t, std::pair<std::string, unsigned>> out;
  for (size_t i = 0; i < addrs.size(); i += 100) {
    std::string cmd = "addr2line -e '" + bin + "'";
    for (size_t k = i; k < addrs.size() && k < i + 100; ++k) {
      char b[32];
      std::snprintf(b, sizeof b, " 0x%llx", static_cast<unsigned long long>(addrs[k]));
      cmd += b;
    }
    FILE* p = popen(cmd.c_str(), "r");
    if (p == nullptr) return out;
    char line[4096];
    size_t k = i;
    while (std::fgets(line, sizeof line, p) != nullptr && k < addrs.size() && k < i + 100) {
      std::string s(line);
      while (!s.empty() && (s.back() == '\n')) s.pop_back();
      const size_t sp = s.find(" (discriminator");
      if (sp != std::string::npos) s = s.substr(0, sp);
      const size_t colon = s.rfind(':');
      if (colon != std::string::npos) {
        const unsigned ln = static_cast<unsigned>(std::atoi(s.c_str() + colon + 1));
        out[addrs[k]] = {s.substr(0, colon), ln};
      }
      ++k;
    }
    pclose(p);
  }
  return out;
}

void check_binary(const std::string& bin, const std::string& source, const char* what) {
  if (!exists(bin)) {
    std::printf("%s: %s not built, skipped\n", what, bin.c_str());
    return;
  }
  kz::Bytes bytes;
  KZ_CHECK_OK(kz::read_whole_file(bin, &bytes));
  ElfImage elf;
  KZ_CHECK_OK(ElfImage::parse(bytes, &elf));
  KZ_CHECK(elf.position_independent());
  KZ_CHECK_EQ(elf.build_id_hex().size(), size_t(40));
  LineTable lines;
  KZ_CHECK_OK(LineTable::parse(elf, &lines));
  KZ_CHECK(lines.rows().size() > 50);
  const std::vector<uint32_t> ids = lines.files_matching("target_prog.c");
  KZ_CHECK_EQ(ids.size(), size_t(1));

  // The functions of the program are in the symbol table, local and global alike.
  for (const char* name : {"tp_send", "tp_wrap_a", "tp_wrap_b", "tp_publish", "tp_task_create", "tp_task_entry", "tp_get_identity", "main", "worker1", "tp_thread_start"})
    KZ_CHECK_EQ(elf.find_functions(name).size(), size_t(1));

  // Cross-check with addr2line at the start of every row of the program's own file.
  if (have_addr2line()) {
    std::vector<uint64_t> addrs;
    std::vector<unsigned> want;
    const auto& rows = lines.rows();
    for (size_t i = 0; i + 1 < rows.size(); ++i) {
      if (rows[i].end_sequence || rows[i].addr == rows[i + 1].addr) continue;  // rows at one address are ambiguous
      if (std::find(ids.begin(), ids.end(), rows[i].file) == ids.end()) continue;
      addrs.push_back(rows[i].addr);
      want.push_back(rows[i].line);
    }
    KZ_CHECK(addrs.size() > 40);
    const auto got = addr2line(bin, addrs);
    int compared = 0;
    for (size_t i = 0; i < addrs.size(); ++i) {
      auto it = got.find(addrs[i]);
      if (it == got.end()) { KZ_CHECK(false); continue; }
      KZ_CHECK_EQ(it->second.second, want[i]);
      ++compared;
    }
    KZ_CHECK_EQ(compared, static_cast<int>(addrs.size()));
  } else {
    std::printf("%s: addr2line not found, the cross-check is skipped\n", what);
  }

  // Resolve the sites the test profile uses and check them against the instructions and the source.
  const std::vector<std::string> src = source_lines(source);
  size_t publish_line = 0;
  for (size_t i = 0; i < src.size(); ++i)
    if (src[i].find("tp_get_identity(&g_identity)") != std::string::npos) publish_line = i + 1;
  KZ_CHECK(publish_line != 0);

  ResolvedTarget t;
  KZ_CHECK_OK(resolve_entry(elf, lines, "tp_send", "", &t));
  KZ_CHECK_EQ(t.delta, 4u);                                       // after endbr64
  KZ_CHECK_EQ(t.insn[0], uint8_t(0x55));                          // push %rbp
  KZ_CHECK_EQ(t.insn_class, uint16_t(KZ_INSN_CLASS_EMULATED));
  KZ_CHECK_EQ(t.func, std::string("tp_send"));
  KZ_CHECK_EQ(t.vaddr, elf.find_functions("tp_send")[0]->vaddr + 4);

  KZ_CHECK_OK(resolve_after_call(elf, lines, "target_prog.c", static_cast<uint32_t>(publish_line), "tp_get_identity", &t));
  KZ_CHECK_EQ(t.func, std::string("tp_publish"));
  KZ_CHECK_EQ(t.line, static_cast<uint32_t>(publish_line));
  // The byte before the return site ends a call to tp_get_identity; the instruction at the site belongs to the next source line.
  uint8_t call[5];
  KZ_CHECK(elf.read_vaddr(t.vaddr - 5, 5, call));
  KZ_CHECK_EQ(call[0], uint8_t(0xe8));
  int32_t rel;
  std::memcpy(&rel, call + 1, 4);
  KZ_CHECK_EQ(static_cast<uint64_t>(static_cast<int64_t>(t.vaddr) + rel), elf.find_functions("tp_get_identity")[0]->vaddr);
  const LineRow* after = lines.row_at(t.vaddr);
  KZ_CHECK(after != nullptr);
  KZ_CHECK_EQ(after->line, static_cast<uint32_t>(publish_line + 1));
  const LineRow* before = lines.row_at(t.vaddr - 1);
  KZ_CHECK(before != nullptr);
  KZ_CHECK_EQ(before->line, static_cast<uint32_t>(publish_line));

  // The same name twice is not a thing in this program, but a name that is not there is refused, and so is a line without that call.
  KZ_CHECK_ERR(resolve_entry(elf, lines, "tp_nosuch", "", &t), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_after_call(elf, lines, "target_prog.c", static_cast<uint32_t>(publish_line), "tp_send", &t), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_after_call(elf, lines, "target_prog.c", static_cast<uint32_t>(publish_line + 1), "tp_get_identity", &t), kz::Err::Invalid);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 7) {
    std::fprintf(stderr, "usage: test_probe_anchor_compilers <target_prog> <dwarf4> <other-compiler> <profile> <source> <root>\n");
    return 2;
  }
  check_binary(argv[1], argv[5], "default compiler, DWARF 5");
  check_binary(argv[2], argv[5], "DWARF 4");
  check_binary(argv[3], argv[5], "other compiler");
  return kztest::finish("test_probe_anchor_compilers");
}
