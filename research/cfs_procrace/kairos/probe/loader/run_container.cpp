// run_container.cpp -- see run_container.hpp.
#include "run_container.hpp"

#include <cstdio>
#include <cstring>

#include "basis.hpp"
#include "kz/core/container.hpp"

namespace kz::probe {

bool build_id_from_hex(const std::string& hex, uint8_t out[20]) {
  if (hex.size() != 40) return false;
  auto val = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  for (size_t i = 0; i < 20; ++i) {
    const int a = val(hex[2 * i]), b = val(hex[2 * i + 1]);
    if (a < 0 || b < 0) return false;
    out[i] = static_cast<uint8_t>(a * 16 + b);
  }
  return true;
}

std::vector<TrapAccount> trap_accounts(const std::vector<kz::trace::SiteSpec>& sites) {
  std::vector<TrapAccount> out(sites.size(), TrapAccount::Uncounted);
  for (size_t i = 0; i < sites.size(); ++i) {
    // The row that counts the trap at a file offset: the first ENTRY or POINT site there.
    size_t own = sites.size();
    for (size_t j = 0; j < sites.size(); ++j)
      if (sites[j].file_offset == sites[i].file_offset && sites[j].kind != KZ_SITE_KIND_RET_PROBE) {
        own = j;
        break;
      }
    if (own == i) out[i] = TrapAccount::CountedHere;
    else if (own < sites.size()) out[i] = TrapAccount::InOtherRow;
  }
  return out;
}

std::string trap_account_text(const std::vector<kz::trace::SiteSpec>& sites, size_t i) {
  const std::vector<TrapAccount> acc = trap_accounts(sites);
  const kz::trace::SiteSpec& s = sites[i];
  char off[32];
  std::snprintf(off, sizeof off, "0x%llx", static_cast<unsigned long long>(s.file_offset));
  const std::string own_cost = s.kind == KZ_SITE_KIND_RET_PROBE ? "this row counts return trampolines" : "this row counts the trap at its address";
  switch (acc[i]) {
    case TrapAccount::CountedHere:
      return std::string("counted-here: the trap at file offset ") + off + " is counted in this row";
    case TrapAccount::InOtherRow:
      for (size_t j = 0; j < sites.size(); ++j)
        if (sites[j].file_offset == s.file_offset && acc[j] == TrapAccount::CountedHere)
          return std::string("in-other-row: the trap at file offset ") + off + " is counted in the row of site " + std::to_string(sites[j].site) + " (" + sites[j].label + ")" +
                 (s.kind == KZ_SITE_KIND_RET_PROBE ? "; this row counts return trampolines" : "; this row repeats the same trap");
      break;
    case TrapAccount::Uncounted:
      break;
  }
  return std::string("uncounted: no entry or point site is at file offset ") + off + ", so the trap there is counted in no row; " + own_cost;
}

Status write_run_container(const RunInputs& in, Bytes* out) {
  ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(in.basis_artifact);
  w.set_profile_hash(in.profile_sha256);
  w.set_tool_hash(in.tool_hash);
  Status s = w.add_parent(KZ_ARTIFACT_BASIS, 0, in.basis_artifact);
  if (!s.ok()) return s;

  // The plan, with the identity of the file that was probed.
  std::vector<kz_site> sites;
  std::vector<kz_site_slot> slots;
  kz::plan::site_records(in.sites, &w, &sites, &slots);
  for (size_t i = 0; i < sites.size(); ++i) {
    kz_site& k = sites[i];
    const uint16_t b = i < in.sites.size() ? in.sites[i].binary : 0;
    if (b == 0 || b > in.also.size()) {
      k.inode = in.inode;
      k.device = in.device;
      k.path = w.intern(in.exe_path);
      std::memcpy(k.build_id, in.build_id, 20);
    } else {
      const FileIdentity& f = in.also[b - 1];
      k.inode = f.inode;
      k.device = f.device;
      k.path = w.intern(f.path);
      std::memcpy(k.build_id, f.build_id, 20);
    }
  }
  if (!(s = w.add_section(KZ_SEC_SITES, sites)).ok()) return s;
  if (!slots.empty() && !(s = w.add_section(KZ_SEC_SITE_SLOTS, slots)).ok()) return s;
  if (!in.decoded.events.empty() && !(s = w.add_section(KZ_SEC_RAW_EVENTS, in.decoded.events)).ok()) return s;
  // Memory reads of fetch slots, call paths of entry-reached sites, and the strings the kernel fetched
  // (sections written only where the plan has them: other run containers are unchanged).
  std::vector<kz_slot_fetch> fetches;
  kz::plan::fetch_records(in.sites, &fetches);
  if (!fetches.empty() && !(s = w.add_section(KZ_SEC_SLOT_FETCHES, fetches)).ok()) return s;
  std::vector<kz_via> vias;
  kz::plan::via_records(in.sites, &w, &vias);
  if (!vias.empty() && !(s = w.add_section(KZ_SEC_VIA, vias)).ok()) return s;
  std::vector<kz_ev_str> strs;
  for (const kz::trace::EventString& e : in.decoded.strings) {
    kz_ev_str k;
    std::memset(&k, 0, sizeof k);
    k.event = e.event;
    k.slot = e.slot;
    k.text = w.intern(e.text);
    strs.push_back(k);
  }
  if (!strs.empty() && !(s = w.add_section(KZ_SEC_EVENT_STRINGS, strs)).ok()) return s;

  std::vector<kz_side_ev> side;
  for (const kz::trace::SideEvent& e : in.decoded.side) {
    kz_side_ev k = e.rec;
    k.comm = w.intern(e.comm);
    side.push_back(k);
  }
  if (!side.empty() && !(s = w.add_section(KZ_SEC_SIDE_EVENTS, side)).ok()) return s;
  std::vector<kz_map_ev> maps;
  for (const kz::trace::MapEvent& e : in.decoded.maps) {
    kz_map_ev k = e.rec;
    k.path = w.intern(e.path);
    maps.push_back(k);
  }
  if (!maps.empty() && !(s = w.add_section(KZ_SEC_MAPS, maps)).ok()) return s;
  std::vector<kz_thread> threads;
  for (const kz::trace::ThreadRecord& t : in.threads) {
    kz_thread k = t.rec;
    k.name = w.intern(t.name);
    k.entry_symbol = w.intern(t.entry_symbol);
    threads.push_back(k);
  }
  if (!threads.empty() && !(s = w.add_section(KZ_SEC_THREADS, threads)).ok()) return s;
  std::vector<kz_app_owner> owners;
  for (const kz::trace::OwnerRecord& o : in.owners) {
    kz_app_owner k = o.rec;
    k.app_name = w.intern(o.app_name);
    owners.push_back(k);
  }
  if (!owners.empty() && !(s = w.add_section(KZ_SEC_ACTOR_OWNERS, owners)).ok()) return s;
  if (!in.decoded.loss.empty() && !(s = w.add_section(KZ_SEC_LOSS, in.decoded.loss)).ok()) return s;

  // Run metadata.
  std::vector<kz_runmeta> meta(1);
  std::memset(&meta[0], 0, sizeof(kz_runmeta));
  std::memcpy(meta[0].basis_sha256, in.basis_artifact.data(), 32);
  const Digest ph = kz::plan::plan_hash(in.sites);
  std::memcpy(meta[0].plan_sha256, ph.data(), 32);
  meta[0].run_ordinal = in.run_ordinal;
  meta[0].stratum = in.stratum;
  meta[0].backend = KZ_BACKEND_PERF_SAMPLE;
  meta[0].fidelity = KZ_FIDELITY_F0;
  uint16_t flags = in.run_flags;
  if (!in.decoded.loss.empty() || in.decoded.stats.stream_errors != 0) flags |= KZ_RUN_FLAG_LOSS_PRESENT;
  meta[0].flags = flags;
  meta[0].kernel = w.intern(in.kernel);
  meta[0].harness = w.intern(in.harness);
  meta[0].placement = w.intern(in.placement);
  meta[0].observer_cpu_ns = in.observer_cpu_ns;
  meta[0].boot_ns = in.boot_ns;
  if (!(s = w.add_section(KZ_SEC_RUNMETA, meta)).ok()) return s;

  // Perturbation accounting (SPEC 4.13): hits per site, observer CPU, process lifetime, /proc reads.
  std::vector<kz_perturbation> pert;
  for (size_t i = 0; i < in.sites.size(); ++i) {
    kz_perturbation p;
    std::memset(&p, 0, sizeof p);
    p.kind = KZ_PERTURBATION_KIND_PROBE_HITS;
    // A return site's own cost is the return trampoline; its entry trap is counted by the first
    // ENTRY or POINT site at the same file offset, if there is one (kz_raw.h; trap_accounts).
    p.cost_class = in.sites[i].kind == KZ_SITE_KIND_RET_PROBE ? static_cast<uint32_t>(KZ_INSN_CLASS_RET_TRAMPOLINE) : in.sites[i].insn_class;
    p.site = in.sites[i].site;
    p.count = i < in.decoded.stats.site_hits.size() ? in.decoded.stats.site_hits[i] : 0;
    pert.push_back(p);
  }
  {
    kz_perturbation p;
    std::memset(&p, 0, sizeof p);
    p.kind = KZ_PERTURBATION_KIND_OBSERVER_CPU;
    p.site = KZ_NONE32;
    p.ns = in.observer_cpu_ns;
    pert.push_back(p);
    std::memset(&p, 0, sizeof p);
    p.kind = KZ_PERTURBATION_KIND_BOOT_TIME;
    p.site = KZ_NONE32;
    p.ns = in.boot_ns;
    pert.push_back(p);
    std::memset(&p, 0, sizeof p);
    p.kind = KZ_PERTURBATION_KIND_PROC_CAPTURE;
    p.cost_class = KZ_INSN_CLASS_UNKNOWN;
    p.site = KZ_NONE32;
    p.count = in.proc_reads;
    p.ns = in.proc_ns;
    pert.push_back(p);
  }
  if (!(s = w.add_section(KZ_SEC_PERTURBATION, pert)).ok()) return s;

  // Conditions and deviations.
  std::vector<kz_deviation> devs;
  for (const Deviation& d : in.record.deviations) {
    kz_deviation k;
    std::memset(&k, 0, sizeof k);
    k.subject = w.intern(d.subject);
    k.reference_value = w.intern(d.reference_value);
    k.used_value = w.intern(d.used_value);
    k.reason = w.intern(d.reason);
    k.effect = w.intern(d.effect);
    k.avoidable = d.avoidable;
    k.prov = KZ_NONE32;
    devs.push_back(k);
  }
  std::vector<kz_basis_item> items;
  for (const Item& it : in.record.items) {
    kz_basis_item b;
    std::memset(&b, 0, sizeof b);
    b.name = w.intern(it.name);
    b.value = w.intern(it.value);
    b.policy = it.policy;
    b.prov = KZ_NONE32;
    b.deviation = it.deviation >= 0 ? static_cast<uint32_t>(it.deviation) : KZ_NONE32;
    items.push_back(b);
  }
  if (!items.empty() && !(s = w.add_section(KZ_SEC_BASIS, items)).ok()) return s;
  if (!devs.empty() && !(s = w.add_section(KZ_SEC_DEVIATIONS, devs)).ok()) return s;
  std::vector<kz_file_hash> files(1);
  std::memset(&files[0], 0, sizeof(kz_file_hash));
  files[0].path = w.intern(in.exe_path);
  std::memcpy(files[0].sha256, in.exe_sha256.data(), 32);
  files[0].size = in.exe_size;
  files[0].prov = KZ_NONE32;
  for (const FileIdentity& f : in.also) {
    kz_file_hash h;
    std::memset(&h, 0, sizeof h);
    h.path = w.intern(f.path);
    std::memcpy(h.sha256, f.sha256.data(), 32);
    h.size = f.size;
    h.prov = KZ_NONE32;
    files.push_back(h);
  }
  if (!(s = w.add_section(KZ_SEC_FILE_HASHES, files)).ok()) return s;
  return w.finish(out);
}

}  // namespace kz::probe
