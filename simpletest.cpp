#include "simpletest.h"
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <stdarg.h>

//---------------------------------------------------------------------------------
// statics
//---------------------------------------------------------------------------------
void DefaultPrint(char const* string) { printf("%s", string); }

TestFixture* TestFixture::ourFirstTest;
thread_local TestFixture* TestFixture::ourCurrentTest;
void (*TestFixture::Print)(char const* string) = DefaultPrint;

//---------------------------------------------------------------------------------
// helper to get the number of decimal places to print for floats and doubles
//---------------------------------------------------------------------------------
template <typename T>
static int locDecimals(T value)
{
	const T eps = 0.00001f;
	T remainder = value - (int)value;
	if (remainder == 0)
		return 0;

	int decimals = 0;

	// add decimals until hitting the first non-zero number that shouldn't be rounded (close to 0 or 1)
	bool hitsomething = int(remainder * 10) != 0;
	while (!hitsomething ||
		((remainder > eps && remainder < (1 - eps)) ||
		(remainder < -eps && remainder >(-1 + eps))))
	{
		remainder = remainder * 10;
		remainder = remainder - (int)remainder;
		hitsomething |= int(remainder * 10) != 0;
		++decimals;
	}
	return decimals;
}

//---------------------------------------------------------------------------------
// Standard type printers
//---------------------------------------------------------------------------------
TempString::TempString(const TempString& other)
{
	if (other.myTextPointer == other.myTextBuffer)
	{
		strcpy(myTextBuffer, other.myTextBuffer);
		myTextPointer = myTextBuffer;
	}
	else
	{
		myTextPointer = other.myTextPointer;
	}
}
TempString TypeToString(int value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%d", value);
	return tempString;
}
TempString TypeToString(int64 value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%lld", value);
	return tempString;
}
TempString TypeToString(uint value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%u", value);
	return tempString;
}
TempString TypeToString(uint64 value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%llu", value);
	return tempString;
}
TempString TypeToString(float value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%0.*f", locDecimals(value), value);
	return tempString;
}
TempString TypeToString(double value)
{
	TempString tempString;
	sprintf(tempString.myTextBuffer, "%0.*f", locDecimals(value), value);
	return tempString;
}
TempString TypeToString(bool value)
{
	return TempString(value ? "true" : "false");
}
TempString TypeToString(char const* value)
{
	return TempString(value ? value : "(nullptr)");
}
TempString TypeToString(void const* value)
{
	if (value == nullptr)
		return TempString("(nullptr)");
	TempString tempString;
	sprintf(tempString.myTextBuffer, "0x%p", value);
	return tempString;
}

//---------------------------------------------------------------------------------
// Fixture implementation
//---------------------------------------------------------------------------------
TestFixture::TestFixture()
{
	// global link list registration
	myNextTest = ourFirstTest;
	ourFirstTest = this;
}
//---------------------------------------------------------------------------------
bool TestFixture::ExecuteTest()
{
	myNumTestsChecked = myNumErrors = 0;
	myNextError = (TestError*)myMessageSpace;

	TestFixture* lastCurrent = ourCurrentTest;
	ourCurrentTest = this;
	Setup();
	RunTest();
	TearDown();
	ourCurrentTest = lastCurrent;

	return myNumErrors == 0;
}
//---------------------------------------------------------------------------------
// Write error into current error object and advance pointer if there's still enough space
//---------------------------------------------------------------------------------
void TestFixture::LogError(char const* string, ...)
{
	++myNumErrors;

	uintptr_t spaceLeft = (myMessageSpace + MESSAGE_SPACE) - (char*)myNextError;

	if (spaceLeft == 0)
	{
		return;
	}

	spaceLeft -= sizeof(TestError);

	va_list args;

	va_start(args, string);

	int printedChars = vsnprintf(myNextError->message, spaceLeft, string, args);

	va_end(args);

	// if there isn't a reasonable amount of space left then just advance to end and stop printing errors
	if (printedChars < (int)(spaceLeft - sizeof(TestError) - 64))
	{
		uintptr_t nextOffset = (uintptr_t(myNextError->message) + printedChars + sizeof(TestError) * 2 - 1);
		nextOffset -= nextOffset % alignof(TestError);
		TestError* next = (TestError*)(nextOffset);
		myNextError->next = next;
	}
	else
	{
		myNextError->next = (TestError*)(myMessageSpace + MESSAGE_SPACE);
	}

	myNextError = myNextError->next;
}
TestFixture const* TestFixture::LinkTest(TestFixture* test)
{
	test->myNextTest = TestFixture::ourFirstTest;
	TestFixture::ourFirstTest = test;
	return test;
}

//---------------------------------------------------------------------------------
// Standard / Example runners
//---------------------------------------------------------------------------------
static bool locExecuteTest(TestFixture* test, TestFixture::OutputMode output)
{
	if (output == TestFixture::Verbose)
		TestFixture::Printf("Running [%s/%s]", test->TestGroup(), test->TestName());

	if (test->ExecuteTest())
	{
		if (output == TestFixture::Verbose)
			TestFixture::Printf(": Passed %d out of %d tests\n", test->NumTests(), test->NumTests());
		return true;
	}

	if (output != TestFixture::Silent)
	{
		if (output != TestFixture::Verbose)
			TestFixture::Printf("[%s/%s]", test->TestGroup(), test->TestName());

		TestFixture::Printf(": Failed %d out of %d tests\n", test->NumErrors(), test->NumTests());

		for (TestError const* err = test->GetFirstError(), *e = test->GetLastError(); err != e; err = err->next)
		{
			TestFixture::Printf("%s\n", err->message);
		}
	}

	return false;
}
void TestFixture::Printf(char const* string, ...)
{
	char tempSpace[4096];
	va_list args;

	va_start(args, string);

	vsnprintf(tempSpace, sizeof(tempSpace), string, args);

	va_end(args);

	TestFixture::Print(tempSpace);
}
bool TestFixture::ExecuteAllTests(char const* groupFilter, char const* nameFilter, OutputMode output)
{
	if (output != Silent)
	{
		if (groupFilter == nullptr && nameFilter == nullptr)
			Printf("Running all tests.\n");
		else if (groupFilter != nullptr && nameFilter == nullptr)
			Printf("Running all tests in groups [%s].\n", groupFilter);
		else if (groupFilter == nullptr && nameFilter != nullptr)
			Printf("Running all tests named [%s].\n", nameFilter);
		else
			Printf("Running all tests named [%s/%s].\n", groupFilter, nameFilter);
	}

	int count = 0;
	int passes = 0;
	int fails = 0;
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		bool matchGroup = groupFilter == nullptr || strcmp(groupFilter, i->TestGroup()) == 0;
		bool matchName = nameFilter == nullptr || strcmp(nameFilter, i->TestName()) == 0;
		if (matchGroup && matchName)
		{
			++count;
			passed &= locExecuteTest(i, output);
			passes += i->NumTests();
			fails += i->NumErrors();
		}
	}

	if (output != Silent)
	{
		if (count == 0)
			Printf("Failed to find any tests.\n");
		else if (passed)
			Printf("%d Tests finished. All %d assertions are passing.\n", count, passes);
		else
			Printf("%d Tests finished, %d of %d assertions failed. Some tests are reporting errors.\n", count, fails, passes);
	}
	return passed;
}
