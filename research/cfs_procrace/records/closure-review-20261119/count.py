# Counts of findings.json: findings by review and severity, verdicts real / before_boot.
# Usage: python3 -I count.py findings.json > counts.txt
import collections, json, sys
d = json.load(open(sys.argv[1], encoding="utf-8"))
f = d["findings"]; v = {x["id"]: x for x in d["verdicts"]}
print("findings", len(f))
for r in ("rules", "builds", "semantics"):
    c = collections.Counter(x["severity"] for x in f if x["id"].startswith(r + "-"))
    print("review", r, "findings", sum(c.values()), " ".join("%s %d" % (s, c.get(s, 0)) for s in ("blocking", "major", "minor")))
c = collections.Counter(x["severity"] for x in f)
print("severity", " ".join("%s %d" % (s, c.get(s, 0)) for s in ("blocking", "major", "minor")))
print("verdicts", len(v))
print("real true", sum(1 for x in v.values() if x["real"] is True), "real false", sum(1 for x in v.values() if x["real"] is False))
print("before_boot true", sum(1 for x in v.values() if x.get("before_boot") is True),
      "before_boot false", sum(1 for x in v.values() if x.get("before_boot") is False),
      "before_boot absent", sum(1 for x in v.values() if "before_boot" not in x))
for x in f:
    y = v[x["id"]]
    print("id", x["id"], x["severity"], "real=" + str(y["real"]).lower(),
          "before_boot=" + ("-" if "before_boot" not in y else str(y["before_boot"]).lower()))
print("prep_open_points", len(d["prep_open_points"]), "xp_open_points", len(d["xp_open_points"]))
