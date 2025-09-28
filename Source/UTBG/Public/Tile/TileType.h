#pragma once
#include "TileType.generated.h"

UENUM(BlueprintType)
enum class EHighlightType : uint8
{
	EHT_None			UMETA(DisplayName = "None"),
	EHT_Movable		UMETA(DisplayName = "Move"),
	EHT_Attackable		UMETA(DisplayName = "Attack"),
	EHT_Selected		UMETA(DisplayName = "Selected"),

	EHT_MAX				UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EBoardState : uint8 
{ 
	EBS_Idle			UMETA(DisplayName = "Idle"),
	EBS_UnitSelected	UMETA(DisplayName = "UnitSelected"),

	EBS_MAX				UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class ETeamtoChange : uint8 { 
	ET_TeamA			UMETA(DisplayName = "TeamA"),
	ET_TeamB			UMETA(DisplayName = "TeamB"),

	ET_MAX				UMETA(DisplayName = "DefaultMAX")
};