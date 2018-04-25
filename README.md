# simpletest

A lot of c++ unit tests claim to be simple, but when I went searching for the perfect one there was always something that prevented me from using it. They were either overly complicated or had some critical flaw like excess memory allocations or dependencies on external programs. So here is the simplest form of unit test I could come up with to cover basic development. My rules for simple were the following:
* Basic test features only: fixtures and test
* Simple, isolated test declaration
* No memory allocations, at all
* Very few, if any dependencies
* Bonus: Threadable

# Basic test features
Every test code seems to use slight variants of terminology so heres mine.
* **Test** as a noun is the logical unit that is designed to satisfy that a piece of code meets requirements.
* **Test** as a verb is an atomic operation checking the state of and object, variables or return values.

You define atomic tests inside logical tests

```c++
DEFINE_TEST(Addition)
{
    TEST(1 + 1 == 2);
}
```

So far, in my experience, **Fixtures** and **Groups** are the most useful built in features of unit tests. Mocks are great as a concept, but are generally better left up to the user to figure out.

**Fixtures** in simpletest function as the base class of tests and offer the ability to add Setup and Teardown code to tests to initialize preconditions and to clean up.

Setup a fixture
```c++
class MyFixture : TestFixture
{
public:
    void Setup() override { myInts.Add(1); }
    void TearDown() override { myInts.Reset(); }

    List<int> myInts;
}
```

Use the _F variant of DEFINE_TEST to use fixture
```c++
DEFINE_TEST_F(ListAdd, MyFixture)
{
    myInts.Add(1);
    TEST(myInts.Count() == 2);
}
```

**Groups**
Groups exist for organizational purposes. They can be used to organize results or to run subsets of all tests. You use _G version of DEFINE_TEST to assign a group.

```c++
DEFINE_TEST_G(ListAdd, ListTests)
{
    ...
}
```

**Groups and Fixtures**
There also exists a _GF version of DEFINE_TEST to specify both group and fixture.

```c++
DEFINE_TEST_GF(ListAdd, ListTests, MyFixture)
{
    ...
}
```
# Simple declaration
In addition the the 4 variants of test declarations and fixtures, there's a small set of TEST macros that help to make error messages more clear. I hope their self explanatory enough
* TEST(condition) - simple version that will print error if false
* TEST_<operator>(a, b) - Operator comparison from a to b
  * Variants include _EQ, _NEQ, _GREATER, _LESS, _GREATER_EQ, _LESS_EQ
* TEST_CLOSE(a, b, eps) - Test that two values are within eps(ilon) of each other

## Custom type printers
**TODO** Actually implement this feature. Until then the operator macros will only work on basic integer, float, const char* and void* types.

# No memory allocations
One of the main reason I want a unit test is to make sure my code doesn't leak. However many implementation use std::string and many other dynamic allocations making it impossible to test my code. Simple test is carefully crafted to only use statically allocated memory. Because of this it needs to know (read guess) memory requirements ahead of time. See configuration section for some options to control memory usage.

# Very few dependencies
After seeing unit tests that need perl or python to generate test harnesses, or other crazy code dependencies, I wanted to use the most limited set of dependencies I could. I didn't go as far as a single header implementation, but even the cpp only depends on two standard headers, stdio.h and string.h.

# Threadable
By keeping the fixture, test and results in a single object it means that the execution of a single test is threadable as long that the test code itself is contained and threadable. There is no default threaded implementation of test execution, but you can see the simpletest_test project for more advanced examples.

# Extra configuration

Global configuration is generally done by defininy macros before including simpletest.h

## Default Fixture
If you want all tests to use the same fixture, for default memory checking or exception catching, then simply define the macro BASE_FIXTURE.

## Static memory usage
To achieve allocation free tests I needed to give each test a memory area to write their error messages. The default is (probably out of date) 100k. You can override this by defining the MESSAGE_SPACE macro.