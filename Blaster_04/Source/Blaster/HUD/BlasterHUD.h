// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	//这里是标记十字准星扩散多大
	float CrosshairSpread;
	FLinearColor CrosshairsColor;
	
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere,Category = "Player State")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	class UCharacterOverlay* CharacterOverlay;
protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture,FVector2D ViewportCenter,FVector2D Spread,FLinearColor CrosshairColor);

	//扩散的最大值
	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package)
	{
		HUDPackage = Package;
	}
	
};
