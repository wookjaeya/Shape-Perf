// dump.hpp -- canonical text dump of records, containers and the schema layout.
//
// The dump is deterministic and canonical: one record per line, fields in declaration order,
// fixed value formats, no host- or time-dependent text.  Reserved fields are printed only
// when they are not zero.  Format summary:
//   unsigned/signed integers   decimal; sets as {a,b}; enumerations by display name, #n if unlisted
//   Hex fields                 0x<lower case hex> per element
//   Idx fields                 "none" for the all-ones sentinel, otherwise decimal
//   byte arrays                lower case hex string; other arrays [v0,v1,...]
//   kz_ref                     "-" (null), "self/<SECTION>#<index>" or "<sha256>/<SECTION>#<index>"
//   kz_str                     "text" with \" \\ \n \t \r and \xHH escapes (ASCII only)
#pragma once

#include <string>
#include <string_view>

#include "kz/core/container.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"

namespace kz {

// "record_name field=value ..." for one record.  `c` resolves kz_str fields; if null they print as str(off,len).
std::string dump_record(const kz_record_desc& rec, const void* record, const Container* c = nullptr);

// Full canonical dump of a container, ending with its artifact hash.
std::string dump_container(const Container& c);

// Records, enums, sections and field formats of the schema, as text.
std::string layout_text();
// SHA-256 of layout_text(): changes whenever any layout, enum value, section id or format changes.
Digest schema_fingerprint();

std::string escape_string(std::string_view s);

// "-" (null), "self/<SECTION>#<index>" or "<sha256>/<SECTION>#<index>": the form the dump uses.
std::string ref_to_text(const kz_ref& ref);

// Text identifying this build of libkz-core for provenance records (a BASIS item of the
// artifact that a tool writes): ABI version, schema fingerprint, source identity of libkz-core
// (kz/core/identity.hpp), compiler and language level.  The compiler and the language level are
// not part of the tool hash, so that the same sources hash the same with gcc and clang; this
// text is where they are recorded.
std::string build_identity();

}  // namespace kz
