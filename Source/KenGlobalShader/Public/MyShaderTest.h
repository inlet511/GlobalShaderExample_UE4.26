#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"


USTRUCT(BlueprintType)
struct FMyShaderStructData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorOne;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorTwo;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorThree;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorFour;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	int32 ColorIndex;
};

UCLASS(MinimalAPI)
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContexObject"))
	static void DrawTestShaderRenderTarget(
		const UObject* WorldContext, 
		class UTextureRenderTarget2D* OutputRenderTarget, 
		FLinearColor MyColor,
		UTexture2D* MyTexture,
		FMyShaderStructData UniformData
	);
};