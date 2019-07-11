#include "../test_helpers.hxx"

namespace
{
  enum colour { red, green, blue };
  enum class weather { hot, cold, wet };
}

namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(colour);
PQXX_DECLARE_ENUM_CONVERSION(weather);
}


namespace
{
void test_strconv_bool()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(false), "false", "Wrong to_string(false).");
  PQXX_CHECK_EQUAL(pqxx::to_string(true), "true", "Wrong to_string(true).");

  bool result;
  pqxx::from_string("false", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('false').");
  pqxx::from_string("FALSE", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('FALSE').");
  pqxx::from_string("f", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('f').");
  pqxx::from_string("F", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('F').");
  pqxx::from_string("0", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('0').");
  pqxx::from_string("true", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('true').");
  pqxx::from_string("TRUE", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('TRUE').");
  pqxx::from_string("t", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('t').");
  pqxx::from_string("T", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('T').");
  pqxx::from_string("1", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('1').");
}


void test_strconv_enum()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(red), "0", "Enum value did not convert.");
  PQXX_CHECK_EQUAL(pqxx::to_string(green), "1", "Enum value did not convert.");
  PQXX_CHECK_EQUAL(pqxx::to_string(blue), "2", "Enum value did not convert.");

  colour col;
  pqxx::from_string("2", col);
  PQXX_CHECK_EQUAL(col, blue, "Could not recover enum value from string.");
}


void test_strconv_class_enum()
{
  PQXX_CHECK_EQUAL(
	pqxx::to_string(weather::hot),
	"0",
	"Class enum value did not convert.");
  PQXX_CHECK_EQUAL(
	pqxx::to_string(weather::wet),
	"2",
	"Enum value did not convert.");

  weather w;
  pqxx::from_string("2", w);
  PQXX_CHECK_EQUAL(
	w,
	weather::wet,
	"Could not recover class enum value from string.");
}


PQXX_REGISTER_TEST(test_strconv_bool);
PQXX_REGISTER_TEST(test_strconv_enum);
PQXX_REGISTER_TEST(test_strconv_class_enum);
}
