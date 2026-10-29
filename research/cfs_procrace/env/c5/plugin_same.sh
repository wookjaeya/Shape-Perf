#!/usr/bin/env bash
# plugin_same.sh <compile_commands.json> <binds dir> <old kzgcc.so> <new kzgcc.so> <out> -- the instrumented units (those
# with a binding file in <binds dir>, the build's own) of an existing derived build compiled again, with the plugin it
# was built with and with another, from the recorded compile
# commands (same directory, same options, only the plugin path, the binding output directory and the object path
# replaced); the objects and the binding lines of the two plugins are compared.  Used for kzgcc 0.2 -> 0.3 (E-ID1: the
# owner-publish link) on the R2 and C1 derived builds, whose manifests have no kzs-sites 3 reader.  Tool check only.
# A unit-test coverage compile (-fprofile-arcs; the plugin's skip-coverage) is left out.
set -uo pipefail
CC_JSON=${1:?usage: plugin_same.sh <compile_commands.json> <binds dir> <old kzgcc.so> <new kzgcc.so> <out>}; BD=${2:?binds}
OLDP=${3:?old}; NEWP=${4:?new}; O=${5:?out}
mkdir -p "$O"
REC="$O/plugin_same.record"
echo "# plugin_same.sh $(date -u +%FT%TZ): $CC_JSON; old $OLDP sha256 $(sha256sum "$OLDP" | cut -c1-64); new $NEWP sha256 $(sha256sum "$NEWP" | cut -c1-64)" > "$REC"
units=$(sed -n 's/^# kzgcc.* unit=\([^ ]*\) .*/\1/p' "$BD"/*.kzs | sort -u)
n=0; same_o=0; same_b=0
while IFS= read -r -d '' dir && IFS= read -r -d '' file && IFS= read -r -d '' cmd; do
  case "$cmd" in *-fplugin=*) ;; *) continue ;; esac
  case "$cmd" in *-fprofile-arcs*) continue ;; esac
  echo "$units" | grep -qxF "$file" || continue
  n=$((n + 1))
  # the same command text for both plugins (the debug information records the command line): the plugin is copied to
  # one path, and the binding directory and the object are one path, moved away after each compile
  c=$(printf '%s' "$cmd" | sed -E "s#-fplugin=[^ ]+#-fplugin=$O/kzgcc.so#; s#-fplugin-arg-kzgcc-out=[^ ]+#-fplugin-arg-kzgcc-out=$O/binds#; s#(^| )-o [^ ]+#\\1-o $O/u.o#")
  for w in old new; do
    p=$OLDP; [ "$w" = new ] && p=$NEWP
    cp "$p" "$O/kzgcc.so"
    rm -rf "$O/binds" "$O/u.o"; mkdir -p "$O/binds" "$O/$w"
    ( cd "$dir" && eval "$c" ) > "$O/$w/u$n.log" 2>&1 || echo "unit $n ($file): $w compile failed" >> "$REC"
    mv "$O/u.o" "$O/$w/u$n.o"; rm -rf "$O/$w/binds_$n"; mv "$O/binds" "$O/$w/binds_$n"
  done
  if cmp -s "$O/old/u$n.o" "$O/new/u$n.o"; then same_o=$((same_o + 1)); r=same; else r=DIFFERENT; fi
  if cmp -s <(cat "$O"/old/binds_$n/*.kzs | grep '^bind ') <(cat "$O"/new/binds_$n/*.kzs | grep '^bind '); then same_b=$((same_b + 1)); rb=same; else rb=DIFFERENT; fi
  echo "unit $n $file: object $r, bind lines $rb ($(cat "$O"/new/binds_$n/*.kzs | grep -c '^bind ') bindings)" >> "$REC"
done < <(jq --raw-output0 '.[] | .directory, .file, .command' "$CC_JSON")
echo "units $n: objects identical $same_o, bind lines identical $same_b" >> "$REC"
cat "$REC"
