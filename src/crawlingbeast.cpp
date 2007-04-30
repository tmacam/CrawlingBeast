/**@file crawlingbeast.cpp
 * @brief Our crawling beast.
 *
 * This crawler was previously known as adelide, my dwarf from Paraguay.
 * Adelaide was previously known as Laracna, the big and fatty crawling
 * spinder.
 */

#include "deepthought.h"
#include "paranoidandroid.h"
#include "sauron.h"

const int N_OF_WORKERS = 20;

int main(int argc, char* argv[])
{
	URLSet seeds;
	ParanoidAndroid* looser = 0;
	std::list<ParanoidAndroid* > ArmyOfMarvins;


	seeds.insert(BaseURLParser("http://www.uol.com.br/"));
	std::string store_dir = "/tmp/down/";

	std::cout << "Starting things up...";

	DeepThought boss(store_dir);
	Sauron MordorTuristGuide(boss, store_dir + "/stats");
	MordorTuristGuide.start();

	std::cout << "Loading data from previous invocations and from seeds...";
	boss.unserialize();
	boss.addPages(seeds);
	std::cout << " done." << std::endl;

	std::cout << "Starting paranoid crawling androids" << std::endl;
	for(int i = 0; i < N_OF_WORKERS; ++i){
		looser  = new ParanoidAndroid(boss);
		looser->start();
		ArmyOfMarvins.push_back( looser );
	}

	
	// Stop asa soon as the user press any key
	std::string dont_panic;
	std::cin >> dont_panic;
	boss.stopPlease();

	std::cout << "Exiting. Waiting for threads... What a bugger!" << std::endl;


	for(int i = 0; i < N_OF_WORKERS; ++i){
		looser = ArmyOfMarvins.front();
		looser->join();
		ArmyOfMarvins.pop_front();
		delete looser;
	}
	MordorTuristGuide.join();

	std::cout << "Finished." << std::endl;

}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
