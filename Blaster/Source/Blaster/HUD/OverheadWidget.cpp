// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include <Windows.Data.Text.h>

#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if(DisplayText)
	{
		//从texttodisplay当中输入字符，用来设置widget
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
	
}
//这个函数是用来找出是哪个玩家在网络里
void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	//这个E开头的是个枚举值
	ENetRole LocalRole = InPawn->GetLocalRole();
	FString Role;
	switch(LocalRole)
	{
	case ENetRole::ROLE_Authority:
		Role=FString("Authority");
		break;
	case ENetRole::ROLE_None:
		Role=FString("None");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role=FString("AutonomousProxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role=FString("Simulated Proxy");
		break;
	}
	FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"),*Role);
	SetDisplayText(LocalRoleString);


	
}

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	//这样控件就会从视口中移除
	RemoveFromParent();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
