#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"

UCLASS(MinimalAPI)
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContexObject"))
	static void DrawTestShaderRenderTarget(const UObject* WorldContextObject, class UTextureRenderTarget2D* OutputRenderTarget, FLinearColor MyColor,UTexture2D* MyTexture);
};