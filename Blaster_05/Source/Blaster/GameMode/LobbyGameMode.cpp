// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
//大厅需要知道有多少玩家连接进来，只要数量足够，就去到我们需要的实际游戏地图

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	//其实gamemode有一个变量叫gamestate，包含一个游戏库，里面有一个玩家数组，记录加入到游戏中的玩家状态和属性。
	Super::PostLogin(NewPlayer);

	//用一个数组记录下来玩家数目
	int32 NumberOfPlayers=GameState.Get()->PlayerArray.Num();
	if(NumberOfPlayers==2)
	{
		UWorld* World=GetWorld();
		if(World)
		{
			//非阻塞连接
			bUseSeamlessTravel=true;
			//服务器需要一个地址去travel
			World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
			
		}
	}
	




	
}
