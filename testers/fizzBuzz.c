#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

int main() {
    int i;
    for (i = 1; i <= 10; i++)
    {
        if (i % 15 == 0) {
            print(WRITETERMINAL, "FizzBuzz\n");
        } else if (i % 3 == 0) {
            print(WRITETERMINAL, "Fizz\n");
        }
        else if (i % 5 == 0)
        {
            print(WRITETERMINAL, "Buzz\n");
        }
        else
        {
            char a[3];
            a[0] = i + '0';
            a[1] = '\n';
            a[2] = '\0';
            print(WRITETERMINAL, a);
        }
    }
    return 0;
}