//#pragma once

#ifndef _SIMPLETEST_H_
#define _SIMPLETEST_H_
	
//---------------------------------------------------------------------------------
// Config
//---------------------------------------------------------------------------------
#if !defined(MESSAGE_SPACE)
//#define MESSAGE_SPACE 10 * 1024 // default 10k of message space is reserved per test
#define MESSAGE_SPACE 100	// default 100 bytes of message space is reserved per test
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

	char const* myTextPointer;
	char myTextBuffer[STRING_LENGTH];
};

TempString TypeToString(int value);
TempString TypeToString(int64 value);
TempString TypeToString(uint value);
TempString TypeToString(uint64 value);
TempString TypeToString(float value);
TempString TypeToString(double value);
TempString TypeToString(bool value);
TempString TypeToString(char const* value);
TempString TypeToString(void const* value);

inline TempString TypeToString(char* value) { return TypeToString((char const*)value); }
inline TempString TypeToString(void* value) { return TypeToString((void const*)value); }

// if nothing specified then print some memory
template<typename T>
TempString TypeToString(T const&) { return TempString("(unknown type)"); }

template<typename T>
TempString TypeToString(T const* pointer) { return pointer == nullptr ? TempString("(nullptr)") : TypeToString(*pointer); }

template<typename T>
TempString TypeToString(T* pointer) { return pointer == nullptr ? TempString("(nullptr)") : TypeToString(*pointer); }

//---------------------------------------------------------------------------------
// Test fixture is the core of SimpleTest. It provides fixture behavior, access
// to registered tests and stores the results of a test run
// Everything is local here so tests can be multithreaded without any extra work
//---------------------------------------------------------------------------------
class TestFixture
{
public:
	TestFixture();
	//virtual ~TestFixture() {;}

	virtual bool ExecuteTest() {
														myNumTestsChecked = myNumErrors = 0;
														myNextError = (TestError*)myMessageSpace;

														TestFixture* lastCurrent = ourCurrentTest;
														ourCurrentTest = this;
														Setup();
														RunTest();
														TearDown();
														ourCurrentTest = lastCurrent;

														return myNumErrors == 0;}

	virtual char const* TestName() {return nullptr;}//const = 0;
	virtual char const* TestGroup() {return nullptr;}//const = 0;

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
	static void (*Print)(char const* string);
	static void Printf(char const* string, ...);

	static bool ExecuteAllTests(char const* groupFilter = nullptr, char const* nameFilter = nullptr, OutputMode output = Normal);
	static bool ExecuteAllTests(OutputMode output) { return ExecuteAllTests(nullptr, nullptr, output); }

	static bool ExecuteTestGroup(char const* groupFilter, OutputMode output = Normal) { return ExecuteAllTests(groupFilter, nullptr, output); }

protected:
	virtual void RunTest() {}//= 0;
	virtual void Setup() {}
	virtual void TearDown() {}
	
	// Test registration
	static TestFixture const* LinkTest(TestFixture* test);
	static TestFixture* ourFirstTest;
	static TestFixture* ourLastTest;

	TestFixture* myNextTest;

	int myNumTestsChecked;
	int myNumErrors;

	TestError* myNextError;
	char myMessageSpace[MESSAGE_SPACE];

	// allow access to current test outside of main code block
	//static thread_local TestFixture* ourCurrentTest;
		static TestFixture* ourCurrentTest;
};

//---------------------------------------------------------------------------------
// Test definition macros
//---------------------------------------------------------------------------------
#define DEFINE_TEST_FULL(name, group, fixture) \
struct TOK(group, name) final : public fixture { \
	char const* TestName() /*const*/ override { return #name; } \
	char const* TestGroup() /*const*/ override { return #group; } \
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
T TestDifference(T const& a, T const& b) { T tmp = a - b; return tmp < 0 ? -tmp : tmp; }

// why are these still needed?
#define STR2(x) #x
#define STR(x) STR2(x)

#define TOK2(a, b) a ## b
#define TOK(a, b) TOK2(a, b)

//---------------------------------------------------------------------------------
// Error reporting don't call directly
//---------------------------------------------------------------------------------
#define TEST_ERROR_PREFIX_ __FILE__ "(" STR(__LINE__) "): Condition [%s] Failed. "
#define TEST_ERROR_(message, ...) do { TestFixture::GetCurrentTest()->LogError(TEST_ERROR_PREFIX_ message, ##__VA_ARGS__); } while(0)
#define TEST_CHECK_(cond, condtext, message, ...) do { TestFixture::GetCurrentTest()->AddTest(); if (!(cond)) TEST_ERROR_(message, condtext, ##__VA_ARGS__); } while(0)

//---------------------------------------------------------------------------------
// Tests
//---------------------------------------------------------------------------------
#define TEST_OPERATOR(a, b, op1, op2) TEST_CHECK_((a) op1 (b), STR(a) " " STR(op1) " " STR(b), "%s " STR(op2) " %s", *TypeToString(a), *TypeToString(b))

#define TEST(cond) TEST_EQ(cond, true)

#define TEST_EQ(a, b) TEST_OPERATOR(a, b, ==, !=)
#define TEST_NEQ(a, b) TEST_OPERATOR(a, b, !=, ==)
#define TEST_GREATER(a, b) TEST_OPERATOR(a, b, >, <=)
#define TEST_GREATER_EQUAL(a, b) TEST_OPERATOR(a, b, >=, <)
#define TEST_LESS(a, b) TEST_OPERATOR(a, b, <, >=)
#define TEST_LESS_EQUAL(a, b) TEST_OPERATOR(a, b, <=, >)

#define TEST_CLOSE(a, b, eps) TEST_CHECK_(TestDifference(a,b) <= eps, STR(a) " Close to " STR(b), "Difference of %s is greater than " STR(eps), *TypeToString(TestDifference(a,b)))
#define TEST_MESSAGE(cond, message, ...) TEST_CHECK_(cond, STR(cond), message, ##__VA_ARGS__)

#endif // _SIMPLETEST_H_