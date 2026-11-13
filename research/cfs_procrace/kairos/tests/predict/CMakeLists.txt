# Tests of kz predict (lib/predict) on a small profile and model written in the test.  Every value is a unit-test
# value, not a research condition (CONDITIONS_POLICY).
include_directories("${CMAKE_SOURCE_DIR}/tests/unit")
kz_add_unit_test(test_predict_model SOURCES test_predict_model.cpp LIBS kz-predict kz-testsupport)
