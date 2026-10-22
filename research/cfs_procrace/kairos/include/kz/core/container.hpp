// container.hpp -- KZ container I/O (SPEC 2.3).
//
// File layout, all little-endian, every region 8-byte aligned and zero padded:
//
//   kz_header                              (160 bytes)
//   kz_parent[n_parents]                   sorted by (kind, role, sha256), no duplicates
//   kz_section_entry[n_sections]           strictly ascending by section type
//   section payloads                       in section-table order; each an array of
//                                          fixed-layout records (or bytes, for STRINGS)
//
// The artifact hash is the SHA-256 of the whole file.  A child lists the artifact hash of
// every parent it was computed from (Merkle style), so a hash binds the whole input chain.
// The canonical layout is the only layout the reader accepts, so equal content gives equal
// bytes and an equal hash: "content" is the records of every section (in the order the
// producer gives them), the parents and the three header hashes.  The STRINGS section is a
// function of that content (see below), so the order in which a producer interns strings
// does not matter.  Nothing time- or host-dependent is stored by this layer.
//
// STRINGS is canonical (lib/core/string_pool.hpp): byte 0 is NUL, then every distinct string
// that a record references, sorted byte-wise, each followed by NUL.  The writer builds it in
// finish() and rewrites the kz_str offsets of the records; the reader checks it in
// verify_content() (verify_strings()).  The kz_str values that intern() returns are
// therefore provisional: they are valid for add_section() and mean nothing outside it.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "kz/core/identity.hpp"
#include "kz/core/record_traits.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz {

using Bytes = std::vector<uint8_t>;

// Magic bytes: PNG-style, so that text-mode transfers and truncation are detected.
constexpr uint8_t kMagic[8] = {0x89, 'K', 'Z', 'C', '\r', '\n', 0x1A, '\n'};

// Provisional string table of a writer.  Offset 0 is the empty string (one leading zero
// byte); equal strings share one entry, in the order of the first intern() call.  The writer
// re-packs the strings that records reference into the canonical pool when it finishes, so
// this order never reaches the file.  A hash index makes intern() O(length).
class StringPool {
 public:
  StringPool();
  kz_str intern(std::string_view s);
  const Bytes& bytes() const { return bytes_; }

 private:
  Bytes bytes_;
  std::unordered_map<std::string, kz_str> index_;  // lookup only: never iterated for output
};

class ContainerWriter {
 public:
  explicit ContainerWriter(uint32_t artifact_kind);

  void set_basis_hash(const Digest& d) { basis_ = d; }
  void set_profile_hash(const Digest& d) { profile_ = d; }
  void set_tool_hash(const Digest& d) { tool_ = d; }
  // Adds a parent artifact (the artifact hash of the parent file).  Order of calls does not
  // matter: parents are stored sorted.  Adding the same parent twice is an error.
  Status add_parent(uint32_t parent_kind, uint32_t role, const Digest& artifact_hash);

  // String pool shared by all records of this container.  The returned reference is
  // provisional (see the note at the top of this file): store it in a record of a section
  // added to this writer and nothing else.
  kz_str intern(std::string_view s) { return pool_.intern(s); }

  // By default the writer is strict: add_section rejects a record whose reserved (`rsv`)
  // field is not zero, and finish() rejects a kz_str that does not resolve in the string
  // pool, so that uninitialized data can never reach a hashed artifact.  Tests of the
  // reader turn this off to build damaged containers (an unresolved kz_str is then left
  // as it is).
  void set_check_reserved(bool on) { check_reserved_ = on; }

  // Adds a section of records of the type the registry binds to `section_type`.
  Status add_section_raw(uint32_t section_type, const void* records, size_t count, size_t record_size);
  template <class T>
  Status add_section(uint32_t section_type, const std::vector<T>& records) {
    const SectionInfo* info = find_section(section_type);
    if (info == nullptr || info->record == nullptr)
      return Status::error(Err::BadSection, "section type " + std::to_string(section_type) + " is not registered");
    if (std::string_view(info->record->name) != RecordName<T>::value)
      return Status::error(Err::BadRecord, std::string("section ") + info->name + " holds " + info->record->name +
                                               ", not " + RecordName<T>::value);
    return add_section_raw(section_type, records.data(), records.size(), sizeof(T));
  }

  // Produces the canonical bytes.  The STRINGS section is always written.
  Status finish(Bytes* out) const;
  Status write_file(const std::string& path) const;

 private:
  struct Section {
    uint32_t type;
    size_t count;
    size_t record_size;
    Bytes data;
  };
  uint32_t artifact_kind_;
  Digest basis_ = {};
  Digest profile_ = {};
  Digest tool_ = {};
  std::vector<kz_parent> parents_;
  std::vector<Section> sections_;
  StringPool pool_;
  bool check_reserved_ = true;
};

struct ParseOptions {
  // Section types that the registry does not know are an error unless this is set; the
  // reader then keeps them as opaque byte ranges.
  bool allow_unknown_sections = false;
};

class Container {
 public:
  // Parses and structurally verifies `bytes` (magic, ABI, header and table CRC, canonical
  // layout, zero padding, section CRCs, record sizes and schema versions).  Content
  // checks (reserved fields, references, strings) are in verify_content().
  static Status parse(Bytes bytes, Container* out, const ParseOptions& options = ParseOptions());
  static Status read_file(const std::string& path, Container* out, const ParseOptions& options = ParseOptions());

  const kz_header& header() const { return header_; }
  const std::vector<kz_parent>& parents() const { return parents_; }
  const std::vector<kz_section_entry>& section_table() const { return table_; }
  const Bytes& bytes() const { return bytes_; }
  // SHA-256 of the whole file: the identity of this artifact.
  const Digest& artifact_hash() const { return hash_; }
  Digest basis_hash() const { return to_digest(header_.basis_sha256); }
  Digest profile_hash() const { return to_digest(header_.profile_sha256); }
  Digest tool_hash() const { return to_digest(header_.tool_sha256); }

  const kz_section_entry* find_section(uint32_t type) const;
  // Payload bytes of a section (empty if absent).
  const uint8_t* section_data(uint32_t type, size_t* size) const;

  template <class T>
  Status read_section(uint32_t section_type, std::vector<T>* out) const {
    out->clear();
    const SectionInfo* info = kz::find_section(section_type);
    if (info == nullptr || info->record == nullptr || std::string_view(info->record->name) != RecordName<T>::value)
      return Status::error(Err::BadRecord, "section " + std::to_string(section_type) + " does not hold " + RecordName<T>::value);
    const kz_section_entry* e = find_section(section_type);
    if (e == nullptr) return Status::success();  // absent section: no records
    out->resize(static_cast<size_t>(e->count));
    if (e->count > 0) copy_section(*e, out->data());
    return Status::success();
  }

  // Resolves a string reference; false if it is out of range.
  bool string(const kz_str& s, std::string* out) const;

  // Content checks.  Each walks every record of every registered section using the field
  // descriptors, so it covers records added to the schema later without change.
  //   verify_reserved: every `rsv` field is zero
  //   verify_refs:     every kz_ref is null, a record of this container, or names a parent;
  //                    every kz_str lies inside the STRINGS section
  //   verify_strings:  the STRINGS section is the canonical pool of the strings that the
  //                    records reference (sorted, no duplicate, no unreferenced string) and
  //                    every kz_str points at the canonical offset of its string.  A
  //                    container with a section type this build does not know (opaque
  //                    sections, ParseOptions::allow_unknown_sections) may reference
  //                    strings that cannot be seen, so only the references of the known
  //                    sections are checked and the pool is not compared as a whole.
  Status verify_reserved() const;
  Status verify_refs() const;
  Status verify_strings() const;
  Status verify_content() const;

  // Merkle check: `parent` is listed among this container's parents, with matching kind.
  Status verify_parent(const Container& parent) const;

 private:
  static Digest to_digest(const uint8_t (&a)[32]) {
    Digest d;
    for (size_t i = 0; i < 32; ++i) d[i] = a[i];
    return d;
  }
  void copy_section(const kz_section_entry& e, void* dst) const;

  Bytes bytes_;
  kz_header header_{};
  std::vector<kz_parent> parents_;
  std::vector<kz_section_entry> table_;
  Digest hash_{};
};

// File helpers.
Status read_whole_file(const std::string& path, Bytes* out);
Status write_whole_file(const std::string& path, const Bytes& data);

// The tool hash for the header (kz::tool_hash) is declared in kz/core/identity.hpp.

}  // namespace kz
