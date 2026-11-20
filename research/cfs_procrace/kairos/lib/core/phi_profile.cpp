// phi_profile.cpp -- the Profile container: lookups, normalization, the profile hash.

#include <algorithm>

#include "kz/core/phi.hpp"

namespace kz {
namespace phi {

namespace {

template <class V>
int32_t find_by_name(const V& v, const std::string& n) {
  for (size_t i = 0; i < v.size(); ++i)
    if (v[i].name == n) return static_cast<int32_t>(i);
  return -1;
}

template <class V>
bool sort_unique(V* v, const char* what, const std::vector<std::string>& files, std::vector<Diag>* diags) {
  std::stable_sort(v->begin(), v->end(), [](const typename V::value_type& a, const typename V::value_type& b) { return a.name < b.name; });
  bool ok = true;
  for (size_t i = 1; i < v->size(); ++i) {
    if ((*v)[i].name != (*v)[i - 1].name) continue;
    Diag d;
    d.loc = (*v)[i].loc;
    d.message = std::string("duplicate ") + what + " '" + (*v)[i].name + "' (first declared at " +
                files[(*v)[i - 1].loc.file < files.size() ? (*v)[i - 1].loc.file : 0] + ":" + std::to_string((*v)[i - 1].loc.line) + ":" +
                std::to_string((*v)[i - 1].loc.col) + ")";
    diags->push_back(std::move(d));
    ok = false;
  }
  return ok;
}

template <class V>
bool sort_unique_labels(V* v, const std::string& owner, const char* what, std::vector<Diag>* diags) {
  std::stable_sort(v->begin(), v->end(), [](const typename V::value_type& a, const typename V::value_type& b) { return a.label < b.label; });
  bool ok = true;
  for (size_t i = 1; i < v->size(); ++i) {
    if ((*v)[i].label != (*v)[i - 1].label) continue;
    Diag d;
    d.loc = (*v)[i].loc;
    d.message = std::string("duplicate ") + what + " label '" + (*v)[i].label + "' in '" + owner + "'";
    diags->push_back(std::move(d));
    ok = false;
  }
  return ok;
}

void sort_unique_strings(std::vector<std::string>* v) {
  std::sort(v->begin(), v->end());
  v->erase(std::unique(v->begin(), v->end()), v->end());
}

}  // namespace

int32_t Profile::find_kind(const std::string& n) const { return find_by_name(kinds, n); }
int32_t Profile::find_const(const std::string& n) const { return find_by_name(consts, n); }
int32_t Profile::find_op(const std::string& n) const { return find_by_name(ops, n); }
int32_t Profile::find_loc(const std::string& n) const { return find_by_name(locs, n); }
int32_t Profile::find_site(const std::string& n) const { return find_by_name(sites, n); }
int32_t Profile::find_prov(const std::string& n) const { return find_by_name(provs, n); }
int32_t Profile::find_source(const std::string& n) const { return find_by_name(sources, n); }
int32_t Profile::find_rspec(const std::string& n) const { return find_by_name(rspecs, n); }
int32_t Profile::find_rule(const std::string& n) const { return find_by_name(rules, n); }
int32_t Profile::find_role(const std::string& n) const { return find_by_name(roles, n); }

bool normalize_profile(Profile* p, std::vector<Diag>* diags) {
  bool ok = true;
  ok &= sort_unique(&p->provs, "provenance record", p->files, diags);
  ok &= sort_unique(&p->sources, "source", p->files, diags);
  ok &= sort_unique(&p->consts, "constant", p->files, diags);
  ok &= sort_unique(&p->kinds, "kind", p->files, diags);
  ok &= sort_unique(&p->locs, "location", p->files, diags);
  ok &= sort_unique(&p->binds, "binding", p->files, diags);
  ok &= sort_unique(&p->sites, "site", p->files, diags);
  ok &= sort_unique(&p->roles, "role", p->files, diags);
  ok &= sort_unique(&p->ops, "operation", p->files, diags);
  ok &= sort_unique(&p->inits, "INIT fact", p->files, diags);
  ok &= sort_unique(&p->rules, "hb rule", p->files, diags);
  ok &= sort_unique(&p->rspecs, "rspec", p->files, diags);
  ok &= sort_unique(&p->trusted, "trusted entry", p->files, diags);

  auto sort_entry = [](EntryBase* e) {
    sort_unique_strings(&e->prov);
    std::stable_sort(e->guards.begin(), e->guards.end(), [](const Guard& a, const Guard& b) {
      if (a.component != b.component) return a.component < b.component;
      if (a.op != b.op) return a.op < b.op;
      return a.version < b.version;
    });
  };
  for (SourceDecl& d : p->sources) sort_entry(&d);
  for (ConstDecl& d : p->consts) sort_entry(&d);
  for (BindDecl& d : p->binds) sort_entry(&d);
  for (SiteDecl& d : p->sites) sort_entry(&d);
  for (RoleDecl& d : p->roles) sort_entry(&d);
  for (InitDecl& d : p->inits) sort_entry(&d);
  for (HbRuleDecl& d : p->rules) {
    sort_entry(&d);
    sort_unique_strings(&d.watch_kinds);
  }
  for (LocDecl& d : p->locs) sort_entry(&d);
  for (TrustedDecl& d : p->trusted) {
    sort_entry(&d);
    sort_unique_strings(&d.refutable_by);
  }
  for (KindDecl& k : p->kinds) {
    sort_entry(&k);
    std::stable_sort(k.tags.begin(), k.tags.end(), [](const TagDecl& a, const TagDecl& b) { return a.name < b.name; });
    for (size_t i = 1; i < k.tags.size(); ++i)
      if (k.tags[i].name == k.tags[i - 1].name) {
        Diag d;
        d.loc = k.tags[i].loc;
        d.message = "duplicate tag '" + k.tags[i].name + "' in kind '" + k.name + "'";
        diags->push_back(std::move(d));
        ok = false;
      }
    std::stable_sort(k.transitions.begin(), k.transitions.end(), [](const TransitionDecl& a, const TransitionDecl& b) {
      if (a.from != b.from) return a.from < b.from;
      if (a.to != b.to) return a.to < b.to;
      return a.action < b.action;
    });
    std::stable_sort(k.derived.begin(), k.derived.end(), [](const DerivedDecl& a, const DerivedDecl& b) { return a.state < b.state; });
  }
  for (OpDecl& o : p->ops) {
    sort_entry(&o);
    ok &= sort_unique_labels(&o.conjuncts, o.name, "conjunct", diags);
    ok &= sort_unique_labels(&o.effects, o.name, "effect", diags);
    for (EffectDecl& e : o.effects) {
      sort_unique_strings(&e.prov);
      sort_unique_strings(&e.refutable_by);
    }
    std::stable_sort(o.outcomes.begin(), o.outcomes.end(), [](const OutcomeDecl& a, const OutcomeDecl& b) { return a.code < b.code; });
    for (size_t i = 1; i < o.outcomes.size(); ++i)
      if (o.outcomes[i].code == o.outcomes[i - 1].code) {
        Diag d;
        d.loc = o.outcomes[i].loc;
        d.message = "duplicate outcome '" + o.outcomes[i].code + "' in '" + o.name + "'";
        diags->push_back(std::move(d));
        ok = false;
      }
    for (OutcomeDecl& oc : o.outcomes) {
      sort_unique_strings(&oc.doc_prov);
      sort_unique_strings(&oc.narrow.revs);
      sort_unique_strings(&oc.narrow.contexts);
      sort_unique_strings(&oc.narrow.prov);
      std::stable_sort(oc.narrow.captured.begin(), oc.narrow.captured.end(), [](const NarrowCaptured& a, const NarrowCaptured& b) { return a.param < b.param; });
    }
  }
  for (RspecDecl& r : p->rspecs) {
    sort_entry(&r);
    sort_unique_strings(&r.oracle_fns);
    for (AllowedEntry& a : r.allowed) {
      sort_unique_strings(&a.prov);
      sort_unique_strings(&a.codes);
    }
  }
  sort_unique_strings(&p->context.prov);
  sort_unique_strings(&p->context.not_app_prov);
  sort_unique_strings(&p->scope.prov);
  return ok;
}

Digest profile_hash(const Profile& p) {
  Sha256 h;
  h.update(std::string_view(kProfileHashDomain));
  h.update(canonical_text(p));
  return h.finish();
}

}  // namespace phi
}  // namespace kz
