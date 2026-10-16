// The kz-dump command line tool: its output is the canonical dump of the library, it
// accepts a good file, and it rejects a damaged one with exit status 1 (2 for I/O errors).

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include <map>

#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"
#include "support/source_identity.hpp"

namespace {

// Runs a command, returns its exit status and captures stdout.
int run(const std::string& command, std::string* out) {
  out->clear();
  FILE* p = popen((command + " 2>/dev/null").c_str(), "r");
  if (p == nullptr) return -1;
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), p)) > 0) out->append(buf, n);
  int status = pclose(p);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// "key=value" lines.
std::map<std::string, std::string> parse_lines(const std::string& text) {
  std::map<std::string, std::string> m;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == std::string::npos) nl = text.size();
    const std::string line = text.substr(pos, nl - pos);
    const size_t eq = line.find('=');
    if (eq != std::string::npos) m[line.substr(0, eq)] = line.substr(eq + 1);
    pos = nl + 1;
  }
  return m;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: test_core_kzdump <path of kz-dump>\n");
    return 2;
  }
  const std::string tool = argv[1];
  const std::string good = "kz_test_dump_good_" + std::to_string(static_cast<long>(getpid())) + ".kz";
  const std::string bad = "kz_test_dump_bad_" + std::to_string(static_cast<long>(getpid())) + ".kz";
  kz::Bytes bytes = kzfix::build_facts_fixture();
  KZ_CHECK_OK(kz::write_whole_file(good, bytes));
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));

  std::string out;
  KZ_CHECK_EQ(run(tool + " " + good, &out), 0);
  KZ_CHECK_EQ(out, kz::dump_container(c));
  KZ_CHECK_EQ(run(tool + " --hash " + good, &out), 0);
  KZ_CHECK_EQ(out, kz::to_hex(c.artifact_hash()) + "\n");
  KZ_CHECK_EQ(run(tool + " --check " + good, &out), 0);
  KZ_CHECK(out.empty());
  KZ_CHECK_EQ(run(tool + " --layout", &out), 0);
  KZ_CHECK_EQ(out, kz::layout_text() + "fingerprint=" + kz::to_hex(kz::schema_fingerprint()) + "\n");

  // --identity: the tool hash of this tool, made of the source identity of libkz-core and the
  // identity of the sources the tool is built from (its own and those of the libraries it links),
  // which the test recomputes from the files of the tree.
  KZ_CHECK_EQ(run(tool + " --identity", &out), 0);
  {
    const std::map<std::string, std::string> id = parse_lines(out);
    KZ_CHECK(id.count("tool") == 1 && id.count("tool_sha256") == 1 && id.count("tool_source") == 1 && id.count("core_source") == 1);
    if (id.count("tool_source") == 1 && id.count("core_source") == 1 && id.count("tool_sha256") == 1) {
      KZ_CHECK(kz::is_hex_digest(id.at("tool_source")));
      KZ_CHECK_EQ(id.at("core_source"), std::string(kz::core_source_identity()));
      const std::vector<std::string> paths = kzfix::split_paths(KZ_KZDUMP_IDENTITY_PATHS);
      KZ_CHECK(!paths.empty());
      bool has_tool_dir = false, has_core_dir = false, has_include = false;
      for (const std::string& q : paths) {
        has_tool_dir = has_tool_dir || q == "tools/kz-dump";
        has_core_dir = has_core_dir || q == "lib/core";  // a library in the link closure
        has_include = has_include || q == "include";
      }
      KZ_CHECK(has_tool_dir && has_core_dir && has_include);
      KZ_CHECK_EQ(id.at("tool_source"), kzfix::recompute_source_identity(KZ_SOURCE_ROOT_DIR, paths));
      KZ_CHECK_EQ(id.at("tool_sha256"), kz::to_hex(kz::tool_hash("kz-dump", KZ_TOOL_VERSION, id.at("tool_source"))));
      // It is not the hash of libkz-core alone: the tool's own sources count.
      KZ_CHECK(id.at("tool_sha256") != kz::to_hex(kz::tool_hash("kz-dump", KZ_TOOL_VERSION)));
    }
  }

  bytes[bytes.size() / 2] ^= 0x40;  // damage one byte
  KZ_CHECK_OK(kz::write_whole_file(bad, bytes));
  KZ_CHECK_EQ(run(tool + " " + bad, &out), 1);
  KZ_CHECK(out.empty());
  KZ_CHECK_EQ(run(tool + " --check " + bad, &out), 1);
  // A container that parses but has a content defect: refused, unless --no-verify is given.
  {
    kz::ContainerWriter w(KZ_ARTIFACT_TEST);
    w.set_check_reserved(false);
    std::vector<kz_deviation> dev(1, kzfix::zero<kz_deviation>());
    dev[0].prov = KZ_NONE32;
    dev[0].rsv0 = 9;
    KZ_CHECK_OK(w.add_section(KZ_SEC_DEVIATIONS, dev));
    KZ_CHECK_OK(w.write_file(bad));
    KZ_CHECK_EQ(run(tool + " " + bad, &out), 1);
    KZ_CHECK_EQ(run(tool + " --check " + bad, &out), 1);
    KZ_CHECK_EQ(run(tool + " --no-verify " + bad, &out), 0);
    KZ_CHECK(out.find("DEVIATIONS[0]") != std::string::npos && out.find("rsv0=9") != std::string::npos);
  }
  KZ_CHECK_EQ(run(tool + " does_not_exist.kz", &out), 2);
  KZ_CHECK_EQ(run(tool, &out), 2);
  KZ_CHECK_EQ(run(tool + " --bogus", &out), 2);
  std::remove(good.c_str());
  std::remove(bad.c_str());
  return kztest::finish("test_core_kzdump");
}
