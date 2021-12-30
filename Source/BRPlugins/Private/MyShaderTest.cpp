// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.  

#include "MyShaderTest.h"  

#include "Engine/TextureRenderTarget2D.h"  
#include "Engine/World.h"  
#include "GlobalShader.h"  
#include "PipelineStateCache.h"  
#include "RHIStaticStates.h"  
#include "SceneUtils.h"  
#include "SceneInterface.h"  
#include "ShaderParameterUtils.h"  
#include "Logging/MessageLog.h"  
#include "Internationalization/Internationalization.h"  
#include "StaticBoundShaderState.h"  

#define LOCTEXT_NAMESPACE "TestShader"


class FMyShaderTest : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FMyShaderTest, NonVirtual);
public:

	FMyShaderTest() {}

	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FLinearColor& MyColor
	)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), SimpleColorVal, MyColor);
	}

private:

	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);

};

class FShaderTestVS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestVS, Global);

public:
	FShaderTestVS() {}

	FShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};


class FShaderTestPS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestPS, Global);

public:
	FShaderTestPS() {}

	FShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};

IMPLEMENT_SHADER_TYPE(, FShaderTestVS, TEXT("/BRPlugins/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FShaderTestPS, TEXT("/BRPlugins/MyShader.usf"), TEXT("MainPS"), SF_Pixel)

static void DrawTestShaderRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutputRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel,
	FName TextureRenderTargetName,
	FLinearColor MyColor
)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS  
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else  
	SCOPED_DRAW_EVENT(RHICmdList, DrawUVDisplacementToRenderTarget_RenderThread);
#endif

	//设置渲染目标  
	//SetRenderTarget(
	//	RHICmdList,
	//	OutputRenderTargetResource->GetRenderTargetTexture(),
	//	FTextureRHIRef(),
	//	ESimpleRenderTargetMode::EUninitializedColorAndDepth,
	//	FExclusiveDepthStencil::DepthNop_StencilNop
	//);



	FRHITexture2D* RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawUVDisplacement"));
	{
		//设置视口  
		//FIntPoint DrawTargetResolution(OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY());  
		//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FShaderTestVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FShaderTestPS> PixelShader(GlobalShaderMap);

		// Set the graphic pipeline state.  
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  
		PixelShader->SetParameters(RHICmdList, MyColor);

		// Draw grid.  
		//uint32 PrimitiveCount = 2;  
		//RHICmdList.DrawPrimitive(PT_TriangleList, 0, PrimitiveCount, 1);  
		FVector4 Vertices[4];
		Vertices[0].Set(-1.0f, 1.0f, 0, 1.0f);
		Vertices[1].Set(1.0f, 1.0f, 0, 1.0f);
		Vertices[2].Set(-1.0f, -1.0f, 0, 1.0f);
		Vertices[3].Set(1.0f, -1.0f, 0, 1.0f);
		static const uint16 Indices[6] =
		{
			0, 1, 2,
			2, 1, 3
		};
		//DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));  
		//DrawIndexedPrimitiveUP(
		//	RHICmdList,
		//	PT_TriangleList,
		//	0,
		//	ARRAY_COUNT(Vertices),
		//	2,
		//	Indices,
		//	sizeof(Indices[0]),
		//	Vertices,
		//	sizeof(Vertices[0])
		//);
		
		//FRHIIndexBuffer idxBuffer = FRHIIndexBuffer(3,sizeof(Indices),)
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = UE_ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(NumIndices);
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		FRHIIndexBuffer* idxBuffer = RHICreateIndexBuffer(sizeof(uint16), sizeof(Indices), BUF_Static, CreateInfo);
		RHICmdList.DrawIndexedPrimitive(idxBuffer, 0, 0, sizeof(Indices[0]), 0, 2, 2);

		//// Resolve render target.  
		//RHICmdList.CopyToResolveTarget(
		//	OutputRenderTargetResource->GetRenderTargetTexture(),
		//	OutputRenderTargetResource->TextureRHI,
		//	false, FResolveParams());
	}

	RHICmdList.EndRenderPass();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
}

void UTestShaderBlueprintLibrary::DrawTestShaderRenderTarget(
	UTextureRenderTarget2D* OutputRenderTarget,
	AActor* Ac,
	FLinearColor MyColor
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	UWorld* World = Ac->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList)
	{
		DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor);
	}
	);

}

#undef LOCTEXT_NAMESPACE  