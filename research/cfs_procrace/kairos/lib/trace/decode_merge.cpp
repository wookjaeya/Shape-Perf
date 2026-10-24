// decode_merge.cpp -- see include/kz/trace/decode_merge.hpp.
#include "kz/trace/decode_merge.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_map>

namespace kz::trace {

namespace {

bool less_key(const PerfRecord& a, const PerfRecord& b) {
  if (a.time != b.time) return a.time < b.time;
  if (a.buffer_cpu != b.buffer_cpu) return a.buffer_cpu < b.buffer_cpu;
  return a.order < b.order;
}

int file_of_map(const std::vector<TracedFile>& files, const kz_map_ev& m) {
  for (size_t f = 0; f < files.size(); ++f)
    if (files[f].known && m.ino == files[f].ino && m.dev_major == files[f].dev_major && m.dev_minor == files[f].dev_minor) return static_cast<int>(f);
  return -1;
}

}  // namespace

Status decode_run(const DecodeInput& in, DecodedRun* out) {
  *out = DecodedRun();
  DecodeStats& st = out->stats;
  st.site_hits.assign(in.sites.size(), 0);

  // 1. Parse every stream.
  std::vector<PerfRecord> all;
  Status first_error = Status::success();
  for (const CpuStream& s : in.streams) {
    ParseStats ps;
    Status r = parse_records(s.bytes.data(), s.bytes.size(), s.cpu, in.layouts, &all, &ps);
    st.parse.records += ps.records;
    st.parse.samples += ps.samples;
    st.parse.other += ps.other;
    st.parse.unknown_id += ps.unknown_id;
    out->stream_status.push_back(r);
    if (!r.ok()) {
      ++st.stream_errors;
      if (first_error.ok()) first_error = r;
    }
  }
  // Stable total order: time, CPU, arrival.
  std::sort(all.begin(), all.end(), less_key);

  // The traced files: the primary and the further files of the plan (index = SiteSpec::binary).
  std::vector<TracedFile> files;
  files.push_back(in.traced);
  files.insert(files.end(), in.also.begin(), in.also.end());
  auto file_of = [&](const PerfRecord& r) -> int {
    for (size_t f = 0; f < files.size(); ++f)
      if (files[f].known && r.ino == files[f].ino && r.dev_major == files[f].dev_major && r.dev_minor == files[f].dev_minor) return static_cast<int>(f);
    return -1;
  };
  out->file_bias_known.assign(files.size(), false);
  out->file_bias.assign(files.size(), 0);

  // 2. Which processes are traced.
  std::vector<uint32_t> pids;
  for (const PerfRecord& r : all) {
    if (r.kind == RecKind::Sample) pids.push_back(r.pid);
    if (r.kind == RecKind::Mmap2 && file_of(r) >= 0) pids.push_back(r.pid);
  }
  std::sort(pids.begin(), pids.end());
  pids.erase(std::unique(pids.begin(), pids.end()), pids.end());
  out->traced_pids = pids;
  auto traced = [&](uint32_t pid) { return std::binary_search(pids.begin(), pids.end(), pid); };

  // 3. Samples, side events, maps, loss -- in the merged order.
  std::unordered_map<uint32_t, uint32_t> next_tseq;
  std::unordered_map<uint64_t, uint64_t> throttled_since;  // event id -> start time
  uint64_t last_time = 0;
  for (const PerfRecord& r : all) {
    if (r.time > last_time) last_time = r.time;
    switch (r.kind) {
      case RecKind::Sample: {
        const EventLayout* lay = in.layouts.find(r.id);
        if (lay == nullptr || lay->site == KZ_NONE32 || lay->site >= in.sites.size()) {
          ++st.samples_without_site;
          break;
        }
        const SiteSpec& site = in.sites[lay->site];
        kz_raw_ev ev;
        std::memset(&ev, 0, sizeof ev);
        ev.t_ns = r.time;
        ev.tid = r.tid;
        ev.tseq = next_tseq[r.tid]++;
        ev.site = site.site;
        ev.phase = phase_of(site);
        uint16_t flags = 0;
        uint64_t vals[4];
        if (!apply_slots(site, r, vals, &flags).ok()) {
          ++st.slot_failures;
          break;
        }
        ev.flags = flags;
        if ((flags & KZ_RAW_FLAG_TRUNC) != 0) ++st.slots_truncated;
        for (int i = 0; i < 4; ++i) ev.a[i] = vals[i];
        out->events.push_back(ev);
        ++st.site_hits[lay->site];
        // The load bias of the site's file: an entry or point sample stops at the probed instruction itself.
        if (site.kind != KZ_SITE_KIND_RET_PROBE && site.vaddr != 0 && site.binary < files.size()) {
          const uint64_t bias = r.ip - site.vaddr;
          if (!out->file_bias_known[site.binary]) {
            out->file_bias_known[site.binary] = true;
            out->file_bias[site.binary] = bias;
          } else if (bias != out->file_bias[site.binary]) {
            ++st.bias_mismatches;
          }
        }
        break;
      }
      case RecKind::Fork:
      case RecKind::Exit:
      case RecKind::Comm: {
        if (!traced(r.pid)) {
          ++st.side_dropped;
          break;
        }
        SideEvent se;
        std::memset(&se.rec, 0, sizeof se.rec);
        se.rec.t_ns = r.time;
        se.rec.pid = r.pid;
        se.rec.tid = r.tid;
        se.rec.ppid = r.ppid;
        se.rec.ptid = r.ptid;
        se.rec.kind = r.kind == RecKind::Fork ? KZ_SIDE_KIND_FORK : r.kind == RecKind::Exit ? KZ_SIDE_KIND_EXIT : KZ_SIDE_KIND_COMM;
        se.rec.cpu = r.cpu <= 0xffffu ? static_cast<uint16_t>(r.cpu) : 0xffffu;
        if (r.kind == RecKind::Comm) {
          se.comm = r.text;
          if ((r.misc & perfabi::kMiscCommExec) != 0) se.rec.flags |= KZ_SIDE_FLAG_COMM_EXEC;
        }
        out->side.push_back(std::move(se));
        break;
      }
      case RecKind::Mmap2: {
        const int fi = file_of(r);
        if (fi < 0) {
          ++st.maps_dropped;
          break;
        }
        MapEvent me;
        std::memset(&me.rec, 0, sizeof me.rec);
        me.rec.t_ns = r.time;
        me.rec.addr = r.addr;
        me.rec.len = r.len;
        me.rec.pgoff = r.pgoff;
        me.rec.ino = r.ino;
        me.rec.pid = r.pid;
        me.rec.tid = r.tid;
        me.rec.dev_major = r.dev_major;
        me.rec.dev_minor = r.dev_minor;
        me.rec.prot = r.prot;
        me.rec.flags = r.map_flags;
        me.path = r.text;
        // The load bias of this mapping, from the plan's sites that lie in the mapped file range:
        // the runtime address of file offset X is addr + (X - pgoff), its link-time address is the
        // site's vaddr.  All such sites must agree.
        bool agree = true, any = false;
        uint64_t bias = 0;
        for (const SiteSpec& site : in.sites) {
          if (site.binary != static_cast<uint16_t>(fi)) continue;
          if (site.vaddr == 0 || site.file_offset < r.pgoff || site.file_offset - r.pgoff >= r.len) continue;
          const uint64_t b = r.addr + (site.file_offset - r.pgoff) - site.vaddr;
          if (!any) { bias = b; any = true; }
          else if (b != bias) agree = false;
        }
        if (any && !agree) ++st.bias_mismatches;
        if (any && agree) {
          me.rec.bias = bias;
          me.rec.bias_known = 1;
        }
        out->maps.push_back(std::move(me));
        break;
      }
      case RecKind::Lost: {
        kz_loss l;
        std::memset(&l, 0, sizeof l);
        l.source = KZ_LOSS_SOURCE_PERF_RECORD_LOST;
        l.cpu = r.buffer_cpu;
        const EventLayout* lay = in.layouts.find(r.id);
        l.site = lay != nullptr ? lay->site : KZ_NONE32;
        l.tid = r.tid;
        l.count = r.lost;
        l.t_ns = r.time;
        out->loss.push_back(l);
        ++st.lost_reports;
        st.dropped_records += r.lost;
        break;
      }
      case RecKind::Throttle:
        throttled_since.emplace(r.id, r.time);  // a second THROTTLE without an UNTHROTTLE keeps the first
        break;
      case RecKind::Unthrottle: {
        auto it = throttled_since.find(r.id);
        if (it == throttled_since.end()) break;  // an unthrottle of an interval whose start we did not see
        kz_loss l;
        std::memset(&l, 0, sizeof l);
        l.source = KZ_LOSS_SOURCE_PERF_THROTTLE;
        l.cpu = r.buffer_cpu;
        const EventLayout* lay = in.layouts.find(r.id);
        l.site = lay != nullptr ? lay->site : KZ_NONE32;
        l.tid = KZ_NONE32;
        l.count = r.time >= it->second ? r.time - it->second : 0;
        l.t_ns = it->second;
        out->loss.push_back(l);
        ++st.throttle_intervals;
        throttled_since.erase(it);
        break;
      }
      case RecKind::Other:
        break;
    }
  }
  // Throttle intervals still open at the end of the stream: the end is unknown (tseq_first = 1).
  std::vector<std::pair<uint64_t, uint64_t>> open(throttled_since.begin(), throttled_since.end());
  std::sort(open.begin(), open.end());
  for (const auto& o : open) {
    kz_loss l;
    std::memset(&l, 0, sizeof l);
    l.source = KZ_LOSS_SOURCE_PERF_THROTTLE;
    const EventLayout* lay = in.layouts.find(o.first);
    l.site = lay != nullptr ? lay->site : KZ_NONE32;
    l.tid = KZ_NONE32;
    l.tseq_first = 1;
    l.count = 0;
    l.t_ns = o.second;
    out->loss.push_back(l);
    ++st.throttle_intervals;
  }
  (void)last_time;
  out->bias_known = out->file_bias_known[0];
  out->load_bias = out->file_bias[0];
  for (const MapEvent& m : out->maps) {
    const int fi = file_of_map(files, m.rec);
    if (fi >= 0 && m.rec.bias_known != 0 && out->file_bias_known[static_cast<size_t>(fi)] && m.rec.bias != out->file_bias[static_cast<size_t>(fi)]) ++st.bias_mismatches;
  }
  std::stable_sort(out->loss.begin(), out->loss.end(), [](const kz_loss& a, const kz_loss& b) {
    if (a.t_ns != b.t_ns) return a.t_ns < b.t_ns;
    if (a.cpu != b.cpu) return a.cpu < b.cpu;
    return a.site < b.site;
  });
  return first_error;
}

}  // namespace kz::trace
