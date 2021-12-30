# GlobalShaderExample_UE4.26
UE4.26 Version of GlobalShaderCreattion

因为[这篇文章](https://zhuanlan.zhihu.com/p/36635394) 已经不适用于UE4.26

且[这篇文章](https://zhuanlan.zhihu.com/p/66514192) 中提到的[Repository](https://github.com/blueroseslol/BRPlugins) 中又有过多与创建GlobalShader不相干的东西，所以我在这里参考这两篇文章及代码做了修改，使其适用于UE4.26.

# GlobalShader 4.26 变动

## 整体参考
参考引擎自带插件LensDistortion即可

	Plugins/Compositing/LensDistortion


## 细节变动

### Build.cs

PrivateDependencyModuleNames 中还要添加一个Projects，因为 IPluginManager.h现在位于Runtime/Projects目录下


### 去掉Serialize函数
彻底删除这个函数，不需要了

	virtual bool Serialize(FArchive& Ar) override  
		
### FGlobalShaderMap
FGlobalShaderMap 替代了原来的

TShareMap\<FGlobalShaderType\>
	
旧代码:

	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel); 
	
新代码:

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);

### GETSAFERHISHADER_VERTEX

这个宏已经不存在了，直接使用XXXShader.GetXXXShader()函数代替
旧代码：

	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

新代码：

	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();


### GetPixelShader

	SetShaderValue(RHICmdList, GetPixelShader(), SimpleColorVal, MyColor);
GetPixelShader()已经不是GlobalShader中的函数了，要替换为：

	RHICmdList.GetBoundPixelShader()


### SetRenderTarget

这个方法已经废弃，要用RHIBeginRenderPass和RHIEndRenderPass来代替
参考：LensDistortionRendering.cpp
旧代码：

	SetRenderTarget(
		RHICmdList,
		OutputRenderTargetResource->GetRenderTargetTexture(),
		FTextureRHIRef(),
		ESimpleRenderTargetMode::EUninitializedColorAndDepth,
		FExclusiveDepthStencil::DepthNop_StencilNop
	);
      //……
      DrawIndexedPrimitiveUP…
      RHICmdList.CopyToResolveTarget(.....


新代码：

	FRHITexture2D* RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawUVDisplacement"));
	{

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

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY(), 1.f);
		PixelShader->SetParameters(RHICmdList, MyColor);


		RHICmdList.SetStreamSource(0, GScreenSpaceVertexBuffer.VertexBufferRHI, 0);
		RHICmdList.DrawIndexedPrimitive(GTwoTrianglesIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
	}

	RHICmdList.EndRenderPass();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));

### DrawIndexedPrimitive函数修改

旧代码

	DrawIndexedPrimitiveUP(  
        RHICmdList,  
        PT_TriangleList,  
        0,  
        ARRAY_COUNT(Vertices),  
        2,  
        Indices,  
        sizeof(Indices[0]),  
        Vertices,  
        sizeof(Vertices[0])  
    ); 
	
新代码

	RHICmdList.DrawIndexedPrimitive(GTwoTrianglesIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);

现在需要在RHICmdList对象上调用

### Shader参数声明方式变化

使用Layout Field替代旧的方式

旧代码
class FMyShaderTest : public FGlobalShader的private部分：

	FShaderParameter SimpleColorVal;

新代码

FMyShaderTest类的开头：

	DECLARE_INLINE_TYPE_LAYOUT(FMyShaderTest, NonVirtual);
	
private部分：

	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
