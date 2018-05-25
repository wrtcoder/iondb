/******************************************************************************/
/**
@file		iinq_functions.c
@author		Dana Klamut
@brief		This code contains definitions for iinq pre-defined functions
@copyright	Copyright 2017
			The University of British Columbia,
			IonDB Project Contributors (see AUTHORS.md)
@par Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
@par 1.Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
@par 2.Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
@par 3.Neither the name of the copyright holder nor the names of its contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.
@par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
/******************************************************************************/

#include "iinq_functions.h"

void
iinq_execute(
	char					*table_name,
	void					*key,
	unsigned char			*value,
	iinq_operation_type_t	type
) {
	ion_err_t					error;
	ion_dictionary_t			dictionary;
	ion_dictionary_handler_t	handler;

	dictionary.handler	= &handler;

	error				= iinq_open_source(table_name, &dictionary, &handler);

	if (err_ok != error) {
		printf("Error occurred. Error code: %i. Line 51 iinq_functions.c.\n", error);
		return;
	}

	ion_status_t status;

	switch (type) {
		case iinq_insert_t:
			status = dictionary_insert(&dictionary, key, value);
			break;

		case iinq_delete_t:
			status = dictionary_delete(&dictionary, key);
			break;

		case iinq_update_t:
			status = dictionary_update(&dictionary, key, value);
			break;
	}

	if (err_ok != status.error) {
		printf("Error occurred. Error code: %i. Line 70 iinq_functions.c.\n", status.error);
		return;
	}

	/* Make more generic */
/*	if (memcmp(table_name, table_name, sizeof(table_name)) == 0) { */
/*		print_table_cats(&dictionary); */
/*	} */

	if (type == iinq_update_t) {
		printf("Update complete.\n");
	}

	error = ion_close_dictionary(&dictionary);

	if (err_ok != error) {
		printf("Error occurred. Error code: %i. Line 82 iinq_functions.c.\n", error);
		return;
	}
}

ion_cursor_status_t
iinq_next_record(
	ion_dict_cursor_t	*cursor,
	ion_record_t		*record
) {
	return cursor->next(cursor, record);
}

ion_boolean_t
where(
	unsigned char	*id,
	ion_record_t	*record,
	int				num_fields,
	va_list			*where
) {
	int		i;
	va_list where_list;

	va_copy(where_list, *where);

	int						fields[num_fields];
	iinq_bool_operator_t	operators[num_fields];
	void					*field_values[num_fields];

	iinq_where_params_t iinq_where;

	ion_boolean_t condition_failed = boolean_false;

	for (i = 0; i < num_fields; i++) {
		iinq_where = va_arg(where_list, iinq_where_params_t);

		unsigned char *curr			= record->value;

		iinq_field_t	field_type	= getFieldType(id, iinq_where.where_field);
		int				int_value	= 0;
		char			*char_value = NULL;

		if (field_type == iinq_int) {
			int_value = (int) iinq_where.field_value;
		}
		else if (field_type == iinq_null_terminated_string){
			char_value = malloc(calculateOffset(id, (iinq_where.where_field))-calculateOffset(id,(iinq_where.where_field - 1)));
			strncpy(char_value, (char *) iinq_where.field_value, sizeof(iinq_where.field_value));
		} else {
			size_t size = calculateOffset(id,(iinq_where.where_field))-calculateOffset(id,(iinq_where.where_field-1));
			char_value = malloc(size);
			memcpy(char_value, (char *) iinq_where.field_value, sizeof(iinq_where.field_value));
		}

		curr = curr + calculateOffset(id, (iinq_where.where_field - 1));

		switch (iinq_where.operator) {
			case iinq_equal:

				if (field_type == iinq_int) {
					if (*(int *) curr != int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string) {
					if (strncmp(char_value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}
				break;

			case iinq_not_equal:

				if (field_type == iinq_int) {
					if (*(int *) curr == int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string){
					if (strncmp(char_value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) == 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}

				break;

			case iinq_less_than:

				if (field_type == iinq_int) {
					if (*(int *) curr >= int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string) {
					if (strncmp(char_value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) >= 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}

				break;

			case iinq_less_than_equal_to:

				if (field_type == iinq_int) {
					if (*(int *) curr > int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string) {
					char *value = (char *) iinq_where.field_value;

					if (strncmp(value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) > 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}

				break;

			case iinq_greater_than:

				if (field_type == iinq_int) {
					if (*(int *) curr <= int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string){
					if (strncmp(char_value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) <= 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}

				break;

			case iinq_greater_than_equal_to:

				if (field_type == iinq_int) {
					if (*(int *) curr < int_value) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_null_terminated_string) {
					if (strncmp(char_value, curr, calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) < 0) {
						condition_failed = boolean_true;
					}
				}
				else if (field_type == iinq_char_array) {
					if (memcmp(char_value, curr,
							   calculateOffset(id, iinq_where.where_field - 1) - calculateOffset(id, iinq_where.where_field - 2)) != 0) {
						condition_failed = boolean_true;
					}
				}

				break;
		}
		if (char_value != NULL) {
			free(char_value);
		}
	}

	return condition_failed;
}

void
SQL_execute(
	char *sql
) {
	printf("\n%s\n", sql);
}