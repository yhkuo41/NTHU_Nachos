#include "syscall.h"

int pass = 1;

void fail(char *msg)
{
	MSG(msg);
	pass = 0;
}

// modify filename to fileXX.test without using std
char *filename_i(char *filename, int i)
{
	*(filename + 4) = i / 10 + '0';
	*(filename + 5) = i % 10 + '0';
	return filename;
}

void test_create(char *filename, int i)
{
	for (i = 0; i <= 20; ++i)
	{
		if (Create(filename_i(filename, i)) != 1)
		{
			fail("Failed on creating file");
		}
	}
}

void test_open(char *filename, int i, OpenFileId *fids)
{
	for (i = 0; i < 20; ++i)
	{
		fids[i] = Open(filename_i(filename, i));
		if (fids[i] < 0)
		{
			fail("Failed on opening file");
		}
	}
	if (Open(filename) != -1)
	{
		fail("Should fail on opening the 21st file, but it doesn't work as expected");
	}
}

void test_write(int i, int j, char *write_buf, OpenFileId *fids)
{
	for (i = 0; i < 20; ++i)
	{
		for (j = 0; j < 26; ++j)
		{
			if (Write(write_buf + j, 1, fids[i]) != 1)
			{
				fail("Failed on writing file");
			}
		}
	}
	if (Write(write_buf, 1, 21) != -1)
	{
		fail("Should fail on writing illegal fid, but it doesn't work as expected");
	}
	if (Write(write_buf, 0, fids[0]) != 0)
	{
		fail("Should be albe to write 0 byte, but it doesn't work as expected");
	}
}

void test_close(int i, OpenFileId *fids)
{
	for (i = 0; i < 20; ++i)
	{
		if (Close(fids[i]) != 1)
		{
			fail("Failed on closing file");
		}
	}
	if (Close(21) != -1)
	{
		fail("Should fail on closing illegal fid, but it doesn't work as expected");
	}
}

void test_read(int i, int j, char *write_buf, char *read_buf, OpenFileId *fids)
{
	for (i = 0; i < 20; ++i)
	{
		if (Read(read_buf, 26, fids[i]) != 26)
		{
			fail("Failed on reading file");
		}
		for (j = 0; j < 26; ++j)
		{
			if (write_buf[j] != read_buf[j])
			{
				fail("Failed: reading wrong result");
			}
		}
	}
	if (Read(read_buf, 1, 21) != -1)
	{
		fail("Should fail on reading illegal fid, but it doesn't work as expected");
	}
	if (Read(read_buf, 0, fids[0]) != 0)
	{
		fail("Should be albe to read 0 byte, but it doesn't work as expected");
	}
}

int main(void)
{
	// The string can't be too long, otherwise compiler may use memcpy here and cause compile error
	char write_buf[27] = "abcdefghijklmnopqrstuvwxyz";
	char read_buf[27];
	char *filename = "file00.test";
	OpenFileId fids[20];
	// Must declare i and j first, otherwise declaring them at the beginning of the for loop is not allowed before C99
	int i, j;
	test_create(filename, i);
	test_open(filename, i, fids);
	test_write(i, j, write_buf, fids);
	test_close(i, fids);
	/* Direct reading will cause an error because the currentOffset of the same OpenFile has been changed after writing,
	   read the same OpenFile again will start from the end of the file and fail. Currently, we don't have a syscall
	   that can reset the currentOffset of the same OpenFile. If we want to read those files from start, we need to close
	   and reopen them
	*/
	test_open(filename, i, fids);
	test_read(i, j, write_buf, read_buf, fids);
	test_close(i, fids);
	if (pass)
	{
		MSG("Passed! ^_^");
	}
	else
	{
		MSG("Failed! :(");
	}
	Halt();
}