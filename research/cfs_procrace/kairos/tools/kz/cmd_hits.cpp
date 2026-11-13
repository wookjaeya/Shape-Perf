// kz hits -- counts in a run container.  Counts only: no judgement about what they mean.
//
//   kz hits <run.kzt> [--site LABEL --before LABEL] [--group-slot ROLE] [--group-return-slot ROLE]
//           [--basis basis.kz --group-caller]
//
// Without options: the events of every site and of every thread.  With --site S and --before T: how
// many events of site S have a timestamp that precedes the first event of site T in the container's
// merged order (time, CPU, arrival), per thread name; --group-slot groups them also by the value of
// S's slot with that role (and per thread), --group-return-slot by the value of that role in the
// paired return event.  With the run's basis (its parent) --group-caller names the function that
// contains the return address (the slot with role return_address, less the load bias of the run) of
// each counted event, also per thread and per value of the --group-slot role.
//
// The merged order is built from timestamps of one clock (recorded in the run's conditions).  It is
// one-clock evidence of what the clock showed first, not a happens-before relation between threads
// (kz_raw.h, SPEC 5.8).  The time of a probe is the time the probe fired, not the time of the
// program effect the site stands for: a probe placed after a call fires after everything the call
// did, so "before T" there means "before the return from the call", an upper bound on the effect.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/trace/decode_match.hpp"
#include "kz/trace/identity_symbols.hpp"

namespace {

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz hits <run.kzt> [--site LABEL --before LABEL] [--group-slot ROLE] [--group-return-slot ROLE]\n"
               "               [--basis basis.kz --group-caller]\n"
               "  --site LABEL            count the events of this site\n"
               "  --before LABEL          only those before the first event of this site (merged order of the container)\n"
               "  --group-slot ROLE       also group by the value of the site's slot with this role\n"
               "  --group-return-slot ROLE  also group by the value of this role in the paired return event\n"
               "  --basis FILE            the run's basis container (its parent); needed by --group-caller\n"
               "  --group-caller          also group by the function that contains the return address of the event\n");
}

std::string hex(uint64_t v) {
  char b[32];
  std::snprintf(b, sizeof b, "0x%llx", static_cast<unsigned long long>(v));
  return b;
}

struct SlotRole {
  uint32_t site;
  uint8_t slot;
  std::string role;
};

int run(int argc, char** argv) {
  std::string path, site_label, before_label, group_slot, group_return, basis_path;
  bool group_caller = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) { std::fprintf(stderr, "kz hits: %s needs a value\n", a.c_str()); return false; }
      *dst = argv[++i];
      return true;
    };
    if (a == "--help" || a == "-h") { usage(stdout); return 0; }
    if (a == "--site") { if (!value(&site_label)) return 1; }
    else if (a == "--before") { if (!value(&before_label)) return 1; }
    else if (a == "--group-slot") { if (!value(&group_slot)) return 1; }
    else if (a == "--group-return-slot") { if (!value(&group_return)) return 1; }
    else if (a == "--basis") { if (!value(&basis_path)) return 1; }
    else if (a == "--group-caller") group_caller = true;
    else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "kz hits: unknown option %s\n", a.c_str()); return 1; }
    else if (path.empty()) path = a;
    else { std::fprintf(stderr, "kz hits: unexpected argument %s\n", a.c_str()); return 1; }
  }
  if (path.empty() || (!before_label.empty() && site_label.empty()) || ((!group_slot.empty() || !group_return.empty() || group_caller) && site_label.empty()) ||
      (group_caller && basis_path.empty())) {
    usage(stderr);
    return 1;
  }

  kz::Container c;
  kz::Status s = kz::Container::read_file(path, &c);
  if (s.ok()) s = c.verify_content();
  if (!s.ok()) { std::fprintf(stderr, "kz hits: %s: %s\n", path.c_str(), s.describe().c_str()); return 2; }
  if (c.header().artifact_kind != KZ_ARTIFACT_RUN) { std::fprintf(stderr, "kz hits: %s is not a run container\n", path.c_str()); return 2; }
  std::vector<kz_raw_ev> events;
  std::vector<kz_site> sites;
  std::vector<kz_site_slot> slots;
  std::vector<kz_thread> threads;
  s = c.read_section(KZ_SEC_RAW_EVENTS, &events);
  if (s.ok()) s = c.read_section(KZ_SEC_SITES, &sites);
  if (s.ok()) s = c.read_section(KZ_SEC_SITE_SLOTS, &slots);
  if (s.ok()) s = c.read_section(KZ_SEC_THREADS, &threads);
  if (!s.ok()) { std::fprintf(stderr, "kz hits: %s\n", s.describe().c_str()); return 2; }

  std::vector<kz_map_ev> maps;
  s = c.read_section(KZ_SEC_MAPS, &maps);
  if (!s.ok()) { std::fprintf(stderr, "kz hits: %s\n", s.describe().c_str()); return 2; }
  kz::trace::SymbolIndex symbols;
  uint64_t bias = 0;
  bool bias_known = false;
  if (group_caller) {
    kz::Container basis;
    s = kz::Container::read_file(basis_path, &basis);
    if (s.ok()) s = basis.verify_content();
    if (s.ok() && basis.header().artifact_kind != KZ_ARTIFACT_BASIS) s = kz::Status::error(kz::Err::Invalid, "not a basis container");
    if (s.ok()) s = c.verify_parent(basis);
    if (s.ok()) s = kz::trace::load_symbols(basis, &symbols);
    if (!s.ok()) { std::fprintf(stderr, "kz hits: --basis %s: %s\n", basis_path.c_str(), s.describe().c_str()); return 2; }
    for (const kz_map_ev& m : maps)
      if (m.bias_known != 0) {
        if (bias_known && m.bias != bias) { std::fprintf(stderr, "kz hits: the mappings of the run disagree on the load bias\n"); return 2; }
        bias = m.bias;
        bias_known = true;
      }
    if (!bias_known) { std::fprintf(stderr, "kz hits: the run has no mapping with a known load bias\n"); return 2; }
  }

  std::vector<std::string> labels(sites.size());
  for (size_t i = 0; i < sites.size(); ++i) {
    if (sites[i].site != i) { std::fprintf(stderr, "kz hits: the site table is not indexed by site id\n"); return 2; }
    c.string(sites[i].label, &labels[i]);
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> role_of;  // (site, slot) -> role
  std::set<std::pair<uint32_t, uint8_t>> string_slot;            // (site, slot) of a string fetch slot
  for (const kz_site_slot& sl : slots) {
    std::string r;
    c.string(sl.role, &r);
    role_of[{sl.site, sl.slot}] = r;
    if (sl.src == KZ_SLOT_SRC_STR) string_slot.insert({sl.site, sl.slot});
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> text_of;   // (event, slot) -> string (EVENT_STRINGS)
  if (c.find_section(KZ_SEC_EVENT_STRINGS) != nullptr) {
    std::vector<kz_ev_str> strs;
    s = c.read_section(KZ_SEC_EVENT_STRINGS, &strs);
    if (!s.ok()) { std::fprintf(stderr, "kz hits: %s\n", s.describe().c_str()); return 2; }
    for (const kz_ev_str& e : strs) c.string(e.text, &text_of[{e.event, e.slot}]);
  }
  // a slot value as printed: a string slot by its text, any other slot in hex
  auto value_text = [&](size_t ev, const kz_raw_ev& e, int k) -> std::string {
    if (string_slot.count({e.site, static_cast<uint8_t>(k)}) == 0) return hex(e.a[k]);
    const auto t = text_of.find({static_cast<uint32_t>(ev), static_cast<uint8_t>(k)});
    return t == text_of.end() ? std::string("(no string)") : "\"" + t->second + "\"";
  };
  auto thread_name = [&](const kz_raw_ev& e) {
    // The incarnation of the tid that was alive at the event: the latest generation forked before it.
    const kz_thread* best = nullptr;
    for (const kz_thread& t : threads) {
      if (t.tid != e.tid) continue;
      if (best == nullptr || (t.t_fork <= e.t_ns && t.generation >= best->generation)) best = &t;
    }
    std::string n;
    if (best != nullptr) c.string(best->name, &n);
    return n.empty() ? std::string("(tid ") + std::to_string(e.tid) + ")" : n;
  };

  std::printf("kz hits %s\nartifact_sha256 %s\nevents %zu\n", path.c_str(), kz::to_hex(c.artifact_hash()).c_str(), events.size());
  std::vector<uint64_t> per_site(sites.size(), 0);
  std::map<std::string, uint64_t> per_thread;
  for (const kz_raw_ev& e : events) {
    if (e.site < per_site.size()) ++per_site[e.site];
    ++per_thread[thread_name(e) + " (tid " + std::to_string(e.tid) + ")"];
  }
  std::printf("sites:\n");
  for (size_t i = 0; i < sites.size(); ++i) std::printf("  site %-2zu %-28s events %llu\n", i, labels[i].c_str(), static_cast<unsigned long long>(per_site[i]));
  std::printf("threads:\n");
  for (const auto& t : per_thread) std::printf("  %-40s events %llu\n", t.first.c_str(), static_cast<unsigned long long>(t.second));

  if (site_label.empty()) return 0;
  auto site_id = [&](const std::string& l) -> int64_t {
    for (size_t i = 0; i < labels.size(); ++i)
      if (labels[i] == l) return static_cast<int64_t>(i);
    return -1;
  };
  const int64_t sid = site_id(site_label);
  if (sid < 0) { std::fprintf(stderr, "kz hits: no site labelled %s\n", site_label.c_str()); return 2; }
  int64_t tid_site = -1;
  if (!before_label.empty()) {
    tid_site = site_id(before_label);
    if (tid_site < 0) { std::fprintf(stderr, "kz hits: no site labelled %s\n", before_label.c_str()); return 2; }
  }

  // The reference event.
  size_t limit = events.size();
  if (tid_site >= 0) {
    limit = events.size();
    for (size_t i = 0; i < events.size(); ++i)
      if (events[i].site == tid_site) { limit = i; break; }
    if (limit == events.size()) {
      std::printf("\nsite %s was not hit: there is no event to count before\n", before_label.c_str());
      std::printf("events of site %s in the whole container: %llu\n", site_label.c_str(), static_cast<unsigned long long>(per_site[sid]));
      return 0;
    }
    std::printf("\nfirst event of site %s: index %zu, thread %s (tid %u), t_ns %llu\n", before_label.c_str(), limit, thread_name(events[limit]).c_str(),
                events[limit].tid, static_cast<unsigned long long>(events[limit].t_ns));
  } else {
    std::printf("\n");
  }

  // Pairing for --group-return-slot: the sites with their slots, as the observer attached them
  // (match_calls pairs only sites that declare the return_address key).
  std::vector<kz::trace::SiteSpec> specs(sites.size());
  for (size_t i = 0; i < sites.size(); ++i) {
    specs[i].site = static_cast<uint32_t>(i);
    specs[i].kind = sites[i].kind;
    specs[i].decoder = sites[i].decoder;
    c.string(sites[i].symbol, &specs[i].symbol);
    c.string(sites[i].label, &specs[i].label);
  }
  for (const kz_site_slot& sl : slots) {
    if (sl.site >= specs.size()) continue;
    kz::trace::SlotSpec ss;
    ss.slot = sl.slot;
    ss.src = sl.src;
    ss.width = sl.width;
    ss.index = sl.index;
    c.string(sl.role, &ss.role);
    specs[sl.site].slots.push_back(ss);
  }
  const std::vector<kz::trace::CallPair> pairs = kz::trace::match_calls(events, specs, nullptr);
  std::map<uint32_t, uint32_t> return_of;  // entry event index -> return event index
  for (const kz::trace::CallPair& p : pairs)
    if (p.entry_event != KZ_NONE32 && p.return_event != KZ_NONE32) return_of[p.entry_event] = p.return_event;

  int slot_index = -1;
  if (!group_slot.empty()) {
    for (int k = 0; k < 4; ++k) {
      auto it = role_of.find({static_cast<uint32_t>(sid), static_cast<uint8_t>(k)});
      if (it != role_of.end() && it->second == group_slot) slot_index = k;
    }
    if (slot_index < 0) { std::fprintf(stderr, "kz hits: site %s has no slot with role %s\n", site_label.c_str(), group_slot.c_str()); return 2; }
  }

  int caller_slot = -1;
  if (group_caller) {
    for (int k = 0; k < 4; ++k) {
      auto it = role_of.find({static_cast<uint32_t>(sid), static_cast<uint8_t>(k)});
      if (it != role_of.end() && it->second == kz::trace::kRoleReturnAddress) caller_slot = k;
    }
    if (caller_slot < 0) { std::fprintf(stderr, "kz hits: site %s has no slot with role return_address\n", site_label.c_str()); return 2; }
  }

  uint64_t total = 0;
  std::map<std::string, uint64_t> by_thread, by_slot, by_return, by_caller;
  std::map<std::pair<std::string, std::string>, uint64_t> by_thread_slot, by_thread_caller, by_caller_slot;
  uint64_t no_return = 0, unreadable = 0;
  for (size_t i = 0; i < limit; ++i) {
    const kz_raw_ev& e = events[i];
    if (e.site != sid) continue;
    ++total;
    if ((e.flags & KZ_RAW_FLAG_TRUNC) != 0) ++unreadable;
    by_thread[thread_name(e)]++;
    if (slot_index >= 0) {
      by_slot[value_text(i, e, slot_index)]++;
      by_thread_slot[{thread_name(e), value_text(i, e, slot_index)}]++;
    }
    if (caller_slot >= 0) {
      const kz::trace::SymbolEntry* sym = symbols.find(e.a[caller_slot] - bias);
      const std::string caller = sym != nullptr ? sym->name : std::string("(no symbol)");
      by_caller[caller]++;
      by_thread_caller[{thread_name(e), caller}]++;
      if (slot_index >= 0) by_caller_slot[{caller, value_text(i, e, slot_index)}]++;
    }
    if (!group_return.empty()) {
      auto it = return_of.find(static_cast<uint32_t>(i));
      if (it == return_of.end()) {
        ++no_return;
      } else {
        const kz_raw_ev& r = events[it->second];
        int rk = -1;
        for (int k = 0; k < 4; ++k) {
          auto rr = role_of.find({r.site, static_cast<uint8_t>(k)});
          if (rr != role_of.end() && rr->second == group_return) rk = k;
        }
        if (rk < 0) { std::fprintf(stderr, "kz hits: the return site has no slot with role %s\n", group_return.c_str()); return 2; }
        by_return[value_text(it->second, r, rk)]++;
      }
    }
  }
  if (tid_site >= 0)
    std::printf("events of site %s before the first event of site %s: %llu (timestamp order of one clock)\n", site_label.c_str(), before_label.c_str(),
                static_cast<unsigned long long>(total));
  else
    std::printf("events of site %s: %llu\n", site_label.c_str(), static_cast<unsigned long long>(total));
  std::printf("  by thread name:\n");
  for (const auto& t : by_thread) std::printf("    %-36s %llu\n", t.first.c_str(), static_cast<unsigned long long>(t.second));
  if (slot_index >= 0) {
    std::printf("  by %s (slot %d):\n", group_slot.c_str(), slot_index);
    for (const auto& t : by_slot) std::printf("    %-36s %llu\n", t.first.c_str(), static_cast<unsigned long long>(t.second));
    std::printf("  by thread name and %s:\n", group_slot.c_str());
    for (const auto& t : by_thread_slot)
      std::printf("    %-24s %-11s %llu\n", t.first.first.c_str(), t.first.second.c_str(), static_cast<unsigned long long>(t.second));
  }
  if (caller_slot >= 0) {
    std::printf("  by the function that contains the return address (slot %d, load bias %s):\n", caller_slot, hex(bias).c_str());
    for (const auto& t : by_caller) std::printf("    %-36s %llu\n", t.first.c_str(), static_cast<unsigned long long>(t.second));
    if (slot_index >= 0) {
      std::printf("  by caller and %s:\n", group_slot.c_str());
      for (const auto& t : by_caller_slot)
        std::printf("    %-36s %-11s %llu\n", t.first.first.c_str(), t.first.second.c_str(), static_cast<unsigned long long>(t.second));
    }
    std::printf("  by thread name and caller:\n");
    for (const auto& t : by_thread_caller)
      std::printf("    %-24s %-36s %llu\n", t.first.first.c_str(), t.first.second.c_str(), static_cast<unsigned long long>(t.second));
  }
  if (unreadable != 0)
    std::printf("  events with an unreadable slot (TRUNC; their slot values are 0 and were counted as such): %llu\n", static_cast<unsigned long long>(unreadable));
  if (!group_return.empty()) {
    std::printf("  by %s of the paired return:\n", group_return.c_str());
    for (const auto& t : by_return) std::printf("    %-36s %llu\n", t.first.c_str(), static_cast<unsigned long long>(t.second));
    std::printf("    %-36s %llu\n", "(no return event paired)", static_cast<unsigned long long>(no_return));
  }
  std::printf("note: \"before\" is the container's merged order (time, CPU, arrival) of one clock; it is one-clock evidence, not a happens-before relation.\n");
  if (tid_site >= 0)
    std::printf("note: an event's time is when its probe fired; for a probe after a call (a point site), that is after the call returned, an upper bound on the time of the call's effect.\n");
  return 0;
}

}  // namespace

KZ_COMMAND(hits, "count the events of a run container (per site, thread, before a reference site)", run);
