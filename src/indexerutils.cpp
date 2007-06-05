// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "indexerutils.hpp"

#include "htmliterators.hpp"
#include "zfilebuf.h"
#include "mmapedfile.h"

#include <fstream>
#include <iomanip>

/***********************************************************************
				 RUN ITERATORS
 ***********************************************************************/

void run_inserter::flush()
{

	// We only run if there are pending triples
	if (cur_triple == run_buf){
		return;
	}

	// sort triples
	std::sort(run_buf,cur_triple);

	// Ok. There are pending triples.
	unsigned int n_triples_pending = cur_triple - run_buf;

	// Let's write 'em to disk
	// FIXME woudn't UNIX I/O rotines perform better?
	std::ofstream out;
	// Turn on exception reporting for file operations
	out.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	out.open(make_run_filename(path_prefix, n_runs).c_str(),
		 std::ios::binary | std::ios::out);
	out.rdbuf()->pubsetbuf(0, 0); // unbuffering out
	out.write( (char *)run_buf,
		   n_triples_pending * sizeof(run_triple));

	// Update the number of runs written
	++n_runs;
	//reset reading/writing position
	cur_triple = run_buf;
}


/***********************************************************************
		     VOCABULARY RETRIEVAL AND NORMALIZATION
 ***********************************************************************/

void getWordFrequency(filebuf f, StrIntMap& wfreq,
			const WideCharConverter& wconv, docid_t docid)
{
	WIsalpha isalpha;

	HTMLContentIterator ci(f), ce;
	std::wstring word;

	// Clear word frequency
	wfreq.clear();


	// For each text node (text inside and between tags) content
	for(; ci != ce; ++ci){
		try{
			std::wstring text_node = wconv.mbs_to_wcs(*ci);
			std::wstring::const_iterator i(text_node.begin()),
					      end(text_node.end());
			// Get all words from this text node 
			while(i != text_node.end()){
				// new word
				word.clear();
				// Find word start
				i = std::find_if(i,end,isalpha);
				// Go until word end
				while(i != end && isalpha(*i)){
					word += *i;
					++i;
				}
				// Grabbed a full word. 
				if(!word.empty()) {
					normalize_term(word);
					std::string w(wconv.wcs_to_mbs(word));
					wfreq[w] = wfreq[w] + 1;
				}
			}
		} catch (WideCharConverter::ConversionError& conv){
			std::cerr << " # ERR " << docid << " " <<
				conv.what() << std::endl;
		}

	}
}

/***********************************************************************
			       INDEXING FUNCTIONS
 ***********************************************************************/

std::string make_filename(std::string store_dir, docid_t docid)
{
	std::ostringstream id_hex;

	id_hex << std::uppercase << std::hex << std::setw(8) <<
                std::setfill('0') << docid;
	std::string id_hex_str = id_hex.str();

	std::string id_path =  store_dir + "/" + 
				id_hex_str.substr(0,2) + "/" +
				id_hex_str.substr(2,2) + "/" +
				id_hex_str.substr(4,2) + "/" +
				id_hex_str.substr(6,2) + "/data.gz"; // FIXME data.gz constant
	return id_path;
}

void mapIdToTerms(const StrIntMap& term2id, IntStrMap& id2term)
{
	StrIntMap::const_iterator v;

	for(v = term2id.begin(); v != term2id.end(); ++v){
		const std::string& term = v->first;
		const int& id = v->second;

		id2term[id] = term;
	}
}

void dump_vocabulary(const StrIntMap& vocabulary, const char* output_dir)
{
	// Setup dump files' filename
	std::string voc_prefix(output_dir);
	voc_prefix += "/vocabulary";
	std::string voc_hdr_filename =  voc_prefix + ".hdr";
	std::string voc_data_filename =  voc_prefix + ".data";

	// Create, turn exception reporting on and open the files
	std::ofstream header;
	std::ofstream data;
	header.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	data.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	header.open(voc_hdr_filename.c_str(),std::ios::out | std::ios::binary);
	data.open(voc_data_filename.c_str(), std::ios::out | std::ios::binary);

	IntStrMap id2term;
	IntStrMap::const_iterator i2ti;
	mapIdToTerms(vocabulary, id2term);

	int expected_id = 0;
	uint32_t pos;

	/* id2term is a (ordered) map. So we assume we will sequentially
	 * iterate from full [0,max_term_id] range.
	 *
	 * If this is found to be incorrect.. well, the assert will tell us.
	 */

	for(i2ti = id2term.begin(); i2ti != id2term.end(); ++i2ti){
		const int& tid = i2ti->first;
		const std::string& term = i2ti->second;

		pos = data.tellp(); // this term start position in .data
		data.write(term.c_str(), term.size() +1); // account for \0
		header.write((char*)&pos, sizeof(pos));

		//FIXME this is here just for DEBUGing purposes
		assert(tid == expected_id);
		++expected_id;

	}
}

void load_vocabulary(StrIntMap& vocabulary, const char* store_dir)
{
	// Setup dump files' filename
	std::string voc_prefix(store_dir);
	voc_prefix += "/vocabulary";
	std::string voc_hdr_filename =  voc_prefix + ".hdr";
	std::string voc_data_filename =  voc_prefix + ".data";

	MMapedFile hdr(voc_hdr_filename);
	uint32_t* h_start = (uint32_t*) hdr.getBuf().start;
	uint32_t* h_end = (uint32_t*) hdr.getBuf().end;
	size_t wcount = h_end - h_start;

	MMapedFile data(voc_data_filename);
	const char* word = data.getBuf().start;
	
	for(size_t i = 0; i < wcount; ++i) {
		const char* word_start = &word[h_start[i]];
		vocabulary[std::string(word_start)] = i;
	}


}


void index_files(const char* store_dir, const std::vector<docid_t> docids_list,
		const char* output_dir)
{
	docid_t docid;

	// Statistics
	docid_t d_count = 0;
	uint64_t byte_count = 0;
	uint64_t last_byte_count = 0;
	time_t last_broadcast = time(NULL);
	time_t time_started = time(NULL);

	const unsigned int KB = 1<<10;
	run_inserter runs(output_dir, 100*KB );

	StrIntMap vocabulary; // term -> term_id
	StrIntMap wfreq; // term -> frequency in current doc
	WideCharConverter wcconv;

	StrIntMap::const_iterator w; // intra-document word iterator

	// For every docid / retrieved document
	for(unsigned int i = 0; i < docids_list.size(); ++i){
		docid = docids_list[i];

		std::string filename = make_filename(store_dir, docid);

		// read document (decompressing)
		AutoFilebuf dec(decompres(filename.c_str()));
		filebuf f = dec.getFilebuf();

		// parse document and get intra-ducument term frequency
		getWordFrequency(f, wfreq, wcconv, docid);

		// For every term in the document
		for(w = wfreq.begin(); w != wfreq.end(); ++w){
			const std::string& term = w->first;
			const int& freq =  w->second;

			// Add to vocabulary if this is an unknown term
			if (vocabulary.find(term) == vocabulary.end()) {
				// We MUST get the current number of elements
				// inside the vocabulary separatly: assigning
				// voc[term] = size() in a single statement
				// would make termid start couting from 1
				// what is something we *do not* want.
				int len = vocabulary.size();
				vocabulary[term] = len;
			}

			// add this triple to the current run(s)
			*runs++ = run_triple(	vocabulary[term],
						docid,
						freq);
		} // end for each word in document

		// Statistics and prefetching
		++d_count;
		byte_count += f.len();
		if (d_count  % 1000 == 0) {
			time_t now = time(NULL);

			uint64_t byte_amount = byte_count - last_byte_count;
			std::cout << "# docs: " << d_count << " bytes: " <<
				byte_amount << " / " << byte_count << " bps: "<<
				byte_amount/(now - last_broadcast) << 
				" elapsed " << now - time_started << std::endl;

			last_broadcast = now;
			last_byte_count = byte_count;

			if( (i + 1000) < docids_list.size()){
				std::vector<docid_t> pref(&docids_list[i],
							&docids_list[i+1000]);
				prefetchDocs(store_dir,pref);
			}
		}


	} // end for each document

	dump_vocabulary(vocabulary, output_dir);
}

/***********************************************************************
				      TEST
 ***********************************************************************/

void prefetchDocs(const char* store_dir, std::vector<docid_t>& ids)
{
	std::vector<docid_t>::const_iterator i;
	for(i = ids.begin(); i != ids.end(); ++i){
		std::string filename = make_filename(store_dir, *i);
		ManagedFilePtr p(filename.c_str());
	}
}

void testTripleInserter()
{
	const int KB = 1<<10;

	run_inserter run("/tmp/runtest/", 10*KB);

	for(int i = int(7.5*float(KB)); i > 0 ; --i){
		*run++ = run_triple(i,i,i);
	}

	run.flush();
	std::cout << "Foram geradas " << run.getNRuns() <<
		" runs" << std::endl;

}


void dumpWFreq(filebuf& f, StrIntMap& wfreq)
{
	WideCharConverter wcconv;

	getWordFrequency(f, wfreq, wcconv);
	StrIntMap::const_iterator i;
//        for(i = wfreq.begin(); i != wfreq.end(); ++i){
//                std::cout << i->first << ": " << i->second 
//                        << std::endl;
//        }

}


// EOF
