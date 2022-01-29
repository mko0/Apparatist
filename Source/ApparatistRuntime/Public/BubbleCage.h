// 2022 Vladislav Dmitrievich Turbanov

#pragma once

#include "Containers/UnrealString.h"
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

#include "SubjectiveActor.h"

#include "BubbleCageCell.h"
#include "BubbleSphere.h"
#include "Located.h"

#include "BubbleCage.generated.h"


#define BUBBLE_DEBUG 0

/**
 * Class provides a cage with attachments logic and collision detection.
 */
UCLASS(Category = "BubbleCage")
class APPARATISTRUNTIME_API ABubbleCage : public ASubjectiveActor
{
	GENERATED_BODY()

  private:

	static ABubbleCage* Instance;

  public:

	void BeginDestroy() override
	{
		if (Instance == this)
		{
			Instance = nullptr;
		}
	}

	/* Call it before use (when the game starts or when spawned). */
	void InitializeInternalState();

	/* Neighbour cells of the cage. Useful for loop iteration. */
	static TArray<FIntVector> NeighbourOffsets;

	/* The size of one cell of the cage in the world units. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "BubbleCage")
	float CellSize = 1;

	/* The size of the cage in number of cells. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "BubbleCage")
	FIntVector Size;

	/* The bounds of the cage in world units. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "BubbleCage")
	FBox Bounds;

	/* Thikness of cage's bound box. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "BubbleCage|Debug")
	float BoundDebugBoxThikness = 100.f;

	/* Allow draw debug 3d-grid*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "BubbleCage|Debug")
	bool bDrawDebugCageCells = false;

	/* Thikness of debug cage cell line. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "BubbleCage|Debug", meta = (EditCondition = "bDrawDebugCageCells"))
	float DebugCageCellThikness = 1.f;

	/* All of the cells of the cage. */
	TArray<FBubbleCageCell> Cells;

	/* The indices of the cells that are currently occupied by the subjects. */
	TSet<int32> OccupiedCells;

	static FORCEINLINE ABubbleCage*
	GetInstance()
	{
		return Instance;
	}

	/* Sets default values for this actor's properties. */
	ABubbleCage();

	void
	BeginPlay() override
	{
		Instance = this;
		InitializeInternalState();
	}

	/* Convert world 3d-location to position in the cage. No checks. */
	FORCEINLINE FIntVector WorldToCage(FVector Point) const
	{
		Point -= Bounds.Min;
		Point /= CellSize;
		return FIntVector(FMath::FloorToInt(Point.X),
						  FMath::FloorToInt(Point.Y),
						  FMath::FloorToInt(Point.Z));
	}

	/* Get the index of the cage cell. */
	FORCEINLINE int32
	GetIndexAt(int32 X, int32 Y, int32 Z) const
	{
		X = FMath::Clamp(X, 0, Size.X - 1);
		Y = FMath::Clamp(Y, 0, Size.Y - 1);
		Z = FMath::Clamp(Z, 0, Size.Z - 1);
		return X + Size.X * (Y + Size.Y * Z);
	}

	/**
	 * Get overlapping spheres for the specified location.
	 */
	int32
	GetOverlapping(const FVector&          Location,
				   TArray<FSubjectHandle>& OutOverlappers) const
	{
		OutOverlappers.Reset();
		const auto CagePos  = WorldToCage(Location);
		for (const auto& Offset : NeighbourOffsets)
		{
			const auto NeighbourCellPos = CagePos + Offset;
			if (LIKELY(IsInside(NeighbourCellPos)))
			{
				const auto& NeighbourCell = At(NeighbourCellPos);
				for (int32 j = 0; j < NeighbourCell.Subjects.Num(); ++j)
				{
					const auto OtherBubble = NeighbourCell.Subjects[j];
					if (LIKELY(OtherBubble))
					{
						const auto OtherBubbleSphere =
							OtherBubble.GetTrait<FBubbleSphere>();
						const auto OtherLocation =
							OtherBubble.GetTrait<FLocated>().GetLocation();
						const auto Delta = Location - OtherLocation;
						const float Distance = Delta.Size();
						if (OtherBubbleSphere.Radius > Distance)
						{
							OutOverlappers.Add(OtherBubble);
						}
					}
				}
			}
		}
		return OutOverlappers.Num();
	}

	/**
	 * Get overlapping spheres for the specified location.
	 */
	int32
	GetOverlapping(const FVector&          Location,
				   const float             Radius,
				   TArray<FSubjectHandle>& OutOverlappers) const
	{
		OutOverlappers.Reset();
		const auto CagePos  = WorldToCage(Location);
		for (const auto& Offset : NeighbourOffsets)
		{
			const auto NeighbourCellPos = CagePos + Offset;
			if (LIKELY(IsInside(NeighbourCellPos)))
			{
				const auto& NeighbourCell = At(NeighbourCellPos);
				for (int32 j = 0; j < NeighbourCell.Subjects.Num(); ++j)
				{
					const auto OtherBubble = NeighbourCell.Subjects[j];
					if (LIKELY(OtherBubble))
					{
						const auto OtherBubbleSphere =
							OtherBubble.GetTrait<FBubbleSphere>();
						const auto OtherLocation =
							OtherBubble.GetTrait<FLocated>().GetLocation();
						const auto Delta = Location - OtherLocation;
						const float Distance = Delta.Size();
						const float DistanceDelta = (Radius + OtherBubbleSphere.Radius) - Distance;
						if (DistanceDelta > 0)
						{
							OutOverlappers.Add(OtherBubble);
						}
					}
				}
			}
		}
		return OutOverlappers.Num();
	}

	/* Get position in the cage by index of the cell*/
	FORCEINLINE FIntVector
	GetCellPointByIndex(int32 Index) const
	{
		int32 z = Index / (Size.X * Size.Y);
		int32 layerPadding = Index - (z * Size.X * Size.Y);

		return FIntVector(layerPadding / Size.X, layerPadding % Size.X , z);
	}

	/* Get the index of the cage cell. */
	int32 GetIndexAt(const FIntVector& CellPoint) const
	{
		return GetIndexAt(CellPoint.X, CellPoint.Y, CellPoint.Z);
	}

	/* Get the index of the cell by the world position. */
	int32 GetIndexAt(FVector Point) const
	{
		return GetIndexAt(WorldToCage(Point));
	}

	/* Get subjects in a specific cage cell. Const version. */
	FORCEINLINE const FBubbleCageCell&
	At(const int32 X, const int32 Y, const int32 Z) const
	{
		return Cells[GetIndexAt(X, Y, Z)];
	}

	/* Get subjects in a specific cage cell. */
	FORCEINLINE FBubbleCageCell&
	At(const int32 X, const int32 Y, const int32 Z)
	{
		return Cells[GetIndexAt(X, Y, Z)];
	}

	/* Check if the cage point is inside the cage. */
	FORCEINLINE bool
	IsInside(const FIntVector& CellPoint) const
	{
		return (CellPoint.X >= 0) && (CellPoint.X < Size.X) &&
			   (CellPoint.Y >= 0) && (CellPoint.Y < Size.Y) &&
			   (CellPoint.Z >= 0) && (CellPoint.Z < Size.Z);
	}

	/* Check if the world point is inside the cage. */
	FORCEINLINE bool
	IsInside(const FVector& WorldPoint) const
	{
		return IsInside(WorldToCage(WorldPoint));
	}

	/* Get subjects in a specific cage cell by position in the cage. */
	FORCEINLINE FBubbleCageCell&
	At(const FIntVector& CellPoint)
	{
		return At(CellPoint.X, CellPoint.Y, CellPoint.Z);
	}

	/* Get subjects in a specific cage cell by position in the cage. */
	FORCEINLINE const FBubbleCageCell&
	At(const FIntVector& CellPoint) const
	{
		return At(CellPoint.X, CellPoint.Y, CellPoint.Z);
	}

	/* Get subjects in a specific cage cell by world 3d-location. */
	FORCEINLINE FBubbleCageCell&
	At(FVector Point)
	{
		return At(WorldToCage(Point));
	}

	/* Get subjects in a specific cage cell by world 3d-location. */
	FORCEINLINE const FBubbleCageCell&
	At(FVector Point) const
	{
		return At(WorldToCage(Point));
	}

	/* Get a box shape representing a cell by position in the cage. */
	FORCEINLINE FBox
	BoxAt(const FIntVector& CellPoint)
	{
		const FVector Min = Bounds.Min + FVector(CellPoint) * CellSize;
		FBox Box(Min, Min + FVector(CellSize));
		return MoveTemp(Box);
	}

	/* Get a box shape representing a cell by world 3d-location. */
	FORCEINLINE FBox
	BoxAt(const FVector Point)
	{
		return BoxAt(WorldToCage(Point));
	}

	/* Draw debug box representing bounds of the cage. */
	void DrawDebugBounds(bool bPersistent = true)
	{
		DrawDebugBox(GetWorld(), GetActorLocation(),
					 Bounds.GetExtent(),
					 FColor::Yellow, bPersistent, -1, 0, BoundDebugBoxThikness);
	}

	/* Draw debug cage cells*/
	void DrawDebugCageCells(bool bPersistent = true)
	{
		const UWorld* world = GetWorld();
		
		/* Draw x-, and y-aligned lines by z coordinates*/
		for (int32 z = 0; z <= Size.Z; z++)
		{
			for (int32 x = 0; x <= Size.X; x++)
			{
				DrawDebugLine(world, FVector(Bounds.Min.X + CellSize * x, Bounds.Min.Y, Bounds.Min.Z + CellSize * z),
					FVector(Bounds.Min.X + CellSize * x, Bounds.Min.Y + CellSize * Size.Y, Bounds.Min.Z + CellSize * z),
					FColor::Green, bPersistent, -1, 0, DebugCageCellThikness);
			}

			for (int32 y = 0; y <= Size.Y; y++)
			{
				DrawDebugLine(world, FVector(Bounds.Min.X, Bounds.Min.Y + CellSize * y, Bounds.Min.Z + CellSize * z),
					FVector(Bounds.Min.X + CellSize * Size.X, Bounds.Min.Y + CellSize * y, Bounds.Min.Z + CellSize * z),
					FColor::Green, bPersistent, -1, 0, DebugCageCellThikness);
			}
		}

		/* Draw vertical lines*/
		for (int32 x = 0; x <= Size.X; x++)
		{
			for (int32 y = 0; y <= Size.Y; y++)
			{
				DrawDebugLine(world, FVector(Bounds.Min.X + CellSize * x, Bounds.Min.Y + CellSize * y, Bounds.Min.Z),
					FVector(Bounds.Min.X + CellSize * x, Bounds.Min.Y + CellSize * y, Bounds.Min.Z + CellSize * Size.Z),
					FColor::Green, bPersistent, -1, 0, DebugCageCellThikness);
			}
		}

	}

	/* Here we calculate the cage bounds and draw them. */
	virtual void OnConstruction(const FTransform& Transform) override
	{
		Super::OnConstruction(Transform);

		const auto Extents = CellSize * FVector(Size) * 0.5f;
		Bounds.Min = FVector(Transform.GetLocation()) - Extents;
		Bounds.Max = FVector(Transform.GetLocation()) + Extents;
		FlushPersistentDebugLines(GetWorld());
		DrawDebugBounds();
		if (bDrawDebugCageCells)
		{
			DrawDebugCageCells();
		}
	}

	/* Calculate the collisions between subjects with BubbleSphere trait that
	 * are positioned.
	 */
	void DoEvaluate()
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_BubbleCage_Evaluate);

		const auto Mechanism = UMachine::ObtainMechanism(GetWorld());

		// Clear-up the cage...
		for (auto Index : OccupiedCells)
		{
			Cells[Index].Subjects.Empty();
		}
		OccupiedCells.Reset();

		// Occupy the cage cells...
		auto Filter = FFilter::Make<FLocated, FBubbleSphere>();
		Mechanism->EnchainSolid(Filter)->Operate([=]
		(FSolidSubjectHandle  Subject,
		 const FLocated&      Located,
		 const FBubbleSphere& BubbleSphere)
		{
			check(BubbleSphere.Radius * 2 <= CellSize);
			const auto Location = Located.Location;
			if (UNLIKELY(!IsInside(Location)))
			{
				Subject.DespawnDeferred();
				return;
			}
			const auto CellIndex = GetIndexAt(Location);
 			Cells[CellIndex].Subjects.Add((FSubjectHandle)Subject);
#if BUBBLE_DEBUG
			const FBox CellBox = BoxAt(Location);
			DrawDebugBox(GetWorld(),
						 CellBox.GetCenter(),
						 CellBox.GetExtent(), FColor::Yellow, false,
						 0, 0, 2);
			DrawDebugSphere(GetWorld(),
							Location,
							BubbleSphere.Radius, 12, FColor::Cyan, false, 0, 0, 1);
			DrawDebugString(
				GetWorld(), CellBox.GetCenter(),
				FString::Format(TEXT("Cell #{0}"),
								FStringFormatOrderedArguments(CellIndex)));
#endif
			OccupiedCells.Add(CellIndex);
		});
		Mechanism->ApplyDeferreds();

		// Detect collisions...
		Mechanism->EnchainSolid(Filter)->OperateConcurrently([=]
		(FSolidSubjectHandle Bubble,
		 FLocated&           Located,
		 FBubbleSphere&      BubbleSphere)
		{
			const auto Location = Located.Location;
			const auto CagePos  = WorldToCage(Location);
			for (const auto& Offset : NeighbourOffsets)
			{
				const auto NeighbourCellPos = CagePos + Offset;
				if (LIKELY(IsInside(NeighbourCellPos)))
				{
					const auto& NeighbourCell = At(NeighbourCellPos);
					for (int32 j = 0; j < NeighbourCell.Subjects.Num(); ++j)
					{
						const auto OtherBubble = NeighbourCell.Subjects[j];
						if (LIKELY(OtherBubble && (OtherBubble != Bubble)))
						{
							const auto OtherBubbleSphere =
								OtherBubble.GetTrait<FBubbleSphere>();
							const auto OtherLocation =
								OtherBubble.GetTrait<FLocated>().GetLocation();
							const auto Delta = Location - OtherLocation;
							const float Distance = Delta.Size();
							const float DistanceDelta =
								(BubbleSphere.Radius + OtherBubbleSphere.Radius) - Distance;
							if (DistanceDelta > 0)
							{
								if (UNLIKELY(Distance <= 0.01f))
								{
									// The distance is too small to get the direction.
									// Use the ids to get the direction.
									FVector RandVector{ FMath::FRand(), FMath::FRand(), FMath::FRand() };

									if (Bubble.GetId() > OtherBubble.GetId())
									{
										BubbleSphere.AccumulatedDecouple +=
											RandVector * DistanceDelta *
											0.5f;
									}
									else
									{
										BubbleSphere.AccumulatedDecouple +=
											(-RandVector) * DistanceDelta *
											0.5f;
									}
								}
								else
								{
									BubbleSphere.AccumulatedDecouple +=
										(Delta / Distance) * DistanceDelta *
										0.5f;
								}
								BubbleSphere.AccumulatedDecoupleCount += 1;
							}
						}
					}
				}
			}
		}, 4);

		// Decouple...
		Mechanism->EnchainSolid(Filter)->OperateConcurrently([=]
		(FLocated& Located, FBubbleSphere& BubbleSphere)
		{
			if (BubbleSphere.AccumulatedDecoupleCount > 0)
			{
				Located.Location += BubbleSphere.AccumulatedDecouple /
									BubbleSphere.AccumulatedDecoupleCount;
				BubbleSphere.AccumulatedDecouple = FVector::ZeroVector;
				BubbleSphere.AccumulatedDecoupleCount = 0;
			}
		}, 4);
	}

	/* Calculate the collisions between subjects with BubbleSphere trait that
	 * are positioned.
	 */
	UFUNCTION(BlueprintCallable)
	static void Evaluate()
	{
		if (!Instance) return;
		Instance->DoEvaluate();
	}
};