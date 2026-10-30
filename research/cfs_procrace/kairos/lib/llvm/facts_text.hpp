// facts_text.hpp -- the text view of a facts container ("facts dump").
//
// A deterministic, human-oriented rendering of the extraction layer: records are joined to what
// they refer to (function indices become names, expression roots become trees, call arguments
// are listed under their call), so a reader does not have to follow indices by hand.  The view
// is a function of the container alone.  The canonical record dump (`kz-dump`) stays the exact
// form; this one is for reading and for golden files.
#pragma once

#include <set>
#include <string>

#include "kz/core/container.hpp"

namespace kzs {

struct TextOptions {
  // Section names to print (empty = all): basis files functions calls derives loops iters init
  // consts writers atype locs targets spawnparams image terminals outcalls fwrites, and, when the
  // container has them, p0 derived spawnwrap (the profile-free mode) and reads returns retconsts
  // loopexits argflow paramflow ptrfields gimages lockops lockids lockset fstatus awaits (the extended facts).
  std::set<std::string> sections;
  // Only the records about this location, "Type.field" (writers, atype, init, locs, outcalls, derives, reads, argflow).
  std::string loc;
  // Only the calls to this callee (calls, derives, outcalls, targets, argflow).
  std::string callee;
  bool full_hashes = false;
};

kz::Status facts_text(const kz::Container& c, const TextOptions& options, std::string* out);

// The names a text view can show, in the order it shows them.
const char* const* facts_text_sections(size_t* count);

}  // namespace kzs
