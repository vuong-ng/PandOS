#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

int isPrime(int n)
{
    if(n <= 1) return 0;
    if(n == 2) return 1;
    int i;
    for(i = 2; i*i <= n; i++)
    {
        if(n % i == 0)
            return 0;
    }
    return 1;
}


void main() {

    int number = 20;
    int i;
    for (i = 0; i < number; i++)
    {
        if (isPrime(i))
            print(WRITETERMINAL, "The number is prime\n");
        else 
            print(WRITETERMINAL, "The number is not prime\n");
    }
    
}