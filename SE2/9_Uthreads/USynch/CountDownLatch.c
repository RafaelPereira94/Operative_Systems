#include "USynch.h"
#include "UThread.h"
#include "List.h"
#include "WaitBlock.h"

VOID UtInitCounterLatch(PUTHREAD_COUNTER_LATCH pcount, DWORD initialValue) {
	// valida o valor inicial
	pcount->Counter = initialValue;
	InitializeListHead(&pcount->WaitListHead);
}

VOID UtWaitCounterLatch(PUTHREAD_COUNTER_LATCH pcount) {
	if (pcount->Counter > 0) {
		WAIT_BLOCK wb;
		wb.Thread = UtSelf();
		InsertTailList(&pcount->WaitListHead, &wb.Link);
		UtDeactivate();
	}
}

/* O sincronizador permanece sinalizado logo que o contador atinja o valor 0
 */
VOID UtSignalCounterLatch(PUTHREAD_COUNTER_LATCH pcount) {

	if (pcount->Counter == 0)
		return;

	pcount->Counter -= 1;

	if (pcount->Counter == 0) {
		// Sincronizador sinalizado
		while (! IsListEmpty(&pcount->WaitListHead) ) {
			PWAIT_BLOCK pwb = CONTAINING_RECORD(RemoveHeadList(&pcount->WaitListHead), WAIT_BLOCK, Link);
			UtActivate(pwb->Thread);
		}
	}
}
