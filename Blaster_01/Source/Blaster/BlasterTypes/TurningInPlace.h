#pragma once
//希望创建的枚举是一个范围枚举，所以是一个枚举类
UENUM(BlueprintType)
enum class ETurningInPlace:uint8
{
	ETIP_Left UMETA(DisplayName = "Turning Left"),
	ETIP_Right UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMAX")
	
};
