// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health,float MaxHealth);
	void SetHUDShield(float Shield,float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime();//Synced with server world clock
	virtual void ReceivedPlayer() override;//Sync with server clock as soon as possible

	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();
	
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();//这个存在的意义是去检测characteroverlay是否还是空指针

	/*Syn time between client and server */

	//请求当前服务器时间，当请求被发送的时候传递客户端的时间
	//requests the current server time,passing in the client's time when the request was sent
	UFUNCTION(Server,Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client,Reliable)
	void ClientReportServerTime(float TimeOfClientRequest,float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;//difference between client and server time

	//每5秒同步一次
	UPROPERTY(EditAnywhere,Category=Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaSeconds);

	UFUNCTION(Server,Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client,Reliable)
	void ClientJoinMidgame(FName StateOfMatch,float Warmup,float Match,float Cooldown,float StartingTime);
	
private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float LevelStartingTime = 0.f;
	//这些时间都不应该在这里设定，这里只做初始化，设定要在gamemode里面
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();
	
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenade = false;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;
};
