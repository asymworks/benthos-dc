/*
 * _dumpbackends.c
 *
 *  Created on: Apr 26, 2013
 *      Author: lowsnr
 */

#include <stdlib.h>
#include <stdio.h>

#include <libdivecomputer/common.h>
#include <libdivecomputer/descriptor.h>
#include <libdivecomputer/iterator.h>

int main(int argc, char ** argv)
{
	dc_iterator_t * iterator = NULL;
	dc_descriptor_t * descriptor = NULL;
	dc_descriptor_iterator(& iterator);

	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS)
	{
		printf("%u,%s,%s,%u\n",
			dc_descriptor_get_type(descriptor),
			dc_descriptor_get_vendor(descriptor),
			dc_descriptor_get_product(descriptor),
			dc_descriptor_get_model(descriptor));

		dc_descriptor_free(descriptor);
	}

	dc_iterator_free(iterator);
}
