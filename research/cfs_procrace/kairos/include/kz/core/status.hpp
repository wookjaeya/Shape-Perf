// status.hpp -- error reporting for libkz-core without exceptions.
#pragma once

#include <string>
#include <utility>

namespace kz {

enum class Err {
  Ok = 0,
  Io,            // file system error
  BadMagic,      // not a KZ container
  BadAbi,        // ABI version or header size does not match this build
  Truncated,     // the buffer ends before the layout says it does
  BadHeader,     // header fields inconsistent
  BadSection,    // section table inconsistent (order, bounds, duplicate, unknown type)
  BadCrc,        // a CRC does not match
  BadPadding,    // a padding byte is not zero
  NotCanonical,  // valid but not the canonical byte layout
  BadRecord,     // record size or schema version does not match the registry
  BadReserved,   // a reserved field is not zero
  BadRef,        // a typed reference or string reference does not resolve
  BadParent,     // parent hash table inconsistent or a parent does not match
  Duplicate,     // a section was added twice
  Invalid,       // invalid argument or invalid content
};

const char* err_name(Err e);

struct Status {
  Err code = Err::Ok;
  std::string message;

  bool ok() const { return code == Err::Ok; }
  static Status success() { return Status{}; }
  static Status error(Err c, std::string m) {
    Status s;
    s.code = c;
    s.message = std::move(m);
    return s;
  }
  // "BadCrc: section 41: ..." -- stable text for logs and tests.
  std::string describe() const;
};

}  // namespace kz
