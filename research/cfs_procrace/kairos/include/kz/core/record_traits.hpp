// record_traits.hpp -- maps each record type to its name in the registry, so that typed
// container accessors can check that a section holds the record type they expect.
#pragma once

#include "kz/schema/kz_schema.h"

namespace kz {

template <class T>
struct RecordName;  // no definition: only schema records have a name

#define KZ_DEFINE_RECORD_NAME(T, LIST) \
  template <>                          \
  struct RecordName<T> {               \
    static constexpr const char* value = #T; \
  };
KZ_ALL_RECORDS(KZ_DEFINE_RECORD_NAME)
#undef KZ_DEFINE_RECORD_NAME

}  // namespace kz
