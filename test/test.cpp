/**@author hoxnox <hoxnox@gmail.com>
 * @date 20160531 16:13:29
 *
 * @brief procman test launcher.*/

// Google Testing Framework
#include <gtest/gtest.h>

// test cases
#include "test_proc_builder.hpp"

int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}


