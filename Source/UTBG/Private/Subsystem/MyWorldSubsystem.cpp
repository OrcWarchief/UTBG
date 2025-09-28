


//#include "Subsystem/MyWorldSubsystem.h"
//
//void UMyWorldSubsystem::GenerateBoard(int32 InRows, int32 InCols)
//{
//    if (bGenerated) { UE_LOG(LogTemp, Warning, TEXT("Board already generated.")); return; }
//    if (!TileClass) { UE_LOG(LogTemp, Error, TEXT("TileClass not set!")); return; }
//
//
//    Rows = InRows;
//    Cols = InCols;
//    // 그리드 배열 크기 확보
//    TileGrid.SetNum(Rows * Cols);
//
//    // 타일 액터 스폰
//    for (int32 Y = 0; Y < Rows; ++Y)
//    {
//        for (int32 X = 0; X < Cols; ++X)
//        {
//            const FIntPoint GridCoord(X, Y);
//            const FVector  WorldPos = GridToWorld(GridCoord);
//
//            ATileActor* NewTile = GetWorld()->SpawnActor<ATileActor>(TileClass, WorldPos, FRotator::ZeroRotator);
//            if (!NewTile) { UE_LOG(LogTemp, Error, TEXT("Failed to spawn tile (%d,%d)"), X, Y); continue; }
//            NewTile->InitializeTile(GridCoord);
//            TileGrid[ToIndex(GridCoord)] = NewTile;
//        }
//    }
//    bGenerated = true;
//}
//
//void UMyWorldSubsystem::ClearBoard()
//{
//    for (ATileActor* T : TileGrid)
//    {
//        if (T) T->Destroy();  
//        TileGrid.Empty(); 
//        bGenerated = false;
//    }
//}
//
//// 격자 좌표 → 월드 좌표(타일 중심) 변환  호출 주체 : 컨트롤러, AI, 이펙트
//FVector UMyWorldSubsystem::GridToWorld(const FIntPoint& Grid) const
//{
//    return BoardOrigin + FVector(Grid.X * TileSize, Grid.Y * TileSize, 0.f);
//}
//
////  월드 좌표 → 격자 좌표 스냅            호출 주체 : 클릭 처리, Path‑finding
//FIntPoint UMyWorldSubsystem::WorldToGrid(const FVector& WorldLoc) const
//{
//    const FVector Local = WorldLoc - BoardOrigin;
//    return FIntPoint( FMath::RoundToInt(Local.X / TileSize), FMath::RoundToInt(Local.Y / TileSize) );
//}
//
//void UMyWorldSubsystem::HighlightTiles(const TArray<FIntPoint>& Tiles, EHighlightType Type)
//{
//    for (const FIntPoint& T : Tiles)
//    {
//        if (!IsValidCoord(T)) continue;
//
//        if (ATileActor* Tile = TileGrid[ToIndex(T)])
//        {
//            Tile->Highlight(Type);   // 색 결정은 서브시스템, 적용은 타일
//        }
//    }
//}
//
//void UMyWorldSubsystem::ResetAllHighlights(const TArray<FIntPoint>& Tiles)
//{
//    for (ATileActor* Tile : TileGrid)
//    {
//        if (Tile) Tile->ClearHighlight();
//    }
//}
