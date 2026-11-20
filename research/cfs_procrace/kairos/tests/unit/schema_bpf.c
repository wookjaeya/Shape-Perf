/*
 * schema_bpf.c -- compiled by `clang -target bpf -ffreestanding -O2 -c` (see CMakeLists.txt)
 * to show that kz_schema.h can be included by a BPF program: no libc, no host headers.
 */
#include "kz/schema/kz_schema.h"

unsigned int kz_bpf_fill(struct kz_raw_ev *ev, unsigned int site, unsigned long long t_ns);

unsigned int kz_bpf_fill(struct kz_raw_ev *ev, unsigned int site, unsigned long long t_ns) {
  ev->t_ns = t_ns;
  ev->site = site;
  ev->phase = KZ_PHASE_ENTRY;
  ev->a[0] = site;
  return (unsigned int)sizeof(*ev);
}
