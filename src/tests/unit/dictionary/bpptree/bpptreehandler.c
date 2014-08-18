
#include "./../generic_dictionary_test.h"
#include "./../../../../dictionary/bpptree/bpptreehandler.h"

void
run_bpptreehandler_generic_test_set_1(
	CuTest		*tc
)
{
	generic_test_t	test;
	
	init_generic_dictionary_test(
		&test,
		bpptree_init,
		key_type_numeric_signed,
		sizeof(int),
		sizeof(int),
		-1		/* Dictionary size, for now is unbounded. */
	);
	
	dictionary_test_init(&test, tc);
	
	dictionary_test_insert_get(
		&test,
		10000,
		tc
	);
	
	int to_delete[] = {7, -9, 32, 1000001};
	int i;
	
	
	for (i = 0; i < sizeof(to_delete)/sizeof(int); i++)
	{
		dictionary_test_delete(
			&test,
			(ion_key_t)(&(to_delete[i])),
			false,
			tc
		);
	}
	
	dictionary_delete_dictionary(&(test.dictionary));
}

CuSuite*
bpptreehandler_get_suite(
)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, run_bpptreehandler_generic_test_set_1);

	return suite;
}

void
run_all_tests_bpptreehandler(
)
{
	CuString	*output	= CuStringNew();
	CuSuite		*suite	= bpptreehandler_get_suite();
	
	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	
	CuSuiteDelete(suite);
	CuStringDelete(output);
}
