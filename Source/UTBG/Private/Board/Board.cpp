// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/Board.h"
#include "Tile/TileActor.h"
#include "Pawn/PawnBase.h"
#include "GamePlay/Team/TeamUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/UTBGPlayerController.h"
#include "GameState/UTBGGameState.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h" 

DEFINE_LOG_CATEGORY_STATIC(LogBoard, Log, All);

static const TCHAR* ToStr(EHighlightType T)
{
	switch (T)
	{
	case EHighlightType::EHT_None:       return TEXT("None");
	case EHighlightType::EHT_Selected:   return TEXT("Selected");
	case EHighlightType::EHT_Movable:    return TEXT("Movable");
	case EHighlightType::EHT_Attackable: return TEXT("Attackable");
	default:                              return TEXT("Unknown");
	}
}

static FString Pt(const FIntPoint& P) { return FString::Printf(TEXT("(%d,%d)"), P.X, P.Y); }
static FString Obj(const UObject* O) { return FString::Printf(TEXT("%s[%p]"), *GetNameSafe(O), O); }

static FString JoinPoints(const TSet<FIntPoint>& S)
{
	TArray<FString> A; A.Reserve(S.Num());
	for (const FIntPoint& P : S) { A.Add(Pt(P)); }
	return FString::Printf(TEXT("[%s]"), *FString::Join(A, TEXT(", ")));
}

ABoard::ABoard()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ABoard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool ABoard::IsGridReady() const
{
	return (Rows > 0 && Cols > 0) && (TileGrid.Num() == Rows * Cols);
}

void ABoard::BeginPlay()
{
	Super::BeginPlay();


	if (bDebugBoardLogs) // d
	{
		UE_LOG(LogBoard, Warning, TEXT("BeginPlay: Rows=%d Cols=%d N=%d, PlacedTiles=%d"), Rows, Cols, Rows * Cols, PlacedTiles.Num()); // d
	}

	// BP에서 배열을 채워두었다고 가정하고, 그걸로 그리드를 구성
	RebuildGridFromManualTiles();

	EnsureGridArrays();

	if (PendingRegister.Num() > 0)
	{
		for (auto& WeakP : PendingRegister)
		{
			if (APawnBase* P = WeakP.Get())
			{
				RegisterPawn(P);
			}
		}
		PendingRegister.Empty();
	}
}

void ABoard::RebuildGridFromManualTiles()
{
	TileGrid.SetNum(Rows * Cols);
	for (auto& E : TileGrid) E = nullptr;

	int32 Filled = 0; // d

	for (ATileActor* Tile : PlacedTiles)
	{
		if (!IsValid(Tile)) continue;

		const FIntPoint GridCoord = Tile->GetGridCoord();
		if (!IsValidCoord(GridCoord))
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("RebuildGrid: Skip invalid coord Tile=%s Coord=%s"),*GetNameSafe(Tile), *Pt(GridCoord));		//d
			continue;
		}

		const int32 index = ToIndex(GridCoord);
		if (!TileGrid.IsValidIndex(index))
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("RebuildGrid: Skip OOB idx=%d for %s"), index, *Pt(GridCoord));		//d
			continue;
		}

		TileGrid[index] = Tile;
		Filled++;			//d
	}
	if (bDebugBoardLogs) // d
	{
		UE_LOG(LogBoard, Warning, TEXT("RebuildGrid: Filled %d / %d tiles"), Filled, TileGrid.Num()); // d
	}
}

void ABoard::RegisterPawn(APawnBase* Pawn)
{
	if (!Pawn) return;

	if (!IsGridReady())
	{
		PendingRegister.AddUnique(Pawn);
		if (bDebugBoardLogs)
			UE_LOG(LogBoard, Warning, TEXT("RegisterPawn DELAYED: %s (Rows=%d Cols=%d TileGrid=%d)"),
				*GetNameSafe(Pawn), Rows, Cols, TileGrid.Num());
		return;
	}

	EnsureGridArrays(); // 배열 크기 확보

	const FIntPoint GridCoord = Pawn->GetGridCoord(); // 폰 좌표 확보
	if (!IsValidCoord(GridCoord))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("RegisterPawn: invalid coord %s (%s)"), *Pt(GridCoord), *GetNameSafe(Pawn));
		return;
	}

	const int32 Index = ToIndex(GridCoord);
	if (!PawnGrid.IsValidIndex(Index))
	{
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("RegisterPawn: OOB idx=%d %s"), Index, *Pt(GridCoord));
			return;
		}
	}
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("RegisterPawn: %s @ %s idx=%d"), *GetNameSafe(Pawn), *Pt(GridCoord), Index);
	PawnGrid[Index] = Pawn; //  폰 그리드에 폰 넣기

	Pawn->SetActorLocation(GridToWorld(GridCoord) + Pawn->PawnOffset);
}

void ABoard::UnRegisterPawn(APawnBase* Pawn)
{
	if (!Pawn) return;
	const FIntPoint Coordinate = Pawn->GetGridCoord();
	if (IsValidCoord(Coordinate))
	{
		const int32 Index = ToIndex(Coordinate);
		if (PawnGrid.IsValidIndex(Index) && PawnGrid[Index].Get() == Pawn)
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("UnRegisterPawn: %s @ %s idx=%d"),
				*GetNameSafe(Pawn), *Pt(Coordinate), Index);
			PawnGrid[Index] = nullptr;
			return;
		}
	}
}

void ABoard::EnsureGridArrays()
{
	const int32 N = Rows * Cols;
	if (TileGrid.Num() != N)  TileGrid.SetNum(N);
	if (PawnGrid.Num() != N)  PawnGrid.SetNum(N);
	if (bDebugBoardLogs) UE_LOG(LogBoard, Verbose, TEXT("EnsureGridArrays: N=%d"), N);
}

FVector ABoard::GridToWorld(const FIntPoint& Grid) const
{
	return BoardOrigin + FVector(Grid.X * TileSize, Grid.Y * TileSize, 0.f);
}

FIntPoint ABoard::WorldToGrid(const FVector& WorldLoc) const
{
	const FVector Local = WorldLoc - BoardOrigin;
	return FIntPoint(FMath::RoundToInt(Local.X / TileSize), FMath::RoundToInt(Local.Y / TileSize));
}

void ABoard::MulticastUnitMoved_Implementation(APawnBase* Unit, FIntPoint From, FIntPoint To)
{
	if (!IsValid(Unit)) return;

	if (bDebugBoardLogs)
	{
		const int32 DebugAP = GetRemainingAP();
		UE_LOG(LogBoard, Warning,
			TEXT("[HL] MulticastUnitMoved: Unit=%s From=%s To=%s HasAuth=%d Sel=%s State=%d AP=%d"),
			*Obj(Unit), *Pt(From), *Pt(To), HasAuthority(), *Obj(SelectedUnit),
			(int32)BoardState, DebugAP);
	}

	if (!HasAuthority())
	{
		const int32 OldIdx = ToIndex(From);
		const int32 NewIdx = ToIndex(To);

		if (PawnGrid.IsValidIndex(OldIdx) && PawnGrid[OldIdx].Get() == Unit)
			PawnGrid[OldIdx] = nullptr;

		if (PawnGrid.IsValidIndex(NewIdx))
			PawnGrid[NewIdx] = Unit;

		const FIntPoint Before = Unit->GetGridCoord();
		if (bDebugBoardLogs)
			UE_LOG(LogBoard, Warning, TEXT("[HL]   ClientUnitCoord: before=%s"), *Pt(Before));

		//Unit->SetGridCoord(To);
		Unit->SetActorLocation(GridToWorld(To) + Unit->PawnOffset);

		if (bDebugBoardLogs)
			UE_LOG(LogBoard, Warning, TEXT("[HL]   ClientUnitCoord: after =%s"), *Pt(Unit->GetGridCoord()));
	}
	ClearAllHighlights();
	if (SelectedUnit && IsValid(SelectedUnit)) EnterUnitSelected(SelectedUnit);
	else UpdateHighLights();
}

void ABoard::UpdatePawnCell(APawnBase* Pawn, const FIntPoint& Prev, const FIntPoint& New)
{
	EnsureGridArrays();

	if (IsValidCoord(Prev))
	{
		const int32 OldIdx = ToIndex(Prev);
		if (PawnGrid.IsValidIndex(OldIdx) && PawnGrid[OldIdx].Get() == Pawn)
			PawnGrid[OldIdx] = nullptr;
	}
	if (IsValidCoord(New))
	{
		const int32 NewIdx = ToIndex(New);
		if (PawnGrid.IsValidIndex(NewIdx))
			PawnGrid[NewIdx] = Pawn;
	}
}

void ABoard::HighlightTile(const FIntPoint& T, EHighlightType Type)
{
	if (!IsValidCoord(T)) return;
	if (ATileActor* Tile = TileGrid[ToIndex(T)])
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("HighlightTile %s -> %s (%s)"), *Pt(T), ToStr(Type), *GetNameSafe(Tile));
		Tile->Highlight(Type);
	}
}

void ABoard::HighlightTiles(const TArray<FIntPoint>& Tiles, EHighlightType Type)
{
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("HighlightTiles(Array) Count=%d Type=%s"),
		Tiles.Num(), ToStr(Type));
	for (const FIntPoint& T : Tiles)
	{
		if (!IsValidCoord(T)) continue;

		if (ATileActor* Tile = TileGrid[ToIndex(T)])
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Verbose, TEXT("  -> %s (%s)"), *Pt(T), *GetNameSafe(Tile));
			Tile->Highlight(Type);
		}
	}
}

void ABoard::HighlightTiles(const TSet<FIntPoint>& Tiles, EHighlightType Type)
{
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("HighlightTiles(Set) Count=%d Type=%s"), Tiles.Num(), ToStr(Type));
	for (const FIntPoint& T : Tiles)
	{
		if (!IsValidCoord(T)) continue;

		if (ATileActor* Tile = TileGrid[ToIndex(T)])
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Verbose, TEXT("  -> %s (%s)"), *Pt(T), *GetNameSafe(Tile));
			Tile->Highlight(Type);
		}
	}
}

void ABoard::ClearAllHighlights()
{
	int32 Cleared = 0; // for debug
	for (ATileActor* T : TileGrid)
	{
		if (IsValid(T)) { T->Highlight(EHighlightType::EHT_None); ++Cleared; }
	}
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("ClearAllHighlights: Cleared=%d"), Cleared);
}

void ABoard::UpdateHighLights()
{
	if (bDebugBoardLogs)
	{
		const int32 DebugAP = GetRemainingAP();
		UE_LOG(LogBoard, Warning, TEXT("[HL] UpdateHighLights: Selected=%s AP=%d"),
			*Obj(SelectedUnit), DebugAP);
		UE_LOG(LogBoard, Warning, TEXT("[HL] -> ClearAllHighlights() (inside Update)"));
	}
	ClearAllHighlights();


	if (!SelectedUnit)
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[HL]   No SelectedUnit -> return"));
		return;
	}

	const FIntPoint Sel = SelectedUnit->GetGridCoord(); // for debug
	if (bDebugBoardLogs)
	{
		UE_LOG(LogBoard, Warning, TEXT("[HL]   Highlight Selected @ %s"), *Pt(Sel));
		UE_LOG(LogBoard, Warning, TEXT("[HL]   Will highlight Movable=%d, Attackable=%d"),
			MovableTileSet.Num(), AttackableTileSet.Num());
	}

	// 1) 선택 타일
	HighlightTile(SelectedUnit->GetGridCoord(), EHighlightType::EHT_Selected);

	// 2) 이동 가능(비어있는 칸)
	HighlightTiles(MovableTileSet, EHighlightType::EHT_Movable);

	// 3) 현재 위치 기준 공격 가능(적이 있는 칸)
	HighlightTiles(AttackableTileSet, EHighlightType::EHT_Attackable);

	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[HL] UpdateHighLights: Done"));
}

void ABoard::HandleTileClicked(ATileActor* Tile)
{
	if (!IsValid(Tile)) return;

	// 클릭한 타일의 좌표
	const FIntPoint ClickGridCoordinate = Tile->GetGridCoord();
	if (!IsValidCoord(ClickGridCoordinate)) return;


	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("OnTileClicked: %s (%s) State=%d"),*Pt(ClickGridCoordinate), *GetNameSafe(Tile), (int32)BoardState);
	
	// 이전 타일 선택 해제
	if (SelectedTile && IsValid(SelectedTile))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Verbose, TEXT("Unselect tile %s"), *GetNameSafe(SelectedTile));
		SelectedTile->Highlight(EHighlightType::EHT_None);
	}

	// 클릭한 타일 셀렉트로 하이라이트
	SelectedTile = Tile;
	SelectedTile->Highlight(EHighlightType::EHT_Selected);
	// 클릭한 타일의 유닛이 있으면...
	const int32	ClickGridIndex	= ToIndex(ClickGridCoordinate);
	APawnBase*	ClickedUnit		= PawnGrid.IsValidIndex(ClickGridIndex) ? PawnGrid[ClickGridIndex].Get() : nullptr;

	APlayerController* PC		= UGameplayStatics::GetPlayerController(this, 0);

	const ETeam UnitTeam	= UTeamUtils::GetActorTeam(ClickedUnit);
	const ETeam PCTeam		= UTeamUtils::GetActorTeam(PC);
	UE_LOG(LogBoard, Warning, TEXT("TeamCheck Unit=%d PC=%d Same=%d"), (int32)UnitTeam, (int32)PCTeam, UTeamUtils::AreSameTeam(ClickedUnit, PC));

	switch (BoardState)
	{
	case EBoardState::EBS_Idle: 			// 1) 아무 것도 선택 안 된 상태 -> 아군 유닛이면 선택
	{
		if (IsValid(ClickedUnit) && PC && UTeamUtils::AreSameTeam(ClickedUnit, PC))
		{
			EnterUnitSelected(ClickedUnit);
		}
		break;
	}
	case EBoardState::EBS_UnitSelected:
	{
		if (!SelectedUnit) { EnterIdle(); return; }

		const FIntPoint CurrentGrid = SelectedUnit->GetGridCoord();

		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("State=UnitSelected: Cur=%s Click=%s"), *Pt(CurrentGrid), *Pt(ClickGridCoordinate));

		// 같은 유닛 클릭 -> 클릭 해제
		if (ClickGridCoordinate == CurrentGrid) { if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT(" -> Deselect"));  EnterIdle(); return; }

		// 다른 아군 선택
		if (IsValid(ClickedUnit) && PC && UTeamUtils::AreSameTeam(ClickedUnit, PC))
		{
			if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT(" -> Switch to other ally"));
			EnterUnitSelected(ClickedUnit);
			return;
		}

		// 빈 칸 이동
		if (!ClickedUnit && MovableTileSet.Contains(ClickGridCoordinate))
		{
			if (bDebugBoardLogs)
			{
				UE_LOG(LogBoard, Warning, TEXT("[CLICK] MoveRequest: Sel=%s Cur=%s -> Click=%s  HasAuth=%d"),
					*Obj(SelectedUnit),
					*Pt(SelectedUnit ? SelectedUnit->GetGridCoord() : FIntPoint(-999, -999)),
					*Pt(ClickGridCoordinate),
					HasAuthority());
			}
			if (HasAuthority())
			{
				const bool bMoved = TryMoveUnit(SelectedUnit, ClickGridCoordinate);
				if (bMoved)
				{
					if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT(" -> TryMove to %s"), *Pt(ClickGridCoordinate));

					if (SelectedUnit)
					{
						if (const AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>())
						{
							if (UTBGGamesState->HasAPForActor(SelectedUnit, 1)) { EnterUnitSelected(SelectedUnit); }
							else { EnterIdle(); }
						}
						else { EnterIdle(); }
					}
				}
			}
			else
			{
				if (auto MyPC = Cast<AUTBGPlayerController>(PC))
				{
					UE_LOG(LogBoard, Warning, TEXT("[CLICK] MoveRequest(Client) -> RPC Server_TryMove Board=%s Unit=%s To=%s"),
						*Obj(this), *Obj(SelectedUnit), *Pt(ClickGridCoordinate));
					MyPC->ServerTryMove(this, SelectedUnit, ClickGridCoordinate);
				}
				else
				{
					UE_LOG(LogBoard, Error, TEXT("[CLICK] FAIL: Client PC cast failed -> cannot send RPC"));
				}
			}
			return;
		}

		// 적 공격
		if (IsValid(ClickedUnit) && UTeamUtils::AreEnemyTeam(ClickedUnit, SelectedUnit) && AttackableTileSet.Contains(ClickGridCoordinate))
		{
			if (HasAuthority())
			{
				const bool bDidAttack = TryAttackUnit(SelectedUnit, ClickedUnit);
				UE_LOG(LogBoard, Warning, TEXT("[CLICK] AttackRequest(Server) -> TryAttackUnit=%d"), bDidAttack);
				if (bDidAttack)
				{
					if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT(" -> TryAttack %s"), *GetNameSafe(ClickedUnit));

					if (SelectedUnit)
					{
						if (const AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>())
						{
							if (UTBGGamesState->IsActorTurn(SelectedUnit) && UTBGGamesState->HasAPForActor(SelectedUnit, 1))
								EnterUnitSelected(SelectedUnit);
							else
								EnterIdle();
						}
						else { EnterIdle(); }
					}
					else { EnterIdle(); }
				}
			}
			else
			{
				if (auto MyPC = Cast<AUTBGPlayerController>(PC))
				{
					UE_LOG(LogBoard, Warning, TEXT("[CLICK] AttackRequest(Client) -> RPC Server_TryAttack Board=%s Attacker=%s Target=%s"),
						*Obj(this), *Obj(SelectedUnit), *Obj(ClickedUnit));
					MyPC->ServerTryAttack(this, SelectedUnit, ClickedUnit);
				}
				else
				{
					UE_LOG(LogBoard, Error, TEXT("[CLICK] FAIL: Client PC cast failed -> cannot send RPC (Attack)"));
				}
			}
			return;
		}

		break;
		}
	}
}

int32 ABoard::GetRemainingAP() const
{
	if (const AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>())
	{
		return UTBGGamesState->GetCurrentAP();
	}
	return -1;
}

void ABoard::EnterIdle()
{
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("EnterIdle"));

	BoardState = EBoardState::EBS_Idle;
	SelectedUnit = nullptr;
	ClearAllHighlights();

	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[EVT] Broadcast OnUnitSelected(NULL)"));
	OnUnitSelected.Broadcast(nullptr);
}

void ABoard::EnterUnitSelected(APawnBase* Unit)
{
	if (bDebugBoardLogs)
	{
		UE_LOG(LogBoard, Warning, TEXT("[HL] EnterUnitSelected: %s StateWas=%d"),
			*Obj(Unit), (int32)BoardState);
	}
	BoardState = EBoardState::EBS_UnitSelected;
	SelectedUnit = Unit;

	MovableTileSet.Reset();
	AttackableTileSet.Reset();

	ComputeMovables(Unit, MovableTileSet);
	ComputeAttackables(Unit, AttackableTileSet);
	
	if (bDebugBoardLogs)
	{
		UE_LOG(LogBoard, Warning, TEXT("[HL]   Movable=%d %s"),
			MovableTileSet.Num(), *JoinPoints(MovableTileSet));
		UE_LOG(LogBoard, Warning, TEXT("[HL]   Attackable=%d %s"),
			AttackableTileSet.Num(), *JoinPoints(AttackableTileSet));
		UE_LOG(LogBoard, Warning, TEXT("[HL] -> UpdateHighLights()"));
	}

	UpdateHighLights();
	if (bDebugBoardLogs)
	{
		UE_LOG(LogBoard, Warning, TEXT("[EVT] Broadcast OnUnitSelected(%s)"), *Obj(Unit));
	}
	OnUnitSelected.Broadcast(Unit);
}

void ABoard::ComputeAttackables(APawnBase* Unit, TSet<FIntPoint>& Out) const
{
	Out.Reset();
	if (!Unit) return;
	const FIntPoint CurrentCoord	= Unit->GetGridCoord();
	const int32		Range			= Unit->AttackRange;

	TQueue<TPair<FIntPoint, int32>> Q;
	TSet<FIntPoint> Visited;
	
	Q.Enqueue({ CurrentCoord, 0 });
	Visited.Add(CurrentCoord);

	const FIntPoint Directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

	while (!Q.IsEmpty())
	{
		TPair<FIntPoint, int32> Cur; Q.Dequeue(Cur);
		const FIntPoint			Coord = Cur.Key;
		const int32				Cost = Cur.Value;

		for (const FIntPoint& Dir : Directions)
		{
			FIntPoint Next(Coord.X + Dir.X, Coord.Y + Dir.Y);

			if (!IsValidCoord(Next) || Visited.Contains(Next))	continue;
			if (Cost + 1 > Range)								continue;

			Visited.Add(Next);

			const int32 index = ToIndex(Next);
			if (!PawnGrid.IsValidIndex(index)) continue;
			APawnBase* PawnAtNext = PawnGrid[index].Get();
			if (PawnAtNext)
			{
				// 유닛이 있는 타일
				if (UTeamUtils::AreEnemyTeam(PawnAtNext, Unit))
				{
					Out.Add(Next); // 적이면 공격 가능
				}
				// 유닛이 있으면 시야 차단 - 더 이상 진행하지 않음
			}
			else
			{
				// 빈 타일이면 계속 진행
				Q.Enqueue({ Next, Cost + 1 });
			}
		}
	}
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("ComputeAttackables: Result=%d"), Out.Num());
}

void ABoard::ComputeMovables(APawnBase* Unit, TSet<FIntPoint>& Out)
{
	Out.Reset();
	if (!Unit) return;
	const FIntPoint CurrentCoord	= Unit->GetGridCoord();
	const int32		Range			= Unit->MoveRange;

	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("ComputeMovables: Start=%s Range=%d"),
		*Pt(CurrentCoord), Range);

	TQueue<TPair<FIntPoint, int32>> Q;
	TSet<FIntPoint> Visited;
	
	Q.Enqueue({ CurrentCoord, 0 });
	Visited.Add(CurrentCoord);

	const FIntPoint Directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

	while (!Q.IsEmpty())
	{
		TPair<FIntPoint, int32> Cur; Q.Dequeue(Cur);
		const FIntPoint			Coord	= Cur.Key;
		const int32				Cost	= Cur.Value;

		for (const FIntPoint& Dir : Directions)
		{
			FIntPoint Next(Coord.X + Dir.X, Coord.Y + Dir.Y);
			if (!IsValidCoord(Next) || Visited.Contains(Next))	continue;
			if (Cost + 1 > Range)								continue;

			const int32 index = ToIndex(Next);
			if (!PawnGrid.IsValidIndex(index)) continue;
			if (PawnGrid[index] == nullptr)
			{
				Out.Add(Next);
				Visited.Add(Next);
				Q.Enqueue({ Next, Cost + 1 });
			}
		}
	}
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("ComputeMovables: Result=%d"), Out.Num());
}

bool ABoard::TryMoveUnit(APawnBase* Unit, const FIntPoint& Target)
{
	if (!IsValid(Unit) || !IsValidCoord(Target)) return false;
	
	AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>();
	if (!UTBGGamesState) return false;

	if (!UTBGGamesState->IsActorTurn(Unit))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[MOVE] FAIL: Not your team turn"));
		return false;
	}

	const FIntPoint From = Unit->GetGridCoord();
	const int32 dist = FMath::Abs(From.X - Target.X) + FMath::Abs(From.Y - Target.Y);
	const int32 cost = FMath::Max(1, dist);			// *Unit->MoveCostPerTile 로 유닛에 맞게 이동거리 변경 가능

	const int32 OldIndex = ToIndex(From);
	const int32 NewIndex = ToIndex(Target);
	if (!PawnGrid.IsValidIndex(OldIndex) || !PawnGrid.IsValidIndex(NewIndex))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[MOVE] FAIL: OOB idx old=%d new=%d"), OldIndex, NewIndex);
		return false;
	}
	if (PawnGrid[NewIndex] != nullptr)
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[MOVE] FAIL: Target occupied"));
		return false;
	}

	if (!UTBGGamesState->TrySpendAPForActor(Unit, cost))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[MOVE] FAIL: Cannot spend AP cost=%d"), cost);
		return false;
	}
	
	// 유닛의 내부 좌표/월드 위치 갱신 (하이라이트/재계산의 기준이 되는 값)
	PawnGrid[OldIndex] = nullptr;
	PawnGrid[NewIndex] = Unit;
	Unit->SetGridCoord(Target);
	Unit->SetActorLocation(GridToWorld(Target) + Unit->PawnOffset);

	if (bDebugBoardLogs)
	{
		const int32 DebugAP = GetRemainingAP();
		UE_LOG(LogBoard, Warning, TEXT("[MOVE] OK: %s %s -> %s | AP now=%d"),
			*Obj(Unit), *Pt(From), *Pt(Target), DebugAP);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(/*Key*/-1, 1.5f, FColor::Cyan,
				FString::Printf(TEXT("[AP] After Move: %d"), DebugAP));
		}
	}

	if (HasAuthority()) MulticastUnitMoved(Unit, From, Target);
	return true;
}

bool ABoard::TryAttackUnit(APawnBase* Attacker, APawnBase* Target)
{
	if (!IsValid(Attacker) || !IsValid(Target)) return false;
	AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>();
	if (!UTBGGamesState) return false;

	if (!UTBGGamesState->IsActorTurn(Attacker))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[ATTACK] FAIL: Not your team turn"));
		return false;
	}
	
	const int32 Cost = Attacker->AttackCost;
	if (!UTBGGamesState->TrySpendAPForActor(Attacker, Cost))
	{
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("[ATTACK] FAIL: Cannot spend AP cost=%d"), Cost);
		return false;
	}

	UGameplayStatics::ApplyDamage(Target, Attacker->Damage, nullptr, Attacker, UDamageType::StaticClass());

	if (bDebugBoardLogs)
	{
		const int32 DebugAP = GetRemainingAP();
		UE_LOG(LogBoard, Warning, TEXT("[ATTACK] OK: %s -> %s | AP now=%d"),
			*Obj(Attacker), *Obj(Target), DebugAP);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(/*Key*/-1, 1.5f, FColor::Orange,
				FString::Printf(TEXT("[AP] After Attack: %d"), DebugAP));
		}
	}

	if (Target->IsDead())
	{
		const int32 Index = ToIndex(Target->GetGridCoord());
		if (PawnGrid.IsValidIndex(Index)) PawnGrid[Index] = nullptr;
		if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("  Target died, cleared grid idx=%d"), Index);
	/*	Target->Destroy();*/
	}
	return true;
}

void ABoard::EndTurn()
{
	if (bDebugBoardLogs) UE_LOG(LogBoard, Warning, TEXT("EndTurn: Request"));

	if (HasAuthority())
	{
		if (AUTBGGameState* UTBGGamesState = GetWorld()->GetGameState<AUTBGGameState>())
		{
			UTBGGamesState->EndTurn();
		}
	}

	// 공통 시각 상태 정리
	SelectedTile = nullptr;
	SelectedUnit = nullptr;
	BoardState = EBoardState::EBS_Idle;
	ClearAllHighlights();

	if (bDebugBoardLogs)
		UE_LOG(LogBoard, Warning, TEXT("[EVT] Broadcast OnUnitSelected(NULL) at EndTurn"));
	OnUnitSelected.Broadcast(nullptr);
}
