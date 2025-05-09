/*	Test Flash Get and Flash Put */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

#define MILLION	1000000
#define MAXBLOCK 512
#define READCOUNT 511 /*change this parameter to test that exceeding max block should terminate the process*/

void main() {
	/*char buffer[PAGESIZE]; */
	int i;
	int dstatus;
	int *buffer;
	
	buffer = (int *)(SEG2 + (20 * PAGESIZE));

	print(WRITETERMINAL, "flashTest starts\n");
	*buffer = 42;  /*buffer[0] = 'a'; */
	dstatus = SYSCALL(FLASH_PUT, (int)buffer, 1, 33);
	
	if (dstatus != READY)
		print(WRITETERMINAL, "flashTest error: flash i/o result\n");
	else
		print(WRITETERMINAL, "flashTest ok: flash i/o result\n");

	*buffer = 100;  /*buffer[0] = 'z'; */
	dstatus = SYSCALL(FLASH_PUT, (int)buffer, 1, 36);

	dstatus = SYSCALL(FLASH_GET, (int)buffer, 1, 33);
	
	if (*buffer != 42)  /*(buffer[0] != 'a') */
		print(WRITETERMINAL, "flashTest error: bad first flash sector readback\n");
	else
		print(WRITETERMINAL, "flashTest ok: first flash sector readback\n");

	dstatus = SYSCALL(FLASH_GET, (int)buffer, 1, 36);
	
	if (*buffer != 100) /*(buffer[0] != 'z') */
		print(WRITETERMINAL, "flashTest error: bad second flash sector readback\n");
	else
		print(WRITETERMINAL, "flashTest ok: second flash sector readback\n");

	/* should eventually exceed device capacity */
	i = 0;
	dstatus = SYSCALL(FLASH_GET, (int)buffer, 1, i);
	while ((dstatus == READY) && (i < READCOUNT - 1)) {
		i++;
		dstatus = SYSCALL(FLASH_GET, (int)buffer, 1, i);
	}
	/*print(WRITETERMINAL, "next flash get should terminate due to exceeding maxblock\n");*/
	i++;
	dstatus = SYSCALL(FLASH_GET, (int)buffer, 1, i);

	if (i > MAXBLOCK)
		print(WRITETERMINAL, "flashTest error: exceeding maxblock did not terminate\n");
	
	print(WRITETERMINAL, "flashTest: completed\n");

	/* try to do a flash read into protected RAM */
	SYSCALL(FLASH_GET, SEG1, 1, 33);

	print(WRITETERMINAL, "flashTest error: just read into segment 1\n");

	/* generate a variety of program traps */
	i = i / 0;
	print(WRITETERMINAL, "flashTest error: divide by 0 did not terminate\n");

	LDST(buffer);
	print(WRITETERMINAL, "flashTest error: priv. instruction did not terminate\n");

	SYSCALL(1, (int) buffer, 0, 0);
	print(WRITETERMINAL, "flashTest error: sys1 did not terminate\n");
	
	SYSCALL(TERMINATE, 0, 0, 0);
}
