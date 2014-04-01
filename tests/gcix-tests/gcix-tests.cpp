#include "gtest/gtest.h"
#include "GlobalAllocator.h"

int main(int argc, wchar_t* argv[])
{
	::testing::InitGoogleTest(&argc, argv);

	// Initialize GlobalAllocator for all tests
	gcix::GlobalAllocator::Initialize();

	int result = RUN_ALL_TESTS();

	return result;
}

