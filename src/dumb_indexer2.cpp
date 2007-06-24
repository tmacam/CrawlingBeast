// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "indexer.h"
#include "zfilebuf.h"
#include "strmisc.h"
#include "indexerutils.hpp"
#include "zfilebuf.h"
#include "crawlerutils.hpp"
#include "isamutils.hpp"
#include "mkstore.hpp"

#include <fstream>
#include <sstream>


struct DumbIndexerVisitor {
	// Statistics
	docid_t d_count;
	uint64_t byte_count;
	uint64_t last_byte_count;
	time_t last_broadcast;
	time_t time_started;

	DumbIndexerVisitor()
	: d_count(0), byte_count(0), last_byte_count(0),
	  last_broadcast(time(NULL)), time_started(time(NULL))
	{}

	void operator()(uint32_t count, const store_hdr_entry_t* hdr,
			filebuf store_data)
	{
		uint32_t* docid = (uint32_t*) store_data.read(sizeof(uint32_t));
		uint32_t* len = (uint32_t*) store_data.read(sizeof(uint32_t));
		assert(*docid == hdr->docid);


		// read document ( no decompressing)
		filebuf f(store_data.readf(*len));

		// Statistics and prefetching
		++d_count;
		byte_count += f.len();
		if (d_count  % 1000 == 0) {
			print_stats();
		}
	}

	void print_stats()
	{
		time_t now = time(NULL);

		if (now == last_broadcast) return; // Avoid FPErr

		uint64_t byte_amount = byte_count - last_byte_count;
		std::cout << "# docs: " << d_count << " bytes: " <<
			byte_amount << " / " << byte_count << " bps: "<<
			byte_amount/(now - last_broadcast) << 
			" elapsed " << now - time_started << std::endl;

		last_broadcast = now;
		last_byte_count = byte_count;
	}
};



int main(int argc, char* argv[])
{
	const char* store_dir;

	if(argc < 2) {
		std::cerr << "wrong number of arguments" << std::endl;
		std::cerr << "dumb_indexer2 store_dir" << std::endl;
		exit(1);
	}

	store_dir = argv[1];

	DumbIndexerVisitor visitor;

	VisitIndexedStore<store_hdr_entry_t>(store_dir, "store", visitor);

	sleep(1);
	visitor.print_stats();

	exit(0);
}


//EOF
