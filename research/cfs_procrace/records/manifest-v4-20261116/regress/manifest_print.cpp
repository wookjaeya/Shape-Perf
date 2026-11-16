// manifest_print.cpp -- a research check program (not a Kairos tool): it reads one or more site manifests into one
// kzrt::Manifest with the host parser it is compiled with (kairos/runtime/tools/manifest/kz_rt_manifest.cpp), and
// prints every field of the format's versions 1-3 in a fixed order, or the parser's error.  Compiled twice by
// build_printers.sh, against the parser before the manifest revision 4 step (git) and the one after it, so that the two
// outputs on the same inputs can be compared byte for byte (regress.sh).  With -DKZ_PRINT_V4 the program also prints a
// summary line of the version 4 statements (the new parser only; that line is not compared).
//
//   manifest_print FILE [FILE...]
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "kz_rt_manifest.hpp"

int main(int argc, char** argv) {
  kzrt::Manifest m;
  std::string err;
  for (int i = 1; i < argc; ++i) {
    std::ifstream f(argv[i], std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    if (!f.good() && !f.eof()) {
      std::printf("error cannot read %s\n", argv[i]);
      return 0;
    }
    if (!kzrt::parse_manifest(ss.str(), argv[i], &m, &err)) {
      std::printf("error %s\n", err.c_str());
      return 0;
    }
  }
  std::printf("version %u\n", m.version);
  for (const kzrt::Source& s : m.sources) std::printf("source %s rev=%s ref=%s line=%d\n", s.name.c_str(), s.rev.c_str(), s.ref.c_str(), s.line);
  for (const kzrt::Module& md : m.modules) std::printf("module %u name=%s source=%s line=%d\n", md.id, md.name.c_str(), md.source.c_str(), md.line);
  for (const kzrt::Site& s : m.sites) {
    std::printf("site %u label=%s op=%s phase=%s tu=%s func=%s callee=%s place=%s line=%u ref=%s at=%d mask=%u\n", s.id, s.label.c_str(), s.op.c_str(),
                s.phase.c_str(), s.tu.c_str(), s.func.c_str(), s.callee.c_str(), s.place.c_str(), s.src_line, s.ref.c_str(), s.line, s.declared_mask());
    for (const kzrt::Value& v : s.values)
      std::printf("  value %u role=%s from=%s type=%s string=%s bound=%u ref=%s at=%d\n", v.slot, v.role.c_str(), v.from.c_str(), v.type.c_str(), v.string_mode.c_str(),
                  v.bound, v.ref.c_str(), v.line);
    if (s.has_control) std::printf("  control ctx=%s ref=%s note=%s\n", s.ctx.c_str(), s.ctl_ref.c_str(), s.ctl_note.c_str());
    if (s.has_identity)
      std::printf("  identity decoder=%s id=%d owner=%d status=%d ok=%lld ref=%s at=%d\n", s.identity.decoder.c_str(), s.identity.id_slot, s.identity.owner_slot,
                  s.identity.status_slot, static_cast<long long>(s.identity.ok), s.identity.ref.c_str(), s.identity.line);
  }
  for (const kzrt::Bind& b : m.binds)
    std::printf("bind %u site=%u module=%u backend=%s at=%s status=%s reason=%s vals=%u given=%d line=%d\n", b.id, b.site, b.module, b.backend.c_str(), b.at.c_str(),
                b.status.c_str(), b.reason.c_str(), b.vals, b.vals_given ? 1 : 0, b.line);
  for (const kzrt::Equiv& e : m.equivs) std::printf("equiv %u backends=%s status=%s ref=%s line=%d\n", e.site, e.backends.c_str(), e.status.c_str(), e.ref.c_str(), e.line);
  std::printf("indexes sites=%zu binds=%zu modules=%zu labels=%zu\n", m.site_index.size(), m.bind_index.size(), m.module_index.size(), m.label_index.size());
#ifdef KZ_PRINT_V4
  size_t spawn = 0, load = 0, skey = 0;
  for (const kzrt::Site& s : m.sites) {
    spawn += s.has_spawn ? 1 : 0;
    load += s.place == "load" ? 1 : 0;
    skey += s.skey.empty() ? 0 : 1;
  }
  std::fprintf(stderr, "v4 plan=%s needs=%zu gaps=%zu spawn=%zu load=%zu skey=%zu\n", m.has_plan ? m.plan.id.c_str() : "-", m.needs.size(), m.gaps.size(), spawn, load, skey);
#endif
  return 0;
}
