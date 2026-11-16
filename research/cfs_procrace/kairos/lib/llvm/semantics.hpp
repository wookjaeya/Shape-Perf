// semantics.hpp -- the semantics table joined to a build (facts section SEMANTICS, kz_facts.h revision 10; static_facts.md
// 5t): `kz-static extract --semantics FILE` reads semantics data files (the framework-layer data under semantics/, generated
// by the mechanical scan of docs/semantics_coverage.md) and writes one row per (api, column item) for the functions the
// build calls, with a verification state.
//
// Three layers.  P0: the entries of the P0 tables the build calls, with the source state the table gives them, and the
// attributes of their declaration in the C library headers of the build when a P0 declarations file pins those headers.
// framework: the entries of a framework data file that apply to the build (the build holds the entry's definition file
// with the pinned sha256 and defines the function on that line, or holds its declaration header with the pinned sha256),
// for the functions the build calls: the documented returns and the argument domains (doc-collated), the constants the
// body returns in these facts (body-derived) and their agreement with the documented returns, and the roles the facts
// derive (delay, terminal, spawn).  status-space: the success constant and the severity masks of each component one of
// whose entries applies.
//
// No Clang types.  The data files are the only input besides the facts; the names they carry are data, never constants of
// this code (lint semantics-names keeps every API name of semantics/ out of the engine).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kzs {

// One semantics data file, as read (format `semantics-data 1`, docs/semantics_coverage.md).
struct SemDataFile {
  std::string path, sha256, layer, label, version, rule;
  std::map<std::string, std::string> files;  // relative path -> sha256 (the `file` lines)
  struct Const {
    std::string kind, name, file;
    int64_t value = 0;
    uint32_t line = 0;
  };
  std::vector<Const> groups;        // `group <kind> NAME value file:line`
  std::vector<Const> status_space;  // `status-space success|severity NAME value file:line`
  struct Ret {
    std::string name, file;
    int64_t value = 0;
    uint32_t line = 0;
  };
  struct Arg {
    uint32_t pos = 0;
    std::string kind, by, file;
    uint32_t line = 0;
  };
  struct Api {
    std::string name, decl_file, def_file, ret_kind, by, rule, attrs, table;
    uint32_t decl_line = 0, def_line = 0;
    std::vector<Ret> rets;
    std::vector<Arg> args;
  };
  std::vector<Api> apis;
  std::string source_name, source_sha256;  // P0: the local copy of C library source text (`source` line)
};

kz::Status load_semantics_file(const std::string& path, SemDataFile* out);

// A SEMANTICS row before its strings are interned (serialize.cpp writes them).
struct SemRow {
  int64_t value = 0;
  uint32_t fn = KZ_NONE32, pos = KZ_NONE32, decl_fh = KZ_NONE32, decl_line = 0, src_line = 0;
  uint8_t layer = 0, column = 0, verify = 0, origin = 0;
  uint32_t flags = 0;
  std::string api, component, item, source;
};

struct SemResult {
  std::vector<SemRow> rows;
  std::vector<std::pair<std::string, std::string>> notes;  // BASIS items
};

// The rows of the facts container `facts` (an extraction with the extended facts) under the data files.
kz::Status semantics_rows(const kz::Container& facts, const std::vector<SemDataFile>& data, SemResult* out);

const char* semantics_revision();

}  // namespace kzs
