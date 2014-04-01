#include "gtest/gtest.h"
int main(int argc, wchar_t* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
}

