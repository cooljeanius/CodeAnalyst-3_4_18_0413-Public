#include <stdlib.h>
#include <stdio.h>

#include "smm.h"

int main ()
{
	smm::cleanup_unused_shared_memory(true);

	exit(0);	
}
