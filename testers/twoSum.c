#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

void twoSum(int* nums, int numsSize, int target, int result[2]) {
    int i;
    int j;
    for (i = 0; i < numsSize; i++) {
        for (j = i + 1; j < numsSize; j++) {
            if (nums[i] + nums[j] == target) {
                result[0] = i;
                result[1] = j;
                return;
            }
        }
    }
    result[0] = -1;
    result[1] = -1;
}

void main() {
    int nums[] = {2, 7, 11, 15};
    int result[2];
    int target = 26;

    twoSum(nums, 4, target, result);

    char a[3];
    char b[3];
    if (result[0] != -1) {
        a[0] = result[0] + '0';
        a[1] = '\n';
        a[2] = '\0';
        b[0] = result[1] + '0';
        b[1] = '\n';
        b[2] = '\0';

        print(WRITETERMINAL, "First index: ");
        print(WRITETERMINAL, a);
        print(WRITETERMINAL, "Second index: ");
        print(WRITETERMINAL, b);
    }
    SYSCALL(TERMINATE, 0, 0, 0);
}
