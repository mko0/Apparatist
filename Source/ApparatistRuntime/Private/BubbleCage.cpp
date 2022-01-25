// 2022 Vladislav Dmitrievich Turbanov

#include "BubbleCage.h"


TWeakObjectPtr<ABubbleCage> ABubbleCage::Instance;

TArray<FIntVector> ABubbleCage::NeighbourOffsets;

/* Sets default values. */
ABubbleCage::ABubbleCage()
{
	PrimaryActorTick.bCanEverTick = false;
}

/* Called when the game starts or when spawned. */
void ABubbleCage::InitializeInternalState()
{
	Cells.Reset();
	Cells.AddDefaulted(Size.X * Size.Y * Size.Z);
	FlushPersistentDebugLines(GetWorld());
}