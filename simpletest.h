#pragma once

//---------------------------------------------------------------------------------
// Config
//---------------------------------------------------------------------------------
#if !defined(MESSAGE_SPACE)
#define MESSAGE_SPACE 100 * 1024 // default 100k of message space is reserved per test
#endif
#if !defined(BASE_FIXTURE)
#define BASE_FIXTURE TestFixture // use TestFixture as the test base class by default
#endif

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
struct TypeToString
{
	TypeToString(int integer);
	TypeToString(__int64 integer);
	TypeToString(unsigned int integer);
	TypeToString(unsigned __int64 integer);
	TypeToString(double floatingPoint);
	TypeToString(bool boolean);
	TypeToString(char const* string);
	TypeToString(void const* pointer);

	char const* operator*() const { return myTextPointer; }
private:
	char myTextBuffer[64];
	char const* myTextPointer;
};

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
	void AddTest() const { ++myNumTestsChecked; }
	void LogError(char const* string, ...) const;

	// Stats from execution
	int NumTests() const { return myNumTestsChecked; }
	int NumErrors() const { return myNumErrors; }

	// Access to any errrors generated
	TestError const* GetFirstError() const { return (TestError*)myMessageSpace; }
	TestError const* GetLastError() const { return myNextError; }

	// Access to registered tests
	static TestFixture* GetFirstTest() { return ourFirstTest; }
	static TestFixture const* GetCurrentTest() { return ourCurrentTest; }
	TestFixture* GetNextTest() const { return myNextTest; }

	// Default execution implementation
	static bool ExecuteAllTests();
	static bool ExecuteTestGroup(const char* group);
	static bool ExecuteSingleTest(const char* group, const char* test);

protected:
	virtual void RunTest() const = 0;
	virtual void Setup() {}
	virtual void TearDown() {}
	
	// Test registration
	static TestFixture const* LinkTest(TestFixture* test);
	static TestFixture* ourFirstTest;

	TestFixture* myNextTest;

	mutable int myNumTestsChecked;
	mutable int myNumErrors;

	mutable TestError* myNextError;
	mutable char myMessageSpace[MESSAGE_SPACE];

	// allow access to current test outside of main code block
	static thread_local TestFixture const* ourCurrentTest;
};

//---------------------------------------------------------------------------------
// Test definition macros
//---------------------------------------------------------------------------------
#define DEFINE_TEST_FULL(name, group, fixture) \
struct name final : public fixture { \
	char const* TestName() const override { return #name; } \
	char const* TestGroup() const override { return #group; } \
	void RunTest() const override; \
} name ## Instance; \
void name::RunTest() const

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
#define TEST_ERROR_STRING_(cond, text) __FILE__ "(" STR(__LINE__) "): Condition [" cond "] Failed. " text
#define TEST_ERROR_(cond, text, ...) do { TestFixture::GetCurrentTest()->LogError(TEST_ERROR_STRING_(cond, text), __VA_ARGS__); } while(0)

//---------------------------------------------------------------------------------
// Tests
//---------------------------------------------------------------------------------
#define TEST_OPERATOR(a, b, op1, op2) TestFixture::GetCurrentTest()->AddTest(); if (!(a op1 b)) TEST_ERROR_(STR(a) " " STR(op1) " " STR(b), "%s " STR(op2) " %s", *TypeToString(a), *TypeToString(b))

#define TEST(cond) TEST_EQ(cond, true)

#define TEST_EQ(a, b) TEST_OPERATOR(a, b, ==, !=)
#define TEST_NEQ(a, b) TEST_OPERATOR(a, b, !=, ==)
#define TEST_GREATER(a, b) TEST_OPERATOR(a, b, >, <=)
#define TEST_GREATER_EQUAL(a, b) TEST_OPERATOR(a, b, >=, <)
#define TEST_LESS(a, b) TEST_OPERATOR(a, b, <, >=)
#define TEST_LESS_EQUAL(a, b) TEST_OPERATOR(a, b, <=, >)

#define TEST_CLOSE(a, b, eps) TestFixture::GetCurrentTest()->AddTest(); if (abs(a-b) > eps) TEST_ERROR_(STR(a) " Close to " STR(b), "Difference of %s is greater than " STR(eps), *TypeToString(abs(a-b)))
