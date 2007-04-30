#ifndef __DOMAINS_TEST_H
#define __DOMAINS_TEST_H

#include "domains.h"
#include "cxxtest/TestSuite.h"

#include <stdlib.h>
#include <unistd.h>
#include <memory>
#include <queue>

/**
 * @todo better tests.
 * */
class DomainsTestSuit : public CxxTest::TestSuite {
public:
	
	void test_OperatorLess()
	{
		AbstractHyperDimentionalCrawlerDeity* manager = 0;

		Domain* a = new Domain(std::string("exemple.tld"), URLSet(), *manager, false);
		Domain* b = new Domain(std::string("exemple.asd"), URLSet(), *manager, false);
		
		a->timestamp = 1;
		b->timestamp = 2;

		std::priority_queue<Domain*,std::vector<Domain*>,DomainPtrSmallest> pri;

		pri.push(b); 
		pri.push(a); 

		TS_ASSERT_EQUALS( pri.top(), a);
		pri.pop();
		TS_ASSERT_EQUALS( pri.top(), b);

		delete a;
		delete b;
	}

};


#endif // __DOMAINS_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
