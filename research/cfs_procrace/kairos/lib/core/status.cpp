#include "kz/core/status.hpp"

namespace kz {

const char* err_name(Err e) {
  switch (e) {
    case Err::Ok: return "Ok";
    case Err::Io: return "Io";
    case Err::BadMagic: return "BadMagic";
    case Err::BadAbi: return "BadAbi";
    case Err::Truncated: return "Truncated";
    case Err::BadHeader: return "BadHeader";
    case Err::BadSection: return "BadSection";
    case Err::BadCrc: return "BadCrc";
    case Err::BadPadding: return "BadPadding";
    case Err::NotCanonical: return "NotCanonical";
    case Err::BadRecord: return "BadRecord";
    case Err::BadReserved: return "BadReserved";
    case Err::BadRef: return "BadRef";
    case Err::BadParent: return "BadParent";
    case Err::Duplicate: return "Duplicate";
    case Err::Invalid: return "Invalid";
  }
  return "?";
}

std::string Status::describe() const {
  if (ok()) return "Ok";
  return std::string(err_name(code)) + ": " + message;
}

}  // namespace kz
