// The function-pointer flow graph and the walk over a table in the initializer images.
#include <string>
#include <vector>

#include "finalize.hpp"
#include "support/kz_test.hpp"

using namespace kzs;

namespace {

ImageFact img(const std::string& object, const std::string& path, uint8_t kind, const std::string& text = "", int64_t value = 0) {
  ImageFact f;
  f.object = object;
  f.path = path;
  f.kind = kind;
  f.text = text;
  f.value = value;
  return f;
}

}  // namespace

int main() {
  // ---- flow graph -------------------------------------------------------------------------------
  {
    // F:a and F:b flow into a struct field; the field flows into a parameter; a cycle through a local
    const std::vector<FlowEdge> edges = {{"F:a", "T.f"}, {"F:b", "T.f"}, {"T.f", "P:g#0"}, {"P:g#0", "L:g/x"}, {"L:g/x", "P:g#0"}, {"F:c", "G:only_c"}};
    const FlowSolution s = solve_flow(edges);
    KZ_CHECK_EQ(s.funcs.at("T.f").size(), size_t(2));
    KZ_CHECK_EQ(s.funcs.at("P:g#0").size(), size_t(2));
    KZ_CHECK_EQ(s.funcs.at("L:g/x").size(), size_t(2));
    KZ_CHECK(s.funcs.at("L:g/x").count("a") == 1 && s.funcs.at("L:g/x").count("b") == 1);
    KZ_CHECK_EQ(s.funcs.at("G:only_c").size(), size_t(1));
    KZ_CHECK(s.funcs.at("G:only_c").count("c") == 1);
    KZ_CHECK(s.funcs.count("G:nothing") == 0);
    // nothing is unknown here
    for (const auto& kv : s.unknown) KZ_CHECK(!kv.second);
  }
  {
    // a source that cannot be followed marks every node it reaches as unknown, but keeps the functions
    const std::vector<FlowEdge> edges = {{"F:a", "P:g#0"}, {"?:loaded from a symbol table", "P:g#0"}, {"P:g#0", "L:g/p"}, {"F:b", "G:clean"}};
    const FlowSolution s = solve_flow(edges);
    KZ_CHECK(s.unknown.at("P:g#0"));
    KZ_CHECK(s.unknown.at("L:g/p"));
    KZ_CHECK(s.funcs.at("L:g/p").count("a") == 1);
    const auto clean = s.unknown.find("G:clean");
    KZ_CHECK(clean == s.unknown.end() || !clean->second);
  }

  // ---- table walk -------------------------------------------------------------------------------
  Model m;
  m.object_images["cfg"] = {img("cfg", ".list", KZ_IMAGE_KIND_ADDR, "list")};
  m.object_images["list"] = {img("list", "[0]", KZ_IMAGE_KIND_ADDR, "mod_a"), img("list", "[1]", KZ_IMAGE_KIND_ADDR, "mod_b"), img("list", "[2]", KZ_IMAGE_KIND_NULLPTR, "NULL")};
  m.object_images["mod_a"] = {img("mod_a", ".name", KZ_IMAGE_KIND_STRING, "a")};
  m.object_images["mod_b"] = {img("mod_b", ".name", KZ_IMAGE_KIND_STRING, "b")};
  Model::WalkSpec walk;
  walk.loop_key = "L";
  walk.root_object = "cfg";
  walk.fields = {"list"};
  std::vector<ImageFact> elements;
  int64_t trip = -1;
  std::string why;
  KZ_CHECK(resolve_table_walk(m, walk, &elements, &trip, &why));
  KZ_CHECK_EQ(trip, int64_t(2));
  // the elements: the field, the entries of the table up to the terminator, the images of the objects they point at
  KZ_CHECK(elements.size() >= 5);

  // a table that no null element ends
  m.object_images["list"] = {img("list", "[0]", KZ_IMAGE_KIND_ADDR, "mod_a")};
  KZ_CHECK(!resolve_table_walk(m, walk, &elements, &trip, &why));
  KZ_CHECK(why.find("no null element") != std::string::npos);

  // a zero-filled rest ends the table
  m.object_images["list"] = {img("list", "[0]", KZ_IMAGE_KIND_ADDR, "mod_a"), img("list", "[1..3]", KZ_IMAGE_KIND_ZERO_FILL, "zero")};
  KZ_CHECK(resolve_table_walk(m, walk, &elements, &trip, &why));
  KZ_CHECK_EQ(trip, int64_t(1));

  // the field is not the address of a table
  m.object_images["cfg"] = {img("cfg", ".list", KZ_IMAGE_KIND_INT, "", 7)};
  KZ_CHECK(!resolve_table_walk(m, walk, &elements, &trip, &why));
  KZ_CHECK(why.find("not the address of a table") != std::string::npos);

  // the object has no initializer in the closed program
  walk.root_object = "missing";
  KZ_CHECK(!resolve_table_walk(m, walk, &elements, &trip, &why));
  KZ_CHECK(why.find("not in the closed program") != std::string::npos);
  return kztest::finish("test_static_flow");
}
