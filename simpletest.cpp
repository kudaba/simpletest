#include "simpletest.h"
#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------------------
// statics
//---------------------------------------------------------------------------------
TestFixture* TestFixture::ourFirstTest;
thread_local TestFixture const* TestFixture::ourCurrentTest;

//---------------------------------------------------------------------------------
// Standard type printers
//---------------------------------------------------------------------------------
TypeToString::TypeToString(int integer)
{
	sprintf_s(myTextBuffer, "%d", integer);
	myTextPointer = myTextBuffer;
}
TypeToString::TypeToString(__int64 integer)
{
	sprintf_s(myTextBuffer, "%lld", integer);
	myTextPointer = myTextBuffer;
}
TypeToString::TypeToString(unsigned int integer)
{
	sprintf_s(myTextBuffer, "%u", integer);
	myTextPointer = myTextBuffer;
}
TypeToString::TypeToString(unsigned __int64 integer)
{
	sprintf_s(myTextBuffer, "%llu", integer);
	myTextPointer = myTextBuffer;
}
TypeToString::TypeToString(double floatingPoint)
{
	sprintf_s(myTextBuffer, "%f", floatingPoint);
	myTextPointer = myTextBuffer;
}
TypeToString::TypeToString(bool boolean)
{
	myTextPointer = boolean ? "TRUE" : "FALSE";
}
TypeToString::TypeToString(char const* string)
{
	myTextPointer = string;
}
TypeToString::TypeToString(void const* pointer)
{
	sprintf_s(myTextBuffer, "0x%p", pointer);
	myTextPointer = myTextBuffer;
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

	Setup();

	ourCurrentTest = this;
	RunTest();
	ourCurrentTest = nullptr;

	TearDown();

	return myNumErrors == 0;
}
//---------------------------------------------------------------------------------
// Write error into current error object and advance pointer if there's still enough space
//---------------------------------------------------------------------------------
void TestFixture::LogError(char const* string, ...) const
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

	int printedChars = vsprintf_s(myNextError->message, spaceLeft, string, args);

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
static bool locExecuteTest(TestFixture* test)
{
	printf("[%s]", test->TestName());
	if (test->ExecuteTest())
	{
		printf(": Passed %d out of %d tests\n", test->NumTests(), test->NumTests());
		return true;
	}

	printf(": Failed %d out of %d tests\n", test->NumErrors(), test->NumTests());
	for (TestError const* err = test->GetFirstError(), *e = test->GetLastError(); err != e; err = err->next)
	{
		printf("%s\n", err->message);
	}

	return false;
}
bool TestFixture::ExecuteAllTests()
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (!locExecuteTest(i))
			passed = false;
	}
	return passed;
}
bool TestFixture::ExecuteTestGroup(const char* group)
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (_stricmp(group, i->TestGroup()) == 0)
		{
			if (!locExecuteTest(i))
				passed = false;
		}
	}
	return passed;
}
bool TestFixture::ExecuteSingleTest(const char* group, const char* test)
{
	bool passed = true;
	for (auto i = TestFixture::GetFirstTest(); i; i = i->GetNextTest())
	{
		if (_stricmp(group, i->TestGroup()) == 0 && _stricmp(test, i->TestName()) == 0)
		{
			if (!locExecuteTest(i))
				passed = false;
		}
	}
	return passed;
}
