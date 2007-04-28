#ifndef __MMAPEDFILE_TEST_H
#define __MMAPEDFILE_TEST_H

#include "mmapedfile.h"
#include "cxxtest/TestSuite.h"

#include <stdlib.h>
#include <unistd.h>

static const char* filename = "___test_mmapedfile";

/**
 * @todo better tests.
 * */
class MMapedFileTestSuit : public CxxTest::TestSuite {
public:
	void setUp()
	{
		std::string cmd = std::string("dd if=/dev/zero of=") +
				filename + " bs=1024 count=2 &> /dev/null";
		system(cmd.c_str());
	}

	void tearDown()
	{
		unlink(filename);
	}

	void test_ManagedFile()
	{
		ManagedFilePtr f(filename);
		TS_ASSERT_EQUALS(f.filesize(), 1024*2);
	}

	void test_MMapedFile()
	{
		MMapedFile m(filename);
		filebuf b = m.getBuf();

		TS_ASSERT_EQUALS(b.len(), 1024*2);
		TS_ASSERT_EQUALS(*b, 0x00);
	}
};


#endif // __MMAPEDFILE_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
