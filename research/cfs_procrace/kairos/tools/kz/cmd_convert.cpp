// kz convert -- writes a run container with the neutral event layer (schema revision 3, docs/schema.md section 12).
//
//   kz convert <run.kzt> --out <file.kzt>
//       an F0 run container: every section unchanged (the backend raw evidence), the neutral layer beside it, the
//       input as a parent (role 1).  `kz replay` then reads the layer (the replay of a container without it converts
//       the same way in memory, so both readings agree).
//   kz convert --kzrt <dump> --manifest <kzs> [--manifest <kzs>]... [--basis <basis.kz>] [--process PID] --out <file.kzt>
//       an instrumented run: a kzrt-dump stream with its site manifests (the logical sites, then the backend's
//       binding list) -> a run container with the neutral layer only (kz/trace/kzrt_convert.hpp); the dump and the
//       manifests are named in FILE_HASHES and BASIS.  --basis makes a basis container of the instrumented build its
//       parent.  --process gives the process id the harness recorded, for a dump that does not record its own
//       (kzrt-dump 1.1): the initial thread is then the run actor whose thread id is that id.  What the conversion cannot state is printed as notes and kept as BASIS items convert.note.
//
// Neither form sets a research condition.  The output names what it read; whether the record is a development or an
// experiment run is stated by the record's own conditions.
#include <cstdio>
#include <string>
#include <vector>

#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/core/identity.hpp"
#include "kz/core/registry.hpp"
#include "kz/trace/kzrt_convert.hpp"
#include "kz/trace/neutral.hpp"

namespace {

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz convert <run.kzt> --out FILE.kzt\n"
               "       kz convert --kzrt DUMP --manifest KZS [--manifest KZS]... [--basis BASIS.kz] [--process PID] --out FILE.kzt\n"
               "  <run.kzt>       an F0 run container: kept unchanged, the neutral event layer added (schema revision 3)\n"
               "  --kzrt DUMP     a kzrt-dump stream of an instrumented run, with its site manifests (kzs-sites 1)\n"
               "  --basis B       (with --kzrt) the basis container of the instrumented build, made the parent\n"
               "  --process PID   (with --kzrt) the process id the harness recorded (a dump without RUNX process)\n"
               "  --out FILE      the run container to write\n");
}

void print_layer(const kz::trace::NeutralRun& n) {
  std::printf("neutral layer: sites %zu, slots %zu, events %zu, values %zu, actors %zu, losses %zu, control records %zu\n", n.sites.size(), n.slots.size(),
              n.events.size(), n.values.size(), n.actors.size(), n.losses.size(), n.ctl.size());
  const kz::EnumDesc* fl = kz::find_enum("KZ_NRUN_FLAG");
  std::string flags;
  for (size_t i = 0; fl != nullptr && i < fl->nitems; ++i)
    if ((n.run.r.flags & fl->items[i].value) != 0) flags += (flags.empty() ? "" : ",") + std::string(fl->items[i].display);
  const kz::EnumDesc* st = kz::find_enum("KZ_NSTART");
  const char* start = st != nullptr ? kz::enum_display(*st, n.run.r.start_evidence) : nullptr;
  std::printf("run: backend %s, flags {%s}, start evidence %s\n", n.run.backend_name.c_str(), flags.c_str(), start != nullptr ? start : "?");
  for (const kz_nloss& l : n.losses) std::printf("loss: %s\n", kz::trace::loss_text(l).c_str());
}

int run(int argc, char** argv) {
  std::string in, dump, out, basis_path, process;
  std::vector<std::string> manifests;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "kz convert: %s needs a value\n", a.c_str());
        return false;
      }
      *dst = argv[++i];
      return true;
    };
    if (a == "--help" || a == "-h") {
      usage(stdout);
      return 0;
    } else if (a == "--out") {
      if (!value(&out)) return 1;
    } else if (a == "--kzrt") {
      if (!value(&dump)) return 1;
    } else if (a == "--basis") {
      if (!value(&basis_path)) return 1;
    } else if (a == "--process") {
      if (!value(&process)) return 1;
    } else if (a == "--manifest") {
      std::string m;
      if (!value(&m)) return 1;
      manifests.push_back(m);
    } else if (!a.empty() && a[0] == '-') {
      std::fprintf(stderr, "kz convert: unknown option %s\n", a.c_str());
      return 1;
    } else if (in.empty()) {
      in = a;
    } else {
      std::fprintf(stderr, "kz convert: unexpected argument %s\n", a.c_str());
      return 1;
    }
  }
  if (out.empty() || in.empty() == dump.empty() || (!dump.empty() && manifests.empty()) || (dump.empty() && (!manifests.empty() || !basis_path.empty() || !process.empty())) ||
      (!process.empty() && process.find_first_not_of("0123456789") != std::string::npos)) {
    usage(stderr);
    return 1;
  }
  kz::Bytes bytes;
  kz::Status s;
  if (!in.empty()) {
    kz::Container c;
    s = kz::Container::read_file(in, &c);
    if (s.ok()) s = c.verify_content();
    kz::trace::NeutralRun layer;
    if (s.ok()) s = kz::trace::convert_f0_container(c, KZ_THIS_TOOL_HASH("kz"), &bytes, &layer);
    if (!s.ok()) {
      std::fprintf(stderr, "kz convert: %s: %s\n", in.c_str(), s.describe().c_str());
      return 2;
    }
    std::printf("kz convert %s (F0 run container)\nartifact_sha256 %s\n", in.c_str(), kz::to_hex(c.artifact_hash()).c_str());
    print_layer(layer);
  } else {
    kz::trace::KzrtConvertInput ki;
    ki.dump_name = dump;
    ki.tool_sha256 = KZ_THIS_TOOL_HASH("kz");
    if (!process.empty()) ki.harness_process = std::stoull(process);
    s = kz::read_whole_file(dump, &ki.dump);
    for (const std::string& m : manifests) {
      kz::Bytes t;
      if (s.ok()) s = kz::read_whole_file(m, &t);
      ki.manifests.push_back({m, std::string(t.begin(), t.end())});
    }
    kz::Container basis;
    if (s.ok() && !basis_path.empty()) {
      s = kz::Container::read_file(basis_path, &basis);
      if (s.ok()) s = basis.verify_content();
      ki.basis = &basis;
    }
    kz::trace::KzrtConvertResult r;
    if (s.ok()) s = kz::trace::convert_kzrt_container(ki, &r);
    if (!s.ok()) {
      std::fprintf(stderr, "kz convert: %s: %s\n", dump.c_str(), s.describe().c_str());
      return 2;
    }
    bytes = std::move(r.container);
    std::printf("kz convert --kzrt %s (instrumented run)\ndump_sha256 %s\n", dump.c_str(), kz::to_hex(kz::sha256(ki.dump.data(), ki.dump.size())).c_str());
    for (const auto& m : ki.manifests) std::printf("manifest %s sha256 %s\n", m.first.c_str(), kz::to_hex(kz::sha256(m.second)).c_str());
    print_layer(r.layer);
    for (const std::string& note : r.notes) std::printf("note: %s\n", note.c_str());
  }
  s = kz::write_whole_file(out, bytes);
  if (!s.ok()) {
    std::fprintf(stderr, "kz convert: --out %s: %s\n", out.c_str(), s.describe().c_str());
    return 2;
  }
  std::printf("wrote %s artifact_sha256 %s\n", out.c_str(), kz::to_hex(kz::sha256(bytes.data(), bytes.size())).c_str());
  return 0;
}

}  // namespace

KZ_COMMAND(convert, "write a run container with the neutral event layer (from an F0 run container or a kzrt-dump)", run);
