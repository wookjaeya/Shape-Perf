// Unit tests of the ELF reader, the DWARF line-table reader and the anchor resolver, on
// synthetic files whose every byte is known (support/elf_synth.hpp).  The same readers are
// cross-checked on compiler output in test_probe_anchor_compilers.
#include <cstdlib>
#include <random>

#include "anchor_resolve.hpp"
#include "dwarf_line.hpp"
#include "elf_image.hpp"
#include "support/elf_synth.hpp"
#include "support/kz_test.hpp"

using namespace kz::anchor;
using namespace kzsyn;

namespace {

// Special opcode for (address advance, line advance) with the default header parameters.
uint8_t special_for(int addr_inc, int line_inc, int8_t line_base = -5, uint8_t line_range = 14, uint8_t opcode_base = 13) {
  return static_cast<uint8_t>((line_inc - line_base) + line_range * addr_inc + opcode_base);
}

ElfImage parse_ok(const Buf& b) {
  ElfImage e;
  KZ_CHECK_OK(ElfImage::parse(b, &e));
  return e;
}

void test_elf_basics() {
  SynthElf s;
  s.text = {0x90, 0x90, 0x90, 0x90, 0xc3, 0xc3, 0xc3, 0xc3};
  s.funcs = {{"zeta", 0x1004, 4, STB_LOCAL}, {"alpha", 0x1000, 4, STB_GLOBAL}, {"alias", 0x1000, 4, STB_WEAK}};
  s.build_id = std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14", 20);
  ElfImage e = parse_ok(s.build());
  KZ_CHECK_EQ(e.build_id_hex(), std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  KZ_CHECK(e.position_independent());
  KZ_CHECK_EQ(e.segments().size(), size_t(1));
  KZ_CHECK_EQ(e.functions().size(), size_t(3));
  KZ_CHECK_EQ(e.functions()[0].vaddr, uint64_t(0x1000));  // sorted by (vaddr, name)
  KZ_CHECK_EQ(e.functions()[0].name, std::string("alias"));
  KZ_CHECK_EQ(e.functions()[1].name, std::string("alpha"));
  KZ_CHECK_EQ(e.functions()[2].name, std::string("zeta"));
  KZ_CHECK_EQ(e.functions()[2].bind, uint8_t(KZ_SYMBOL_BIND_LOCAL));
  KZ_CHECK_EQ(e.functions()[0].bind, uint8_t(KZ_SYMBOL_BIND_WEAK));
  KZ_CHECK_EQ(e.find_functions("alpha").size(), size_t(1));
  KZ_CHECK_EQ(e.find_functions("nosuch").size(), size_t(0));
  // function_at prefers the stronger binding among aliases
  KZ_CHECK_EQ(e.function_at(0x1002)->name, std::string("alpha"));
  KZ_CHECK_EQ(e.function_at(0x1004)->name, std::string("zeta"));
  KZ_CHECK(e.function_at(0x1008) == nullptr);
  uint64_t off = 0;
  KZ_CHECK(e.vaddr_to_offset(0x1004, &off));
  KZ_CHECK_EQ(off, uint64_t(0x1004));
  KZ_CHECK(!e.vaddr_to_offset(0x10, &off) || off == 0x10);  // inside the headers: part of the one segment
  uint8_t buf[8];
  KZ_CHECK(e.read_vaddr(0x1004, 4, buf));
  KZ_CHECK_EQ(buf[0], uint8_t(0xc3));
  KZ_CHECK(!e.read_vaddr(0x1000000, 4, buf));            // no segment there
  KZ_CHECK(!e.read_vaddr(e.segments()[0].vaddr + e.segments()[0].filesz - 1, 4, buf));  // would run past the segment's file bytes
  KZ_CHECK(e.section(".text").size == 8);
  KZ_CHECK(e.section(".nosuch").empty());
  // no build-id, no symbols
  SynthElf bare;
  bare.text = {0x90};
  bare.with_symtab = false;
  ElfImage b = parse_ok(bare.build());
  KZ_CHECK_EQ(b.build_id_hex(), std::string(""));
  KZ_CHECK_EQ(b.functions().size(), size_t(0));
}

void test_elf_rejects() {
  SynthElf s;
  s.text = {0x90, 0xc3};
  s.funcs = {{"f", 0x1000, 2}};
  const Buf good = s.build();
  ElfImage e;
  // every truncation either parses (cut inside trailing section headers is not possible: they end the file) or is rejected, never crashes
  for (size_t cut = 0; cut < good.size(); ++cut) {
    Buf c(good.begin(), good.begin() + cut);
    ElfImage x;
    (void)ElfImage::parse(c, &x);
  }
  { Buf c = good; c[4] = 1; KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }            // ELFCLASS32
  { Buf c = good; c[5] = 2; KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }            // big endian
  { Buf c = good; c[18] = 40; KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }          // e_machine ARM
  { Buf c = good; c[16] = 1; KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }           // ET_REL
  { Buf c = good; c[0] = 'X'; KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }
  { Buf c(10, 0); KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }
  // a section header table that points outside the file
  { Buf c = good; const uint64_t huge = 0x7fffffffffffull; std::memcpy(c.data() + 40, &huge, 8); KZ_CHECK_ERR(ElfImage::parse(c, &e), kz::Err::Invalid); }
  // random single-byte damage never crashes
  std::mt19937_64 rng(99);
  for (int i = 0; i < 4000; ++i) {
    Buf c = good;
    c[rng() % c.size()] = static_cast<uint8_t>(rng());
    ElfImage x;
    (void)ElfImage::parse(c, &x);
  }
}

// Builds an ELF with one line program and returns the table.
LineTable table_of(const Buf& unit_bytes, kz::Status* st = nullptr, const Buf& line_str = Buf()) {
  SynthElf s;
  s.text = {0x90, 0xc3};
  s.debug_line = unit_bytes;
  s.debug_line_str = line_str;
  ElfImage e;
  KZ_CHECK_OK(ElfImage::parse(s.build(), &e));
  LineTable t;
  kz::Status r = LineTable::parse(e, &t);
  if (st != nullptr) *st = r;
  else KZ_CHECK_OK(r);
  return t;
}

LineProgram basic_program(int version) {
  LineProgram p;
  p.version = version;
  p.dirs = {"/src", "/src/inc"};
  p.files = {{"a.c", 0}, {"a.c", 0}, {"b.h", 1}};  // v5 indices 0..2; v<5: 1..3 after the implicit slot
  if (version < 5) p.files = {{"a.c", 0}, {"b.h", 1}};
  return p;
}

void test_line_program_forms() {
  for (int version : {2, 3, 4, 5}) {
    LineProgram p = basic_program(version);
    p.set_address(0x1000);
    p.advance_line(9);          // line 10
    p.copy();                   // row: 0x1000, line 10
    p.special(special_for(4, 2));   // 0x1004, line 12
    p.set_column(7);
    p.special(special_for(3, 0));   // 0x1007, line 12 col 7
    p.const_add_pc();           // address += (255-13)/14 = 17
    p.copy();                   // 0x1018 line 12
    p.advance_pc(8);            // 0x1020
    p.negate_stmt();
    p.set_file(version >= 5 ? 2 : 2);   // b.h
    p.advance_line(-3);         // line 9
    p.copy();                   // 0x1020 line 9 file b.h, not a statement
    p.fixed_advance_pc(0x100);  // 0x1120
    p.advance_pc(4);
    p.end_sequence();           // end row at 0x1124
    LineTable t = table_of(p.build());
    KZ_CHECK_EQ(t.rows().size(), size_t(6));
    const LineRow* r = t.rows().data();
    KZ_CHECK_EQ(r[0].addr, uint64_t(0x1000)); KZ_CHECK_EQ(r[0].line, 10u); KZ_CHECK(r[0].is_stmt); KZ_CHECK_EQ(r[0].col, 0u);
    KZ_CHECK_EQ(r[1].addr, uint64_t(0x1004)); KZ_CHECK_EQ(r[1].line, 12u);
    KZ_CHECK_EQ(r[2].addr, uint64_t(0x1007)); KZ_CHECK_EQ(r[2].line, 12u); KZ_CHECK_EQ(r[2].col, 7u);
    KZ_CHECK_EQ(r[3].addr, uint64_t(0x1018)); KZ_CHECK_EQ(r[3].line, 12u);
    KZ_CHECK_EQ(r[4].addr, uint64_t(0x1020)); KZ_CHECK_EQ(r[4].line, 9u); KZ_CHECK(!r[4].is_stmt);
    KZ_CHECK(r[5].end_sequence); KZ_CHECK_EQ(r[5].addr, uint64_t(0x1124));
    // DWARF 5 lists the compilation directory as directory 0; before it, directory 0 is implicit and unknown here.
    KZ_CHECK_EQ(t.files()[r[0].file], version >= 5 ? std::string("/src/a.c") : std::string("a.c"));
    KZ_CHECK_EQ(t.files()[r[4].file], std::string("/src/inc/b.h"));
    // lookups
    KZ_CHECK_EQ(t.row_at(0x1006)->line, 12u);
    KZ_CHECK_EQ(t.row_at(0x1000)->line, 10u);
    KZ_CHECK(t.row_at(0xfff) == nullptr);
    KZ_CHECK(t.row_at(0x1124) == nullptr);  // the end row covers nothing
    const std::vector<uint32_t> a_ids = t.files_matching("a.c");
    KZ_CHECK_EQ(a_ids.size(), size_t(1));
    // line 12 appears in rows 1,2,3: one range [0x1004, 0x1020) -- consecutive rows of one (file, line) merge
    const std::vector<LineRange> r12 = t.ranges_of(a_ids, 12);
    KZ_CHECK_EQ(r12.size(), size_t(1));
    KZ_CHECK_EQ(r12[0].begin, uint64_t(0x1004));
    KZ_CHECK_EQ(r12[0].end, uint64_t(0x1020));
    KZ_CHECK_EQ(r12[0].col, 0u);
    KZ_CHECK_EQ(t.ranges_of(a_ids, 10).size(), size_t(1));
    // line 9 of b.h has no is_stmt row: not a range
    KZ_CHECK_EQ(t.ranges_of(t.files_matching("b.h"), 9).size(), size_t(0));
    KZ_CHECK_EQ(t.ranges_of(a_ids, 999).size(), size_t(0));
  }
}

void test_line_strp_and_dwarf64() {
  for (int mode = 0; mode < 3; ++mode) {
    Buf strs;
    LineProgram p = basic_program(5);
    p.use_line_strp = mode != 0;
    p.dwarf64 = mode == 2;
    p.line_str = &strs;
    p.set_address(0x2000);
    p.advance_line(4);
    p.copy();
    p.advance_pc(16);
    p.end_sequence();
    LineTable t = table_of(p.build(), nullptr, strs);
    KZ_CHECK_EQ(t.rows().size(), size_t(2));
    KZ_CHECK_EQ(t.files()[t.rows()[0].file], std::string("/src/a.c"));
    KZ_CHECK_EQ(t.rows()[0].line, 5u);
  }
}

void test_two_units_and_sequence_order() {
  // Two compilation units; the second one's sequence is at a lower address.  Rows come out ordered by sequence start.
  LineProgram a = basic_program(5);
  a.set_address(0x3000); a.copy(); a.advance_pc(8); a.end_sequence();
  LineProgram b = basic_program(5);
  b.files = {{"c.c", 0}, {"c.c", 0}};
  b.set_address(0x1000); b.advance_line(99); b.copy(); b.advance_pc(8); b.end_sequence();
  Buf both = a.build();
  const Buf bb = b.build();
  both.insert(both.end(), bb.begin(), bb.end());
  LineTable t = table_of(both);
  KZ_CHECK_EQ(t.rows().size(), size_t(4));
  KZ_CHECK_EQ(t.rows()[0].addr, uint64_t(0x1000));
  KZ_CHECK_EQ(t.rows()[0].line, 100u);
  KZ_CHECK_EQ(t.rows()[2].addr, uint64_t(0x3000));
  KZ_CHECK_EQ(t.files()[t.rows()[0].file], std::string("/src/c.c"));
  KZ_CHECK_EQ(t.files()[t.rows()[2].file], std::string("/src/a.c"));
  // files of the two units are numbered once per distinct path
  KZ_CHECK_EQ(t.files().size(), size_t(3));  // /src/a.c, /src/inc/b.h, /src/c.c
}

void test_ranges_and_suffixes() {
  LineProgram p = basic_program(5);
  p.dirs = {"/work/proj", "/work/proj/modules/sb/src", "/work/proj/other"};
  p.files = {{"task.c", 1}, {"task.c", 1}, {"mytask.c", 2}, {"task.c", 2}};
  p.set_address(0x1000);
  p.set_file(1);
  p.advance_line(19);         // line 20
  p.copy();
  p.advance_pc(8);
  p.advance_line(1);          // line 21
  p.copy();
  p.advance_pc(8);
  p.advance_line(-1);         // line 20 again, later in the code
  p.copy();
  p.advance_pc(8);
  p.end_sequence();
  LineTable t = table_of(p.build());
  // "task.c" matches modules/sb/src/task.c and other/task.c but not mytask.c
  KZ_CHECK_EQ(t.files_matching("task.c").size(), size_t(2));
  KZ_CHECK_EQ(t.files_matching("sb/src/task.c").size(), size_t(1));
  KZ_CHECK_EQ(t.files_matching("ask.c").size(), size_t(0));      // not at a component boundary
  KZ_CHECK_EQ(t.files_matching("/work/proj/modules/sb/src/task.c").size(), size_t(1));
  KZ_CHECK_EQ(t.files_matching("nosuch.c").size(), size_t(0));
  // line 20 occurs in two separate stretches of code
  const std::vector<LineRange> r = t.ranges_of(t.files_matching("sb/src/task.c"), 20);
  KZ_CHECK_EQ(r.size(), size_t(2));
  KZ_CHECK_EQ(r[0].begin, uint64_t(0x1000));
  KZ_CHECK_EQ(r[0].end, uint64_t(0x1008));
  KZ_CHECK_EQ(r[1].begin, uint64_t(0x1010));
  KZ_CHECK_EQ(r[1].end, uint64_t(0x1018));
}

void test_line_program_rejects() {
  auto status_of = [&](const Buf& unit) {
    kz::Status st;
    (void)table_of(unit, &st);
    return st;
  };
  {
    LineProgram p = basic_program(5);
    p.set_address(0x1000); p.copy();  // no end_sequence
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    LineProgram p = basic_program(5);
    p.version = 6;
    p.set_address(0x1000); p.copy(); p.end_sequence();
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    LineProgram p = basic_program(5);
    p.max_ops = 4;  // VLIW
    p.set_address(0x1000); p.copy(); p.end_sequence();
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    LineProgram p = basic_program(5);
    p.line_range = 0;
    p.set_address(0x1000); p.copy(); p.end_sequence();
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    LineProgram p = basic_program(5);
    p.set_address(0x1000); p.set_file(77); p.copy(); p.end_sequence();  // a file the header does not list
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    // a set_address operand that is not 8 bytes
    LineProgram p = basic_program(5);
    Buf o; put32(o, 0x1000);
    p.extended(2, o);
    p.copy(); p.end_sequence();
    KZ_CHECK_ERR(status_of(p.build()), kz::Err::Invalid);
  }
  {
    // an unknown extended opcode is skipped by its length; an unknown standard opcode by the header's operand count
    LineProgram p = basic_program(5);
    p.opcode_base = 15;  // two extra standard opcodes (13 and 14), each with one operand
    p.set_address(0x1000);
    Buf junk; put32(junk, 0xdeadbeef);
    p.extended(0x80, junk);
    put8(p.program, 13); uleb(p.program, 12345);  // unknown standard opcode with one operand
    p.advance_line(2);
    p.copy();
    p.advance_pc(4);
    p.end_sequence();
    LineTable t = table_of(p.build());
    KZ_CHECK_EQ(t.rows().size(), size_t(2));
    KZ_CHECK_EQ(t.rows()[0].line, 3u);
  }
  // truncations and damage never crash and never read outside the section
  LineProgram p = basic_program(5);
  p.set_address(0x1000); p.advance_line(5); p.copy(); p.advance_pc(8); p.end_sequence();
  const Buf good = p.build();
  for (size_t cut = 0; cut < good.size(); ++cut) {
    Buf c(good.begin(), good.begin() + cut);
    kz::Status st;
    (void)table_of(c, &st);
  }
  std::mt19937_64 rng(5);
  for (int i = 0; i < 3000; ++i) {
    Buf c = good;
    for (int k = 0; k < 1 + static_cast<int>(rng() % 3); ++k) c[rng() % c.size()] = static_cast<uint8_t>(rng());
    kz::Status st;
    (void)table_of(c, &st);
  }
  // an image without a line table gives an empty table, not an error
  SynthElf s;
  s.text = {0x90};
  ElfImage e = parse_ok(s.build());
  LineTable t;
  KZ_CHECK_OK(LineTable::parse(e, &t));
  KZ_CHECK_EQ(t.rows().size(), size_t(0));
}

// ---- the resolver -------------------------------------------------------------------------------------------

struct Resolve {
  Buf text;
  SynthElf elf;
  LineProgram prog;
  ElfImage image;
  LineTable lines;

  // A at 0x1100: endbr64; push rbp; mov rbp,rsp; nops; call B (at 0x1110, 5 bytes, ends 0x1115); nop; nop; ret
  // B at 0x1200: push rbp; mov rbp,rsp; ret            (no endbr64)
  // C at 0x1300: endbr64; xor eax,eax; ret             (endbr64 but no push)
  Resolve() {
    elf.text_vaddr = 0x1000;
    text.assign(0x400, 0x90);
    auto at = [&](uint64_t v, std::initializer_list<uint8_t> bytes) {
      size_t i = v - 0x1000;
      for (uint8_t b : bytes) text[i++] = b;
    };
    at(0x1100, {0xf3, 0x0f, 0x1e, 0xfa, 0x55, 0x48, 0x89, 0xe5});
    const int32_t rel = static_cast<int32_t>(0x1200 - 0x1115);
    at(0x1110, {0xe8, static_cast<uint8_t>(rel), static_cast<uint8_t>(rel >> 8), static_cast<uint8_t>(rel >> 16), static_cast<uint8_t>(rel >> 24)});
    at(0x1117, {0xc3});
    at(0x1200, {0x55, 0x48, 0x89, 0xe5, 0xc3});
    at(0x1300, {0xf3, 0x0f, 0x1e, 0xfa, 0x31, 0xc0, 0xc3});
    elf.text = text;
    elf.funcs = {{"A", 0x1100, 0x20}, {"B", 0x1200, 5}, {"C", 0x1300, 7}};
    prog.dirs = {"/w", "/w/sb"};
    prog.files = {{"x.c", 1}, {"x.c", 1}};
  }
  void finish() {
    elf.debug_line = prog.build();
    KZ_CHECK_OK(ElfImage::parse(elf.build(), &image));
    KZ_CHECK_OK(LineTable::parse(image, &lines));
  }
};

void test_resolve_entry() {
  Resolve r;
  r.prog.set_address(0x1100); r.prog.advance_line(9); r.prog.copy(); r.prog.advance_pc(0x30); r.prog.end_sequence();
  r.finish();
  ResolvedTarget t;
  KZ_CHECK_OK(resolve_entry(r.image, r.lines, "A", "", &t));
  KZ_CHECK_EQ(t.kind, uint16_t(KZ_TARGET_KIND_FUNC_ENTRY));
  KZ_CHECK_EQ(t.vaddr, uint64_t(0x1104));   // after endbr64
  KZ_CHECK_EQ(t.delta, 4u);
  KZ_CHECK_EQ(t.file_offset, uint64_t(0x1104));
  KZ_CHECK_EQ(t.func_vaddr, uint64_t(0x1100));
  KZ_CHECK_EQ(t.insn_class, uint16_t(KZ_INSN_CLASS_EMULATED));  // push %rbp after endbr64: the measured class
  KZ_CHECK_EQ(t.insn[0], uint8_t(0x55));
  KZ_CHECK_EQ(t.func, std::string("A"));
  KZ_CHECK_EQ(t.line, 10u);
  KZ_CHECK_EQ(t.file, std::string("/w/sb/x.c"));
  KZ_CHECK_OK(resolve_entry(r.image, r.lines, "B", "", &t));
  KZ_CHECK_EQ(t.vaddr, uint64_t(0x1200));   // no endbr64: the first byte
  KZ_CHECK_EQ(t.delta, 0u);
  KZ_CHECK_EQ(t.insn_class, uint16_t(KZ_INSN_CLASS_UNKNOWN));   // not a measured pattern
  KZ_CHECK_OK(resolve_entry(r.image, r.lines, "C", "", &t));
  KZ_CHECK_EQ(t.delta, 4u);
  KZ_CHECK_EQ(t.insn_class, uint16_t(KZ_INSN_CLASS_UNKNOWN));   // endbr64 then xor: not push
  KZ_CHECK_ERR(resolve_entry(r.image, r.lines, "nosuch", "", &t), kz::Err::Invalid);
  // the classifier on its own
  const uint8_t push_after_endbr[8] = {0xf3, 0x0f, 0x1e, 0xfa, 0x55, 0x48, 0x89, 0xe5};
  KZ_CHECK_EQ(classify_entry_probe(push_after_endbr, 4), uint16_t(KZ_INSN_CLASS_EMULATED));
  KZ_CHECK_EQ(classify_entry_probe(push_after_endbr, 0), uint16_t(KZ_INSN_CLASS_UNKNOWN));
  KZ_CHECK_EQ(classify_entry_probe(push_after_endbr, 5), uint16_t(KZ_INSN_CLASS_UNKNOWN));
}

void test_resolve_entry_ambiguity() {
  // Two different functions with one name (two file-local functions): refused unless the file tells them apart.
  Resolve r;
  r.elf.funcs.push_back({"B", 0x1300, 7, STB_LOCAL});
  r.prog.dirs = {"/w", "/w/one", "/w/two"};
  r.prog.files = {{"one.c", 1}, {"one.c", 1}, {"two.c", 2}};
  r.prog.set_address(0x1200); r.prog.set_file(1); r.prog.copy(); r.prog.advance_pc(5);
  r.prog.set_file(2); r.prog.set_address(0x1300); r.prog.copy(); r.prog.advance_pc(7); r.prog.end_sequence();
  r.finish();
  ResolvedTarget t;
  KZ_CHECK_ERR(resolve_entry(r.image, r.lines, "B", "", &t), kz::Err::Invalid);
  KZ_CHECK_OK(resolve_entry(r.image, r.lines, "B", "two.c", &t));
  KZ_CHECK_EQ(t.func_vaddr, uint64_t(0x1300));
  KZ_CHECK_OK(resolve_entry(r.image, r.lines, "B", "one.c", &t));
  KZ_CHECK_EQ(t.func_vaddr, uint64_t(0x1200));
  KZ_CHECK_ERR(resolve_entry(r.image, r.lines, "B", "three.c", &t), kz::Err::Invalid);
  // an alias at one address is not an ambiguity
  Resolve a;
  a.elf.funcs.push_back({"B_alias", 0x1200, 5});
  a.prog.set_address(0x1100); a.prog.copy(); a.prog.advance_pc(0x30); a.prog.end_sequence();
  a.finish();
  KZ_CHECK_OK(resolve_entry(a.image, a.lines, "B_alias", "", &t));
  KZ_CHECK_EQ(t.func_vaddr, uint64_t(0x1200));
}

void test_resolve_after_call() {
  Resolve r;
  // line 20 is [0x1108, 0x1115): it ends with the call; line 21 is [0x1115, 0x1118)
  r.prog.set_address(0x1100); r.prog.advance_line(9); r.prog.copy();           // line 10 at 0x1100
  r.prog.advance_pc(8); r.prog.advance_line(10); r.prog.set_column(5); r.prog.copy();  // line 20 at 0x1108
  r.prog.advance_pc(0x0d); r.prog.advance_line(1); r.prog.copy();              // line 21 at 0x1115
  r.prog.advance_pc(3); r.prog.end_sequence();                                 // ends 0x1118
  r.finish();
  ResolvedTarget t;
  KZ_CHECK_OK(resolve_after_call(r.image, r.lines, "sb/x.c", 20, "B", &t));
  KZ_CHECK_EQ(t.kind, uint16_t(KZ_TARGET_KIND_AFTER_CALL));
  KZ_CHECK_EQ(t.vaddr, uint64_t(0x1115));           // the first byte after the call
  KZ_CHECK_EQ(t.file_offset, uint64_t(0x1115));
  KZ_CHECK_EQ(t.func, std::string("A"));
  KZ_CHECK_EQ(t.delta, 0x15u);
  KZ_CHECK_EQ(t.line, 20u);
  KZ_CHECK_EQ(t.col, 5u);
  KZ_CHECK_EQ(t.insn_class, uint16_t(KZ_INSN_CLASS_UNKNOWN));
  KZ_CHECK_EQ(t.file, std::string("/w/sb/x.c"));
  // errors, each for its own reason
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/x.c", 20, "C", &t), kz::Err::Invalid);        // the call is to B, not C
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/x.c", 20, "nosuch", &t), kz::Err::Invalid);
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/x.c", 99, "B", &t), kz::Err::Invalid);       // no code at that line
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/y.c", 20, "B", &t), kz::Err::Invalid);       // no such file
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/x.c", 10, "B", &t), kz::Err::Invalid);       // line 10 holds no call at its end
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "sb/x.c", 21, "B", &t), kz::Err::Invalid);
}

void test_resolve_after_call_refuses_when_the_call_is_not_last() {
  // The line's code continues after the call (a store of the result, say): the return site is not at the end of the line.
  Resolve r;
  r.prog.set_address(0x1108); r.prog.advance_line(19); r.prog.copy();   // line 20 covers [0x1108, 0x1118): the call at 0x1110 is not last
  r.prog.advance_pc(0x10); r.prog.end_sequence();
  r.finish();
  ResolvedTarget t;
  KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "x.c", 20, "B", &t), kz::Err::Invalid);
}

void test_resolve_after_call_ambiguity_and_function_end() {
  // two calls to B that each end a range of line 20
  {
    Resolve r;
    const int32_t rel2 = static_cast<int32_t>(0x1200 - 0x1125);
    r.text[0x120] = 0xe8;
    r.text[0x121] = static_cast<uint8_t>(rel2); r.text[0x122] = static_cast<uint8_t>(rel2 >> 8);
    r.text[0x123] = static_cast<uint8_t>(rel2 >> 16); r.text[0x124] = static_cast<uint8_t>(rel2 >> 24);
    r.elf.text = r.text;
    r.prog.set_address(0x1108); r.prog.advance_line(19); r.prog.copy();   // line 20: [0x1108,0x1115)
    r.prog.advance_pc(0x0d); r.prog.advance_line(1); r.prog.copy();       // line 21: [0x1115, 0x1120)
    r.prog.advance_pc(0x0b); r.prog.advance_line(-1); r.prog.copy();      // line 20 again: [0x1120, 0x1125)
    r.prog.advance_pc(5); r.prog.advance_line(2); r.prog.copy();          // line 22
    r.prog.advance_pc(3); r.prog.end_sequence();
    r.finish();
    ResolvedTarget t;
    KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "x.c", 20, "B", &t), kz::Err::Invalid);
  }
  // the call is the last instruction of its function: nothing follows it
  {
    Resolve r;
    r.elf.funcs[0].size = 0x15;  // A ends right after the call
    r.prog.set_address(0x1108); r.prog.advance_line(19); r.prog.copy();
    r.prog.advance_pc(0x0d); r.prog.end_sequence();
    r.finish();
    ResolvedTarget t;
    KZ_CHECK_ERR(resolve_after_call(r.image, r.lines, "x.c", 20, "B", &t), kz::Err::Invalid);
  }
}

}  // namespace

int main() {
  test_elf_basics();
  test_elf_rejects();
  test_line_program_forms();
  test_line_strp_and_dwarf64();
  test_two_units_and_sequence_order();
  test_ranges_and_suffixes();
  test_line_program_rejects();
  test_resolve_entry();
  test_resolve_entry_ambiguity();
  test_resolve_after_call();
  test_resolve_after_call_refuses_when_the_call_is_not_last();
  test_resolve_after_call_ambiguity_and_function_end();
  return kztest::finish("test_probe_elf_dwarf");
}
