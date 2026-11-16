# derive_args.sh -- sourced by derive_c1.sh and derive_c5.sh: the optional plan inputs of a derived build.
#
#   --manifest FILE   the site manifest (kzs-sites) the plugin reads
#   --modules TABLE   the module table: lines "<id><TAB><name><TAB><source prefix>" and one line "default<TAB><id>";
#                     a prefix starting with @ROOT/ is taken relative to the build's source root (C1: the work space
#                     <W>; C5: the release tree <W>/b/<release>); '#' lines are comments.  The plugin gets one
#                     -fplugin-arg-kzgcc-module=<id>:<prefix> per line and -fplugin-arg-kzgcc-default-module=<id>.
#   --exes LIST       the executables to hash and check: lines "<group><TAB><path>" with the path relative to the
#                     install root (C1: <W>/ws, for example exe/tx/core-tx; C5: the release tree, for example
#                     build/cpu1/exe/core-linux.bin); the files of one group are checked together (bincheck.sh)
# The three come together or not at all.  Without them the scripts keep their old fixed values (only for reruns of the
# old hand manifests).  The entry for generated plans is derive_plan.sh, which requires all three and checks that the
# module table and the manifest name the plan sidecar they came from.
# Sets: PLAN_MODE (0|1), PLAN_MANIFEST, PLAN_MODULES, PLAN_EXES.  On a bad argument: message and exit 64.
PLAN_MODE=0; PLAN_MANIFEST=; PLAN_MODULES=; PLAN_EXES=
derive_plan_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --manifest) PLAN_MANIFEST=${2:-}; shift 2 ;;
      --modules) PLAN_MODULES=${2:-}; shift 2 ;;
      --exes) PLAN_EXES=${2:-}; shift 2 ;;
      *) echo "unknown argument $1 (plan inputs: --manifest FILE --modules TABLE --exes LIST)" >&2; exit 64 ;;
    esac
  done
  if [ -z "$PLAN_MANIFEST$PLAN_MODULES$PLAN_EXES" ]; then PLAN_MODE=0; return 0; fi
  [ -n "$PLAN_MANIFEST" ] && [ -n "$PLAN_MODULES" ] && [ -n "$PLAN_EXES" ] || { echo "refusing: --manifest, --modules and --exes come together" >&2; exit 64; }
  local f
  for f in "$PLAN_MANIFEST" "$PLAN_MODULES" "$PLAN_EXES"; do [ -s "$f" ] || { echo "refusing: $f is missing or empty" >&2; exit 64; }; done
  derive_module_flags @CHECK > /dev/null || exit 64
  [ -n "$(derive_exe_paths)" ] || { echo "refusing: $PLAN_EXES names no executable" >&2; exit 64; }
  PLAN_MODE=1
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
    pre=${pre/#@ROOT/$root}
    out+="${out:+ }-fplugin-arg-kzgcc-module=$id:$pre"; n=$((n + 1))
  done < "$PLAN_MODULES"
  [ -n "$def" ] || { echo "module table: no default line" >&2; return 1; }
  echo "$out${out:+ }-fplugin-arg-kzgcc-default-module=$def"
}
# derive_exe_paths [GROUP]: the paths of the executable list (of one group)
derive_exe_paths() {
  awk -F'\t' -v g="${1:-}" '$0 !~ /^#/ && NF >= 2 && (g == "" || $1 == g) {print $2}' "$PLAN_EXES"
}
derive_exe_groups() {
  awk -F'\t' '$0 !~ /^#/ && NF >= 2 && !seen[$1]++ {print $1}' "$PLAN_EXES"
}
