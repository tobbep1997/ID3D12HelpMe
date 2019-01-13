#include "DirectX12EnginePCH.h"
#include "SSAOPass.h"
#include "WrapperFunctions/RenderingHelpClass.h"
#include "WrapperFunctions/X12RenderTargetView.h"
#include "WrapperFunctions/X12DepthStencil.h"
#include "WrapperFunctions/X12ConstantBuffer.h"
#include "DeferredRender.h"

SSAOPass::SSAOPass(RenderingManager* renderingManager, const Window& window)
	: IRender(renderingManager, window)
{
	m_vertexList[0] = Vertex(DirectX::XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(0, 1, 0, 0));
	m_vertexList[1] = Vertex(DirectX::XMFLOAT4(-1.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(0, 0, 0, 0));
	m_vertexList[2] = Vertex(DirectX::XMFLOAT4(1.0f, -1.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1, 1, 0, 0));
	m_vertexList[3] = Vertex(DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1, 0, 0, 0));



}

SSAOPass::~SSAOPass()
{

}

HRESULT SSAOPass::Init()
{
	HRESULT hr = 0;
	if (SUCCEEDED(hr = p_createCommandList()))
	{
		if (SUCCEEDED(hr = _preInit()))
		{
			if (SUCCEEDED(hr = p_renderingManager->SignalGPU(p_commandList)))
			{
				m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
				m_vertexBufferView.StrideInBytes = sizeof(Vertex);
				m_vertexBufferView.SizeInBytes = m_vertexBufferSize;
			}
		}	
	}
	
	return hr;
}

void SSAOPass::Update(const Camera& camera, const float& deltaTime)
{

	m_cameraValues.ViewProjection = camera.GetViewProjectionMatrix();
	m_cameraBuffer->Copy(&m_cameraValues, sizeof(m_cameraValues));

	OpenCommandList();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_renderTarget->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
		*p_renderingManager->GetFrameIndex(),
		m_renderTarget->GetDescriptorSize());

	m_renderTarget->Clear(rtvHandle);

	p_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	p_commandList->SetPipelineState(m_pipelineState);
	p_commandList->SetGraphicsRootSignature(m_rootSignature);
	p_commandList->RSSetViewports(1, &m_viewport);
	p_commandList->RSSetScissorRects(1, &m_rect);
	p_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D12DescriptorHeap* worldDescriptorHeaps[] = { m_worldPos->GetTextureDescriptorHeap() };
	p_commandList->SetDescriptorHeaps(_countof(worldDescriptorHeaps), worldDescriptorHeaps);
	p_commandList->SetGraphicsRootDescriptorTable(0, m_worldPos->GetTextureDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* depthDescriptorHeaps[] = { m_depthStencils->GetTextureDescriptorHeap() };
	p_commandList->SetDescriptorHeaps(_countof(depthDescriptorHeaps), depthDescriptorHeaps);
	p_commandList->SetGraphicsRootDescriptorTable(1, m_depthStencils->GetTextureDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

	m_cameraBuffer->SetGraphicsRootConstantBufferView(2);
}

void SSAOPass::Draw()
{
	p_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	p_commandList->DrawInstanced(4, 1, 0, 0);

	p_renderingManager->GetDeferredRender()->SetSSAO(m_renderTarget);

	ExecuteCommandList();
}

void SSAOPass::Clear()
{
	p_drawQueue->clear();
	p_instanceGroups->clear();
}

void SSAOPass::Release()
{
	m_cameraBuffer->Release();
	SAFE_DELETE(m_cameraBuffer);

	m_renderTarget->Release();
	SAFE_DELETE(m_renderTarget);

	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_vertexHeapBuffer);
	SAFE_RELEASE(m_pipelineState);
	SAFE_RELEASE(m_rootSignature);

	p_releaseCommandList();
}

void SSAOPass::SetWorldPos(X12RenderTargetView* renderTargetView)
{
	this->m_worldPos = renderTargetView;
}

void SSAOPass::SetDepthStencil(X12DepthStencil* depthStencil)
{
	m_depthStencils = depthStencil;
}

HRESULT SSAOPass::_preInit()
{
	HRESULT hr = 0;

	_createViewport();
	if (SUCCEEDED(hr = OpenCommandList()))
	{
		if (SUCCEEDED(hr = _initID3D12RootSignature()))
		{
			if (SUCCEEDED(hr = _initShaders()))
			{
				if (SUCCEEDED(hr = _initID3D12PipelineState()))
				{
					if (SUCCEEDED(hr = _createQuadBuffer()))
					{
						SAFE_NEW(m_renderTarget, new X12RenderTargetView(p_renderingManager, *p_window, p_commandList))
						if (SUCCEEDED(hr = m_renderTarget->CreateRenderTarget(
							0,
							0,
							1,
							TRUE, 
							DXGI_FORMAT_R32_FLOAT)))
						{
							SAFE_NEW(m_cameraBuffer, new X12ConstantBuffer(p_renderingManager, *p_window, p_commandList));
							if (SUCCEEDED(hr = m_cameraBuffer->CreateBuffer(L"SSAO camera", &m_cameraValues, sizeof(CameraBuffer))))
							{
								
							}
						}
					}
				}
			}
		}
	}

	if (FAILED(hr))
		Release();
	return hr;
}

HRESULT SSAOPass::_initID3D12RootSignature()
{
	HRESULT hr = 0;

	D3D12_DESCRIPTOR_RANGE worldRangeTable;
	D3D12_ROOT_DESCRIPTOR_TABLE worldTable;
	RenderingHelpClass::CreateRootDescriptorTable(worldRangeTable, worldTable, 0);

	D3D12_DESCRIPTOR_RANGE depthRangeTable;
	D3D12_ROOT_DESCRIPTOR_TABLE depthTable;
	RenderingHelpClass::CreateRootDescriptorTable(depthRangeTable, depthTable, 1);

	D3D12_ROOT_DESCRIPTOR cameraDescriptor{};
	cameraDescriptor.RegisterSpace = 0;
	cameraDescriptor.ShaderRegister = 0;

	m_rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_rootParameters[0].DescriptorTable = worldTable;
	m_rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_rootParameters[1].DescriptorTable = depthTable;
	m_rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[2].Descriptor = cameraDescriptor;
	m_rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	RenderingHelpClass::CreateSampler(sampler, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC compSampler{};
	RenderingHelpClass::CreateSampler(compSampler, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0, 0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL);

	D3D12_STATIC_SAMPLER_DESC samplers[] = { sampler, compSampler };

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(m_rootParameters),
		m_rootParameters,
		_countof(samplers),
		samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ID3DBlob * signature = nullptr;
	if (SUCCEEDED(hr = D3D12SerializeRootSignature(&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		nullptr)))
	{
		if (FAILED(hr = p_renderingManager->GetDevice()->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature))))
		{
			SAFE_RELEASE(m_rootSignature);
		}
	}
	SAFE_RELEASE(signature);
	return hr;
}

HRESULT SSAOPass::_initShaders()
{
	HRESULT hr = 0;
	ID3DBlob * blob = nullptr;

	if (FAILED(hr = ShaderCreator::CreateShader(L"../DirectX12Engine/DirectX/Shaders/SSAOPass/DefaultSSAOVertex.hlsl", blob, "vs_5_1")))
	{
		return hr;
	}
	else
	{
		m_vertexShader.BytecodeLength = blob->GetBufferSize();
		m_vertexShader.pShaderBytecode = blob->GetBufferPointer();
	}

	if (FAILED(hr = ShaderCreator::CreateShader(L"../DirectX12Engine/DirectX/Shaders/SSAOPass/DefaultSSAOPixel.hlsl", blob, "ps_5_1")))
	{
		return hr;
	}
	else
	{
		m_pixelShader.BytecodeLength = blob->GetBufferSize();
		m_pixelShader.pShaderBytecode = blob->GetBufferPointer();
	}

	return hr;
}

HRESULT SSAOPass::_initID3D12PipelineState()
{
	HRESULT hr = 0;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	m_inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	m_inputLayoutDesc.pInputElementDescs = inputLayout;


	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
	graphicsPipelineStateDesc.InputLayout = m_inputLayoutDesc;
	graphicsPipelineStateDesc.pRootSignature = m_rootSignature;
	graphicsPipelineStateDesc.VS = m_vertexShader;
	graphicsPipelineStateDesc.PS = m_pixelShader;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
	graphicsPipelineStateDesc.SampleMask = 0xffffffff;
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	DXGI_SWAP_CHAIN_DESC desc;
	if (SUCCEEDED(hr = p_renderingManager->GetSwapChain()->GetDesc(&desc)))
	{
		graphicsPipelineStateDesc.SampleDesc = desc.SampleDesc;
	}
	else
		return hr;

	if (FAILED(hr = p_renderingManager->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&m_pipelineState))))
	{
		SAFE_RELEASE(m_pipelineState);
	}

	return hr;
}

void SSAOPass::_createViewport()
{
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<FLOAT>(p_window->GetWidth());
	m_viewport.Height = static_cast<FLOAT>(p_window->GetHeight());
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_rect.left = 0;
	m_rect.top = 0;
	m_rect.right = p_window->GetWidth();
	m_rect.bottom = p_window->GetHeight();
}

HRESULT SSAOPass::_createQuadBuffer()
{
	HRESULT hr = 0;

	m_vertexBufferSize = sizeof(m_vertexList);

	if (SUCCEEDED(hr = p_renderingManager->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer))))
	{

		if (SUCCEEDED(hr = p_renderingManager->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexHeapBuffer))))
		{

			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = reinterpret_cast<void*>(m_vertexList);
			vertexData.RowPitch = m_vertexBufferSize;
			vertexData.SlicePitch = m_vertexBufferSize;

			UpdateSubresources(p_commandList, m_vertexBuffer, m_vertexHeapBuffer, 0, 0, 1, &vertexData);

			p_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		}
	}
	return hr;
}