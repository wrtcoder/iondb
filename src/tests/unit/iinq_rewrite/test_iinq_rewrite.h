/******************************************************************************/
/**
@file		test_iinq_rewrite.h
@author		Kai Neubauer
@brief		Entry point for iinq tests.
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

#ifndef PLANCK_UNIT_TEST_IINQ_REWRITE_H
#define PLANCK_UNIT_TEST_IINQ_REWRITE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "../../planck-unit/src/planck_unit.h"
#include "../../../iinq/iinq.h"
#include "../../../iinq_rewrite/iinq_rewrite.h"

#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)

#define DEFINE_SCHEMA(source_name, struct_def) \
	struct iinq_ ## source_name ## _schema struct_def

#define SCHEMA_SIZE(source_name) \
	sizeof(struct iinq_ ## source_name ## _schema)

#define DECLARE_SCHEMA_VAR(source_name, var_name) \
	struct iinq_ ## source_name ## _schema var_name

void
run_all_tests_iinq_rewrite(
);

#if defined(__cplusplus)
}
#endif

#endif /* PLANCK_UNIT_TEST_IINQ_H */