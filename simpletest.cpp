#include "simpletest.h"
#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------------------
// statics
//---------------------------------------------------------------------------------
TestFixture* TestFixture::ourFirstTest;
thread_local TestFixture* TestFixture::ourCurrentTest;

//---------------------------------------------------------------------------------
// Standard type printers
//---------------------------------------------------------------------------------
TempString TypeToString(int value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "%d", value);
	return tempString;
}
TempString TypeToString(__int64 value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "%lld", value);
	return tempString;
}
TempString TypeToString(unsigned int value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "%u", value);
	return tempString;
}
TempString TypeToString(unsigned __int64 value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "%llu", value);
	return tempString;
}
TempString TypeToString(double value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "%f", value);
	return tempString;
}
TempString TypeToString(bool value)
{
	return TempString(value ? "true" : "false");
}
TempString TypeToString(char const* value)
{
	return TempString(value);
}
TempString TypeToString(void const* value)
{
	TempString tempString;
	sprintf_s(tempString.myTextBuffer, "0x%p", value);
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
	__crt_va_start_a(args, string);

	int printedChars = vsnprintf_s(myNextError->message, spaceLeft, spaceLeft-1, string, args);

	__crt_va_end(args);

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
		printf("Running [%s/%s]", test->TestGroup(), test->TestName());

	if (test->ExecuteTest())
	{
		if (output == TestFixture::Verbose)
			printf(": Passed %d out of %d tests\n", test->NumTests(), test->NumTests());
		return true;
	}

	if (output != TestFixture::Silent)
	{
		if (output != TestFixture::Verbose)
			printf("[%s/%s]", test->TestGroup(), test->TestName());

		printf(": Failed %d out of %d tests\n", test->NumErrors(), test->NumTests());

		for (TestError const* err = test->GetFirstError(), *e = test->GetLastError(); err != e; err = err->next)
		{
			printf("%s\n", err->message);
		}
	}

	return false;
}
bool TestFixture::ExecuteAllTests(OutputMode output)
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (!locExecuteTest(i, output))
			passed = false;
	}
	return passed;
}
bool TestFixture::ExecuteTestGroup(const char* group, OutputMode output)
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (_stricmp(group, i->TestGroup()) == 0)
		{
			if (!locExecuteTest(i, output))
				passed = false;
		}
	}
	return passed;
}
bool TestFixture::ExecuteSingleTest(const char* group, const char* test, OutputMode output)
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (_stricmp(group, i->TestGroup()) == 0 && _stricmp(test, i->TestName()) == 0)
		{
			if (!locExecuteTest(i, output))
				passed = false;
		}
	}
	return passed;
}
