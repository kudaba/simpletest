#pragma once

//---------------------------------------------------------------------------------
// Config
//---------------------------------------------------------------------------------
#if !defined(MESSAGE_SPACE)
#define MESSAGE_SPACE 10 * 1024 // default 10k of message space is reserved per test
#endif
#if !defined(STRING_LENGTH)
#define STRING_LENGTH 64 // size of temp strings for converting types
#endif
#if !defined(BASE_FIXTURE)
#define BASE_FIXTURE TestFixture // use TestFixture as the test base class by default
#endif

typedef long long int64;
typedef unsigned int uint;
typedef unsigned long long uint64;

//---------------------------------------------------------------------------------
// Link list of errors build into MESSAGE_SPACE
//---------------------------------------------------------------------------------
struct TestError
{
	TestError* next;
	char message[1];
};

//---------------------------------------------------------------------------------
// simple converter of basic types to text
// TODO: Use a global template function for conversion so users can override this
//---------------------------------------------------------------------------------
struct TempString
{
	TempString() : myTextPointer(myTextBuffer) {}
	TempString(const TempString& other);
	TempString(char const* string) : myTextPointer(string) {}

	char const* operator*() const { return myTextPointer; }

	char myTextBuffer[STRING_LENGTH];
	char const* myTextPointer;
};

TempString TypeToString(int value);
TempString TypeToString(int64 value);
TempString TypeToString(uint value);
TempString TypeToString(uint64 value);
TempString TypeToString(double value);
TempString TypeToString(bool value);
TempString TypeToString(char const* value);

//---------------------------------------------------------------------------------
// Test fixture is the core of SimpleTest. It provides fixture behavior, access
// to registered tests and stores the results of a test run
// Everything is local here so tests can be multithreaded without any extra work
//---------------------------------------------------------------------------------
class TestFixture
{
public:
	TestFixture();
	virtual ~TestFixture() {};

	virtual bool ExecuteTest();

	virtual char const* TestName() const = 0;
	virtual char const* TestGroup() const = 0;

	// Reporting used during testing process
	void AddTest() { ++myNumTestsChecked; }
	void LogError(char const* string, ...);

	// Stats from execution
	int NumTests() const { return myNumTestsChecked; }
	int NumErrors() const { return myNumErrors; }

	// Access to any errrors generated
	TestError const* GetFirstError() const { return (TestError*)myMessageSpace; }
	TestError const* GetLastError() const { return myNextError; }

	// Access to registered tests
	static TestFixture* GetFirstTest() { return ourFirstTest; }
	static TestFixture* GetCurrentTest() { return ourCurrentTest; }
	TestFixture* GetNextTest() const { return myNextTest; }

	enum OutputMode
	{
		Silent,
		Normal,
		Verbose
	};

	// Default execution implementation
	static bool ExecuteAllTests(OutputMode output = Normal);
	static bool ExecuteTestGroup(const char* group, OutputMode output = Normal);
	static bool ExecuteSingleTest(const char* group, const char* test, OutputMode output = Normal);

protected:
	virtual void RunTest() = 0;
	virtual void Setup() {}
	virtual void TearDown() {}
	
	// Test registration
	static TestFixture const* LinkTest(TestFixture* test);
	static TestFixture* ourFirstTest;

	TestFixture* myNextTest;

	int myNumTestsChecked;
	int myNumErrors;

	TestError* myNextError;
	char myMessageSpace[MESSAGE_SPACE];

	// allow access to current test outside of main code block
	static thread_local TestFixture* ourCurrentTest;
};

//---------------------------------------------------------------------------------
// Test definition macros
//---------------------------------------------------------------------------------
#define DEFINE_TEST_FULL(name, group, fixture) \
struct TOK(group, name) final : public fixture { \
	char const* TestName() const override { return #name; } \
	char const* TestGroup() const override { return #group; } \
	void RunTest() override; \
} TOK(TOK(group, name), Instance); \
void TOK(group, name)::RunTest()

#define DEFINE_TEST(name) DEFINE_TEST_FULL(name, Global, BASE_FIXTURE)
#define DEFINE_TEST_G(name, group) DEFINE_TEST_FULL(name, group, BASE_FIXTURE)
#define DEFINE_TEST_F(name, fixture) DEFINE_TEST_FULL(name, Global, fixture)
#define DEFINE_TEST_GF(name, group, fixture) DEFINE_TEST_FULL(name, group, fixture)

//---------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------
template <typename T>
T abs(T t) { return t < 0 ? -t : t; }

// why are these still needed?
#define STR2(x) #x
#define STR(x) STR2(x)

#define TOK2(a, b) a ## b
#define TOK(a, b) TOK2(a, b)

//---------------------------------------------------------------------------------
// Error reporting
//---------------------------------------------------------------------------------
#define TEST_ERROR_STRING_(cond) __FILE__ "(" STR(__LINE__) "): Condition [" cond "] Failed. "
#define TEST_ERROR_(cond, ...) do { TestFixture::GetCurrentTest()->LogError(TEST_ERROR_STRING_(cond) __VA_ARGS__); } while(0)
#define TEST_CHECK_(cond, condtext, ...) TestFixture::GetCurrentTest()->AddTest(); if (!(cond)) TEST_ERROR_(condtext, __VA_ARGS__)

//---------------------------------------------------------------------------------
// Tests
//---------------------------------------------------------------------------------
#define TEST_OPERATOR(a, b, op1, op2) TEST_CHECK_(a op1 b, STR(a) " " STR(op1) " " STR(b), "%s " STR(op2) " %s", *TypeToString(a), *TypeToString(b))

#define TEST(cond) TEST_EQ(cond, true)

#define TEST_EQ(a, b) TEST_OPERATOR(a, b, ==, !=)
#define TEST_NEQ(a, b) TEST_OPERATOR(a, b, !=, ==)
#define TEST_GREATER(a, b) TEST_OPERATOR(a, b, >, <=)
#define TEST_GREATER_EQUAL(a, b) TEST_OPERATOR(a, b, >=, <)
#define TEST_LESS(a, b) TEST_OPERATOR(a, b, <, >=)
#define TEST_LESS_EQUAL(a, b) TEST_OPERATOR(a, b, <=, >)

#define TEST_CLOSE(a, b, eps) TEST_CHECK_(abs(a-b) <= eps, STR(a) " Close to " STR(b), "Difference of %s is greater than " STR(eps), *TypeToString(abs(a-b)))
#define TEST_MESSAGE(cond, ...) TEST_CHECK_(cond, STR(cond), __VA_ARGS__)
