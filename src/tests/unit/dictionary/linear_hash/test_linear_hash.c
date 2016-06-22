/*
 * test_linear_hash.c
 *
 *  Created on: Apr 7, 2015
 *	  Author: workstation
 */

#include "test_linear_hash.h"
#include "../../../../kv_system.h"

#define TEST_FILE "lh_main.bin"

/**
 * Deletes linear hashmap from disk
 * @param lhm
 */
void
delete_linear_hash(
	linear_hashmap_t *lhm
) {
	lh_destroy(lhm);
}

/**
@brief	  Tests the size restriction on the initial size of the linear hash

@param	  tc
				CuTest
 */
void
test_file_linear_size_test(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 1;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	/* this will generate an error */
	initial_size			= 1;

	err_t err = lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	PLANCK_UNIT_ASSERT_TRUE(tc, err == err_invalid_initial_size);

	structure_ID = 2;

	ion_key_size_t test_size;

	for (test_size = 0; test_size < 1 << 16; test_size++) {
		err = lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, test_size, structure_ID);

		if ((test_size < 2) || ((1 << (int) floor(log2(test_size))) != test_size)) {
			/** check to ensure that the size is a 2^n value as required*/
			PLANCK_UNIT_ASSERT_TRUE(tc, err == err_invalid_initial_size);
		}
		else {
#if DEBUG
			DUMP(test_size, "%i");
			DUMP((1 << (int) floor(log2(test_size))), "%i");
			DUMP(err, "%i");
#endif
			PLANCK_UNIT_ASSERT_TRUE(tc, err == err_ok);
			delete_linear_hash(&hashmap);
		}
	}
}

/**
@brief	  Tests the initialization of the linear hash

@param	  tc
				CuTest
 */
void
test_file_linear_initialization(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 4;
	int					initial_size;
	record_info_t		record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.super.record.key_size == record.key_size);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.super.record.value_size == record.value_size);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.initial_map_size == initial_size);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.compute_hash == &lh_compute_hash);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.write_concern == wc_duplicate);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.bucket_pointer == 0);
	PLANCK_UNIT_ASSERT_TRUE(tc, hashmap.file_level == 0);

	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the hash function based on file level

@param	  tc
				CuTest
 */
void
test_file_linear_hash_hash_test(
	planck_unit_test_t *tc
) {
	linear_hashmap_t hashmap;

	hashmap.initial_map_size = 4;

	int			size_of_key = sizeof(int);
	int			file_level	= 0;
	hash_set_t	*hash_set	= NULL;
	int			key;

#if DEBUG
	DUMP(hash_set, "%p");
#endif

	/** file level should be know */
	err_t status = lh_compute_hash(&hashmap, (ion_key_t) &key, size_of_key, file_level, hash_set);

	/* check without malloc */
	PLANCK_UNIT_ASSERT_TRUE(tc, err_uninitialized == status);

	hash_set	= (hash_set_t *) malloc(sizeof(hash_set_t));

	key			= 10;
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_compute_hash(&hashmap, (ion_key_t) &key, size_of_key, file_level, hash_set));
	PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << (file_level + 1)) * hashmap.initial_map_size) == hash_set->upper_hash);
	PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << file_level) * hashmap.initial_map_size) == hash_set->lower_hash);	/** Value should be invalid as file level is 0*/

	file_level = 1;	/** increase file level which should produce 2 hashes */

	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_compute_hash(&hashmap, (ion_key_t) &key, size_of_key, file_level, hash_set));
	PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << file_level) * hashmap.initial_map_size) == hash_set->lower_hash);
	PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << (file_level + 1)) * hashmap.initial_map_size) == hash_set->upper_hash);

	/* check correctness for both key and file level */

	for (file_level = 0; file_level < 10 /** 10 levels */; file_level++) {
		for (key = 0; key < (1 << 16); key++) {
			PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_compute_hash(&hashmap, (ion_key_t) &key, size_of_key, file_level, hash_set));
			PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << file_level) * hashmap.initial_map_size) == hash_set->lower_hash);
			PLANCK_UNIT_ASSERT_TRUE(tc, key % ((1 << (file_level + 1)) * hashmap.initial_map_size) == hash_set->upper_hash);
		}
	}

	free(hash_set);
}

/**
@brief	  Tests the initialization of the linear hash

@param	  tc
				CuTest
 */
void
test_file_linear_hash_insert(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 5;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key		= 10;
	char	*value	= "value";

#if DEBUG
	io_printf("inserting a new value\n");
#endif


	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	key = 14;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	key = 18;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	key = 22;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	lh_close(&hashmap);	/** closes the structure */

	delete_linear_hash(&hashmap);	/** closes and deletes? */
}

/**
@brief	  Tests the initialization of the linear hash

@param	  tc
				CuTest
 */
void
test_file_linear_hash_insert_negative(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 5;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */
	hashmap.super.compare	= dictionary_compare_signed_value;

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key[4] = { 0, -4, -8, -12 };
	char	value[hashmap.super.record.value_size];
	char	query_value[hashmap.super.record.value_size];

#if DEBUG
	io_printf("inserting a new value\n");
#endif

	int idx = 0;

	ion_status_t status;
	for (; idx < 4; idx++) {
		sprintf(value, "value:%i", key[idx]);
		status = lh_insert(&hashmap, (ion_key_t) &key[idx], (ion_value_t) value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);
	}

	for (idx = 0; idx < 4; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		status = lh_query(&hashmap, (ion_key_t) &key[idx], (ion_value_t) query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);
		sprintf(value, "value:%i", key[idx]);

#if DEBUG
		DUMP(value, "%s");
		DUMP(query_value, "%s");
#endif
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	lh_close(&hashmap);	/** closes the structure */

	delete_linear_hash(&hashmap);	/** closes and deletes? */
}

/**
@brief	  Tests the split function

@param	  tc
				CuTest
 */
void
test_file_linear_hash_split_1(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 6;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key		= 0;
	char	*value	= "value";

#if DEBUG
	io_printf("inserting a new value\n");
#endif

	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 4;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	/** and check correctness of file */
	frewind(hashmap.file);

	int				idx;
	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4 };
	int				pre_split_status[]	= { IN_USE, IN_USE, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
#if DEBUG
			DUMP(*(int *) item->data, "%i");
#endif
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	/* printf("getting ready to split\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);

	frewind(hashmap.file);
	record_size = SIZEOF(STATUS) + record.key_size + record.value_size;

	int post_split[]		= { 0, 4, -1, -1, -1, -1, -1, -1, 4, -1 };
	int post_split_status[] = { IN_USE, DELETED, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, IN_USE, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, post_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, post_split[idx] == *(int *) item->data);
		}
	}

	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the split function with overflow files

@param	  tc
				CuTest
 */
void
test_file_linear_hash_split_2(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 7;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key		= 0;
	char	*value	= "value";

#if DEBUG
	io_printf("inserting a new value\n");
#endif

	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 4;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 8;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 12;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	/** and check correctness of file */
	frewind(hashmap.file);

	int				idx					= 0;
	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4 };
	int				pre_split_status[]	= { IN_USE, IN_USE, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	/** and check ll */
	ll_file_t ll_file;

	fll_open(&ll_file, NULL, hashmap.super.key_type, hashmap.super.record.key_size, hashmap.super.record.value_size, 0, hashmap.id);/* valgrind, data loss --FIXME */
	fll_reset(&ll_file);

	ll_file_node_t ll_node;

	/*
	int overflow[] = {8,12};
	idx = 0;
	int idxx = 0;
	int idxxx = 0;
	int idxxxx = 0; //HEATH Issues within the next 64 bytes exist; overflow exists somewhere. */
	while (fll_next(&ll_file, &ll_node) != err_item_not_found) {
		/* if (idx == 30060) {idx = 0;}	//HEATH DEBUG BREAKPOINT --FIXME //Moment it enters this loop, even if I make a different variable, it becomes 30060. */
#if DEBUG
		DUMP(*(int *) ll_node.data, "%i");
		/*
		DUMP(idx,"%i");
		DUMP(idxx,"%i");
		DUMP(idxxx,"%i");
		DUMP(idxxxx,"%i"); */
#endif
		/* io_printf("\n%i  %i\n%i\n",overflow[0],overflow[1],ll_node.data); */
		/* PLANCK_UNIT_ASSERT_TRUE(tc, overflow[idxxxx++]			== *(int*)ll_node.data); */
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, err_item_not_found == fll_next(&ll_file, &ll_node));

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);

	frewind(hashmap.file);
	record_size = SIZEOF(STATUS) + record.key_size + record.value_size;

	int post_split[]		= { 0, 8, -1, -1, -1, -1, -1, -1, 4, 12 };
	int post_split_status[] = { IN_USE, IN_USE, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, post_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, post_split[idx] == *(int *) item->data);
		}
	}

	fll_close(&ll_file);
	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the split function with overflow files

@param	  tc
				CuTest
 */
void
test_file_linear_hash_split_3(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 8;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key		= 0;
	char	*value	= "value";

#if DEBUG
	io_printf("inserting a new value\n");
#endif

	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 4;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 8;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 12;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 16;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	key = 20;
	lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

	/** and check correctness of file */
	frewind(hashmap.file);

	int				idx;
	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4 };
	int				pre_split_status[]	= { IN_USE, IN_USE, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	/** and check ll */
	ll_file_t ll_file;

	fll_open(&ll_file, NULL, hashmap.super.key_type, hashmap.super.record.key_size, hashmap.super.record.value_size, 0, hashmap.id);
	fll_reset(&ll_file);

	ll_file_node_t ll_node;
	/* FIXME address this unit test.
	int overflow[] = {8,12,16,20};
	idx = 0;
	int idxx = 0;
	int idxxx = 0;
	int idxxxx = 0; */
	err_t dbg;

	while ((dbg = fll_next(&ll_file, &ll_node)) != err_item_not_found) {
		/* Next didnt advance */

		/* if (idx == 30060) {idx = 0;}	//HEATH DEBUG BREAKPOINT --FIXME //Moment it enters this loop, even if I make a different variable, it becomes 30060. */
#if DEBUG
		io_printf("\n");
		DUMP(*(int *) ll_node.data, "%i");
		DUMP(dbg, "%i");
		/*
		DUMP(idx,"%i");
		DUMP(idxx,"%i");
		DUMP(idxxx,"%i");
		DUMP(idxxxx,"%i"); */
#endif
		/* idxxxx++;//PLANCK_UNIT_ASSERT_TRUE(tc, overflow[idxxxx++]				== *(int*)ll_node.data); */
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, err_item_not_found == fll_next(&ll_file, &ll_node));

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);

	frewind(hashmap.file);
	record_size = SIZEOF(STATUS) + record.key_size + record.value_size;

	int post_split[]		= { 0, 8, -1, -1, -1, -1, -1, -1, 4, 12 };
	int post_split_status[] = { IN_USE, IN_USE, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level); idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, post_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, post_split[idx] == *(int *) item->data);
		}
	}

	fll_close(&ll_file);
	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the split function with overflow files

@param	  tc
				CuTest
 */
void
test_file_linear_hash_split_4(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 9;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	int		key[]	= { 0, 1, 2, 3, 4, 5, 6, 7 };
	char	*value	= "value";
	int		idx		= 0;

	for (idx = 0; idx < 8; idx++) {
#if DEBUG
		io_printf("inserting a new value\n");
#endif
		lh_insert(&hashmap, (ion_key_t) &key[idx], (ion_value_t) value);
	}

	/** and check correctness of file */
	frewind(hashmap.file);

	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4, 1, 5, 2, 6, 3, 7 };
	int				pre_split_status[]	= { IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	fflush(hashmap.file);
	/* printf("**************Split!****************\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));

	/* printf("**************Split!****************\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));

	/* printf("**************Split!****************\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, 2 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));

	/* printf("**************Split!****************\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, 3 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));

	/* printf("**************Split!****************\n"); */
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.file_level);

	frewind(hashmap.file);
	record_size = SIZEOF(STATUS) + record.key_size + record.value_size;

	int post_split[]		= { 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1 };
	int post_split_status[] = { IN_USE, DELETED, IN_USE, DELETED, IN_USE, DELETED, IN_USE, DELETED, IN_USE, EMPTY, IN_USE, EMPTY, IN_USE, EMPTY, IN_USE, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, post_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, post_split[idx] == *(int *) item->data);
		}
	}

	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the query operation

@param	  tc
				CuTest
 */
void
test_file_linear_hash_query(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 11;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	hashmap.super.compare	= dictionary_compare_signed_value;

	int		key[]	= { 0, 1, 2, 3, 4, 5, 6, 7 };
	char	value[10];
	int		idx		= 0;

	for (idx = 0; idx < 8; idx++) {
#if DEBUG
		io_printf("inserting a new value\n");
#endif
		sprintf(value, "value:%i", idx);
		lh_insert(&hashmap, (ion_key_t) &key[idx], (ion_value_t) value);
	}

	/** and check correctness of file */
	frewind(hashmap.file);

	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4, 1, 5, 2, 6, 3, 7 };
	int				pre_split_status[]	= { IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 2 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 3 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.file_level);

	frewind(hashmap.file);
	record_size = SIZEOF(STATUS) + record.key_size + record.value_size;

	int post_split[]		= { 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1 };
	int post_split_status[] = { IN_USE, DELETED, IN_USE, DELETED, IN_USE, DELETED, IN_USE, DELETED, IN_USE, EMPTY, IN_USE, EMPTY, IN_USE, EMPTY, IN_USE, EMPTY };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, post_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, post_split[idx] == *(int *) item->data);
		}
	}

	/** query file to ensure that values are found */
	ion_value_t query_value = (ion_value_t) malloc(record.value_size);

	for (idx = 0; idx < 8; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		ion_status_t status = lh_query(&hashmap, (ion_key_t) &key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		/* TODO Verify the count here */
		sprintf(value, "value:%i", key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	free(query_value);
	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the query operation

@param	  tc
				CuTest
 */
void
test_file_linear_hash_query_2(
	planck_unit_test_t *tc
) {
	int structure_ID = 12;

	linear_hashmap_t hashmap;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	hashmap.super.compare	= dictionary_compare_signed_value;

	int		key[]	= { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	char	value[10];
	int		idx		= 0;

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("inserting a new value\n");
#endif
		sprintf(value, "value:%i", idx);
		lh_insert(&hashmap, (ion_key_t) &key[idx], (ion_value_t) value);
	}

	/** and check correctness of file */
	frewind(hashmap.file);

	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4, 1, 5, 2, 6, 3, 7 };
	int				pre_split_status[]	= { IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	/** query file to ensure that values are found before split as they should be in overflow files */
	ion_value_t query_value = (ion_value_t) malloc(record.value_size);

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		ion_status_t status = lh_query(&hashmap, (ion_key_t) &key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		/* TODO Verify count? */
		sprintf(value, "value:%i", key[idx]);

#if DEBUG
		DUMP(value, "%s");
		DUMP(query_value, "%s");
#endif
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 2 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 3 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.file_level);

	/** query file to ensure that values are found */

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		ion_status_t status = lh_query(&hashmap, (ion_key_t) &key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		/* TODO Verify count? */
		sprintf(value, "value:%i", key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	free(query_value);
	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the query operation

@param	  tc
				CuTest
 */
void
test_file_linear_hash_delete(
	planck_unit_test_t *tc
) {
	int structure_ID = 13;

	linear_hashmap_t hashmap;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;
	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	hashmap.super.compare	= dictionary_compare_signed_value;

	int		key[]	= { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	char	value[10];
	int		idx		= 0;

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("inserting a new value\n");
#endif
		sprintf(value, "value:%i", idx);
		lh_insert(&hashmap, (ion_key_t) &key[idx], (ion_value_t) value);
	}

	/** and check correctness of file */
	frewind(hashmap.file);

	int				record_size			= SIZEOF(STATUS) + record.key_size + record.value_size;
	l_hash_bucket_t *item				= (l_hash_bucket_t *) malloc(record_size);
	int				pre_split[]			= { 0, 4, 1, 5, 2, 6, 3, 7 };
	int				pre_split_status[]	= { IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE, IN_USE };

	for (idx = 0; idx < hashmap.initial_map_size * (1 << hashmap.file_level) * RECORDS_PER_BUCKET; idx++) {
		fread(item, record_size, 1, hashmap.file);
		PLANCK_UNIT_ASSERT_TRUE(tc, pre_split_status[idx] == item->status);

		if (IN_USE == item->status) {
			PLANCK_UNIT_ASSERT_TRUE(tc, pre_split[idx] == *(int *) item->data);
		}
	}

	/** query file to ensure that values are found before split as they should be in overflow files */
	ion_value_t query_value = (ion_value_t) malloc(record.value_size);

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		ion_status_t status = lh_query(&hashmap, (ion_key_t) &key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		sprintf(value, "value:%i", key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 2 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 3 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.file_level);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == lh_split(&hashmap));
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.bucket_pointer);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == hashmap.file_level);

	/** query file to ensure that values are found */

	for (idx = 0; idx < 12; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", key[idx]);
#endif
		ion_status_t status = lh_query(&hashmap, (ion_key_t) &key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		sprintf(value, "value:%i", key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	int del_key[]		= { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11 };
	int key_to_delete	= 5;

	ion_status_t status = lh_delete(&hashmap, (ion_key_t) &key_to_delete);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);
	status = lh_delete(&hashmap, (ion_key_t) &key_to_delete);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_item_not_found == status.error);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == status.count);

	for (idx = 0; idx < 11; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", del_key[idx]);
#endif
		status = lh_query(&hashmap, (ion_key_t) &del_key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		sprintf(value, "value:%i", del_key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
	}

	status = lh_query(&hashmap, (ion_key_t) &key_to_delete, query_value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_item_not_found == status.error);
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == status.count);

	for (idx = 0; idx < 11; idx++) {
#if DEBUG
		io_printf("starting search for key: %i", del_key[idx]);
#endif
		status = lh_query(&hashmap, (ion_key_t) &del_key[idx], query_value);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		sprintf(value, "value:%i", del_key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == strcmp(value, (char *) query_value));
		status  =lh_delete(&hashmap, (ion_key_t) &del_key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
		PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);
		status = lh_delete(&hashmap, (ion_key_t) &del_key[idx]);
		PLANCK_UNIT_ASSERT_TRUE(tc, err_item_not_found == status.error);
		PLANCK_UNIT_ASSERT_TRUE(tc, 0 == status.count);
	}

	free(query_value);
	free(item);
	delete_linear_hash(&hashmap);
}

/**
@brief	  Tests the initialization of the linear hash

@param	  tc
				CuTest
 */
void
test_linear_hash_load_factor(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 5;

	int				size;							/** number of pages */
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	size					= 4;

	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, size, structure_ID);

	/** and don't forget to bind the compare as this is usually done at dict level */
	hashmap.super.compare = dictionary_compare_signed_value;

	/*empty so load should be 0 */
	/*Initial size = 4 with 2 records per page*/
	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == lh_compute_load_factor(&hashmap));

	/* DUMP(lh_compute_load_factor(&hashmap),"%i"); */

	char *value = "value";

	int key		= 1;

	for (; key < 9; key++) {
		lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

		int actual_load = 100 * (key) / (RECORDS_PER_BUCKET * size);

		PLANCK_UNIT_ASSERT_TRUE(tc, actual_load == lh_compute_load_factor(&hashmap));
	}

	key--;

	for (; key > 0; key--) {
		lh_delete(&hashmap, (ion_key_t) &key);

		int actual_load = 100 * (key - 1) / (RECORDS_PER_BUCKET * size);

		PLANCK_UNIT_ASSERT_TRUE(tc, actual_load == lh_compute_load_factor(&hashmap));
	}

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == hashmap.number_of_records);

	key++;	/** start back at 1 */

	/** test with overflow without split */
	for (; key < 19; key++) {
		lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);

		int actual_load = 100 * (key) / (RECORDS_PER_BUCKET * size);

		PLANCK_UNIT_ASSERT_TRUE(tc, actual_load == lh_compute_load_factor(&hashmap));
	}

	key--;

	/** Test load factor under split */

	int split_cnt = 0;

	for (; split_cnt < key; split_cnt++) {
		lh_split(&hashmap);	/* HEATH BREAKPOINT --FIXME */
		size++;	/** number of pages is increased */

		int actual_load = 100 * (key) / (RECORDS_PER_BUCKET * size);

		PLANCK_UNIT_ASSERT_TRUE(tc, actual_load == lh_compute_load_factor(&hashmap));
	}

	lh_close(&hashmap);	/** closes the structure */

	delete_linear_hash(&hashmap);	/** closes and deletes? */
}

/**
@brief	  Tests the initialization of the linear hash

@param	  tc
				CuTest
 */
void
test_file_linear_hash_update(
	planck_unit_test_t *tc
) {
	linear_hashmap_t	hashmap;
	int					structure_ID = 5;

	int				initial_size;
	record_info_t	record;

	record.key_size			= 4;
	record.value_size		= 10;

	hashmap.super.key_type	= key_type_numeric_signed;			/* default to use int key type */

	initial_size			= 4;

	lh_initialize(&hashmap, lh_compute_hash, hashmap.super.key_type, record.key_size, record.value_size, initial_size, structure_ID);

	hashmap.super.compare = dictionary_compare_signed_value;

	int key		= 1;

	char *value = "value";

	for (; key < 9; key++) {
		lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	}

	char *new_value = "eulav";

	key = 1;

	ion_status_t status = lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) new_value);

	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	ion_value_t query_value = (ion_value_t) malloc(record.value_size);

	status = lh_query(&hashmap, (ion_key_t) &key, query_value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == memcmp(query_value, new_value, hashmap.super.record.value_size));

	free(query_value);

	key		= 5;

	status	= lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) new_value);

	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	query_value = (ion_value_t) malloc(record.value_size);

	status = lh_query(&hashmap, (ion_key_t) &key, query_value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);

	PLANCK_UNIT_ASSERT_TRUE(tc, 0 == memcmp(query_value, new_value, hashmap.super.record.value_size));

	free(query_value);

	key = 1;

	status = lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);

	status = lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);

	char *next_value = "abcde";

	status = lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) next_value);

	PLANCK_UNIT_ASSERT_TRUE(tc, 3 == status.count);

	/** Delete records in pp to ensure that updates will happen in overflow */
	key = 1;
	status = lh_delete(&hashmap, (ion_key_t) &key);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
//	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	key = 5;
	status = lh_delete(&hashmap, (ion_key_t) &key);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
//	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	char *next_value2 = "fghij";

	key		= 1;

	status	= lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) next_value2);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	status	= lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) next_value2);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	status = lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1== status.count);

	/** Insert key and it should go into overflow page */
	key		= 9;
	status = lh_insert(&hashmap, (ion_key_t) &key, (ion_value_t) value);
	PLANCK_UNIT_ASSERT_TRUE(tc, err_ok == status.error);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1== status.count);

	key		= 5;
	status	= lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) next_value2);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	key		= 13;
	status	= lh_update(&hashmap, (ion_key_t) &key, (ion_value_t) next_value2);
	PLANCK_UNIT_ASSERT_TRUE(tc, 1 == status.count);

	lh_close(&hashmap);	/** closes the structure */

	delete_linear_hash(&hashmap);	/** closes and deletes? */
}

planck_unit_suite_t *
linear_hash_getsuite(
) {
	planck_unit_suite_t *suite = planck_unit_new_suite();

	planck_unit_add_to_suite(suite, test_file_linear_hash_hash_test);
	planck_unit_add_to_suite(suite, test_file_linear_size_test);
	planck_unit_add_to_suite(suite, test_file_linear_initialization);
	planck_unit_add_to_suite(suite, test_file_linear_hash_insert);
	planck_unit_add_to_suite(suite, test_file_linear_hash_insert_negative);
	planck_unit_add_to_suite(suite, test_file_linear_hash_split_1);
	planck_unit_add_to_suite(suite, test_file_linear_hash_split_2);
	planck_unit_add_to_suite(suite, test_file_linear_hash_split_3);
	planck_unit_add_to_suite(suite, test_file_linear_hash_split_4);
	planck_unit_add_to_suite(suite, test_file_linear_hash_query);
	planck_unit_add_to_suite(suite, test_file_linear_hash_query_2);
	planck_unit_add_to_suite(suite, test_file_linear_hash_delete);
	planck_unit_add_to_suite(suite, test_linear_hash_load_factor);
	planck_unit_add_to_suite(suite, test_file_linear_hash_update);
	return suite;
}

void
runalltests_linear_hash(
) {
	/* CuString	*output	= CuStringNew(); */
	planck_unit_suite_t *suite = linear_hash_getsuite();

	planck_unit_run_suite(suite);
	/* CuSuiteSummary(suite, output); */
	/* CuSuiteDetails(suite, output); */
	/* io_printf("%s\n", output->buffer); */

	planck_unit_destroy_suite(suite);
	/* CuSuiteDelete(suite); */
	/* CuStringDelete(output); */
}
