#include <time.h>
#include <stdlib.h>

#include "type.h"
#include "err.h"
#include "random.h"

volatile static bool init = false;

void init_random()
{
    if (!init) {
        init = true;
        srand(time(NULL));
    }
}

int random_in(int a, int b)
{
    ASSERT(a <= b, "random_in range err");
    return a + rand() / (RAND_MAX / (b - a + 1) + 1);
}
