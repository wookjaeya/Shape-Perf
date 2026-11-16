# derive_args.sh -- sourced by derive_c1.sh and derive_c5.sh: the optional plan inputs of a derived build.
#
#   --manifest FILE   the site manifest (kzs-sites) the plugin reads
#   --modules TABLE   the module table: lines "<id><TAB><name><TAB><source prefix>" and one line "default<TAB><id>";
#                     a prefix starting with @ROOT/ is taken relative to the build's source root (C1: the work space
#                     <W>; C5: the release tree <W>/b/<release>); '#' lines are comments.  The plugin gets one
#                     -fplugin-arg-kzgcc-module=<id>:<prefix> per line and -fplugin-arg-kzgcc-default-module=<id>.
#   --exes LIST       the executables to hash and check: lines "<group><TAB><path>" with the path relative to the
#                     install root (C1: <W>/ws, for example exe/tx/core-tx; C5: the release tree, for example
#                     build/cpu1/exe/core-linux.bin) or absolute; the files of one group are checked together (bincheck.sh)
#   --path-map FILE   the map of the reference build's absolute paths into this work space (path_map.sh; C1: the
#                     <W>/path_map.tsv that derive_c1.sh prepare writes from its copies).  With it, every absolute module
#                     prefix and executable path is mapped, and one that no map line covers is refused; relative ones and
#                     @ROOT/ stay as they are.  A generated plan names the reference build's paths (kz-static plan writes
#                     them from the basis), so derive_plan.sh requires it.
# The first three come together or not at all; --path-map only with them.  Without them the scripts keep their old
# fixed values (only for reruns of the old hand manifests).  The entry for generated plans is derive_plan.sh, which
# requires all four and checks that the module table names the plan sidecar it came from.
# Sets: PLAN_MODE (0|1), PLAN_MANIFEST, PLAN_MODULES, PLAN_EXES, PLAN_PATH_MAP.  On a bad argument: message and exit 64.
PLAN_MODE=0; PLAN_MANIFEST=; PLAN_MODULES=; PLAN_EXES=; PLAN_PATH_MAP=
. "$(dirname "${BASH_SOURCE[0]}")/path_map.sh"
derive_plan_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --manifest) PLAN_MANIFEST=${2:-}; shift 2 ;;
      --modules) PLAN_MODULES=${2:-}; shift 2 ;;
      --exes) PLAN_EXES=${2:-}; shift 2 ;;
      --path-map) PLAN_PATH_MAP=${2:-}; shift 2 ;;
      *) echo "unknown argument $1 (plan inputs: --manifest FILE --modules TABLE --exes LIST [--path-map FILE])" >&2; exit 64 ;;
    esac
  done
  if [ -z "$PLAN_MANIFEST$PLAN_MODULES$PLAN_EXES$PLAN_PATH_MAP" ]; then PLAN_MODE=0; return 0; fi
  [ -n "$PLAN_MANIFEST" ] && [ -n "$PLAN_MODULES" ] && [ -n "$PLAN_EXES" ] || { echo "refusing: --manifest, --modules and --exes come together (--path-map only with them)" >&2; exit 64; }
  local f
  for f in "$PLAN_MANIFEST" "$PLAN_MODULES" "$PLAN_EXES"; do [ -s "$f" ] || { echo "refusing: $f is missing or empty" >&2; exit 64; }; done
  if [ -n "$PLAN_PATH_MAP" ]; then path_map_load "$PLAN_PATH_MAP" || exit 64; fi
  derive_module_flags @CHECK > /dev/null || exit 64
  derive_exe_paths > /dev/null || exit 64
  [ -n "$(derive_exe_paths)" ] || { echo "refusing: $PLAN_EXES names no executable" >&2; exit 64; }
  PLAN_MODE=1
}
# derive_map PATH: PATH through the path map when one is given (an absolute path no line covers: rc 1), else unchanged
derive_map() {
  if [ -n "$PLAN_PATH_MAP" ]; then path_map_apply "$1"; else printf '%s\n' "$1"; fi
}
# derive_module_flags ROOT: the plugin's module options from the module table (ROOT replaces @ROOT)
derive_module_flags() {
  local root=$1 out= id name pre def= n=0
  while IFS=$'\t' read -r id name pre; do
    case "$id" in ''|'#'*) continue ;; esac
    if [ "$id" = default ]; then
      case "$name" in ''|*[!0-9]*) echo "module table: bad default line" >&2; return 1 ;; esac
      def=$name; continue
    fi
    case "$id" in *[!0-9]*) echo "module table: bad module id $id" >&2; return 1 ;; esac
    [ -n "$pre" ] || { echo "module table: no prefix for module $id" >&2; return 1; }
    case "$pre" in
      @ROOT/*) pre=${pre/#@ROOT/$root} ;;
      *) pre=$(derive_map "$pre") || { echo "module table: the prefix of module $id is not covered by the path map" >&2; return 1; } ;;
    esac
    out+="${out:+ }-fplugin-arg-kzgcc-module=$id:$pre"; n=$((n + 1))
  done < "$PLAN_MODULES"
  [ -n "$def" ] || { echo "module table: no default line" >&2; return 1; }
  echo "$out${out:+ }-fplugin-arg-kzgcc-default-module=$def"
}
# derive_exe_refs [GROUP]: the paths of the executable list (of one group) as the list names them
derive_exe_refs() {
  awk -F'\t' -v g="${1:-}" '$0 !~ /^#/ && NF >= 2 && (g == "" || $1 == g) {print $2}' "$PLAN_EXES"
}
# derive_exe_paths [GROUP]: the same paths in this work space (through the path map when one is given); rc 1 if one
# is not covered
derive_exe_paths() {
  local p
  while IFS= read -r p; do derive_map "$p" || return 1; done < <(derive_exe_refs "${1:-}")
}
derive_exe_groups() {
  awk -F'\t' '$0 !~ /^#/ && NF >= 2 && !seen[$1]++ {print $1}' "$PLAN_EXES"
}
