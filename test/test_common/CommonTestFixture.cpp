#include "CommonTestFixture.h"

#include "TestUtils.h"

void CommonTestFixture::SetUp() {
    TestTimeUtils::resetTime();
    TestMemoryUtils::resetMemoryTracking();
}

void CommonTestFixture::TearDown() {
    // Memory leak detection in this project is explicit: tests that allocate via TestMemoryUtils
    // must free via TestMemoryUtils::deallocate().
    ASSERT_FALSE(TestMemoryUtils::hasMemoryLeaks());
}
