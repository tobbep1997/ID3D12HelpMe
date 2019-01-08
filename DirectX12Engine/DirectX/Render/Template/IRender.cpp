#include  "DirectX12EnginePCH.h"
#include "IRender.h"
#include "DirectX/Render/WrapperFunctions/Functions/Instancing.h"

IRender::IRender(RenderingManager* renderingManager,
                 const Window& window)
{
	this->p_renderingManager = renderingManager;
	this->p_window = &window;
	p_drawQueue = new std::vector<Drawable*>();
	p_lightQueue = new std::vector<ILight*>();
	p_instanceGroups = new std::vector<Instancing::InstanceGroup>();
}

IRender::~IRender()
{
	SAFE_DELETE(p_lightQueue);
	SAFE_DELETE(p_drawQueue);
	SAFE_DELETE(p_instanceGroups);
}

void IRender::Queue(Drawable* drawable) const
{
	p_drawQueue->push_back(drawable);
	Instancing::AddInstance(p_instanceGroups, drawable);
}

void IRender::QueueLight(ILight* light) const
{
	p_lightQueue->push_back(light);
}

HRESULT IRender::p_createCommandList()
{
	HRESULT hr = 0;

	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		if (FAILED(hr = p_renderingManager->GetDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, 
			IID_PPV_ARGS(&p_commandAllocator[i]))))
		{
			return hr;
		}
	}
	if (SUCCEEDED(hr = p_renderingManager->GetDevice()->CreateCommandList(
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		p_commandAllocator[0], 
		nullptr, 
		IID_PPV_ARGS(&p_commandList))))
	{		
		p_commandList->Close();
	}
	return hr;
}

void IRender::p_releaseCommandList()
{
	SAFE_RELEASE(p_commandList);
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		SAFE_RELEASE(p_commandAllocator[i]);
	}
}

HRESULT IRender::p_createInstanceBuffer(const UINT & bufferSize)
{
	HRESULT hr = 0;
	
	if (SUCCEEDED(hr = p_renderingManager->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		nullptr,
		IID_PPV_ARGS(&p_instanceBuffer))))
	{
		if (SUCCEEDED(hr = p_renderingManager->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&p_intermediateInstanceBuffer))))
		{

		}
	}
	return hr;
}

BOOL IRender::p_updateInstanceBuffer(const size_t& index, D3D12_VERTEX_BUFFER_VIEW & vertexBufferView) const
{
	return static_cast<BOOL>(Instancing::UpdateInstanceGroup(p_commandList,
		vertexBufferView,
		p_instanceBuffer,
		p_intermediateInstanceBuffer,
		&p_instanceGroups->at(index)));
}

void IRender::p_drawInstance(const UINT & textureStartIndex, const BOOL& mapTextures)
{
	ID3D12GraphicsCommandList * gcl = p_commandList ? p_commandList : p_renderingManager->GetCommandList();

	const size_t instanceGroupSize = p_instanceGroups->size();
	for (size_t i = 0; i < instanceGroupSize; i++)
	{		
		D3D12_VERTEX_BUFFER_VIEW instanceBufferView = {};
		if (!p_updateInstanceBuffer(i, instanceBufferView))
			throw "FAILED TO UPDATE INSTANCE BUFFER";
		if (mapTextures)
			p_instanceGroups->at(i).MapTextures(textureStartIndex, p_commandList);

		D3D12_VERTEX_BUFFER_VIEW bufferArr[2] = 
			{ 
				p_instanceGroups->at(i).StaticMesh->GetVertexBufferView(),
				instanceBufferView
			};

		gcl->IASetVertexBuffers(0, 2, bufferArr);

		gcl->DrawInstanced(
			p_instanceGroups->at(i).StaticMesh->GetStaticMesh().size(),
			p_instanceGroups->at(i).GetSize(),
			0,
			0);

	}
}

void IRender::p_releaseInstanceBuffer()
{
	Instancing::ClearInstanceGroup(p_instanceGroups);
	SAFE_RELEASE(p_instanceBuffer);
	SAFE_RELEASE(p_intermediateInstanceBuffer);
}

HRESULT IRender::OpenCommandList()
{
	HRESULT hr = 0;
	const UINT frameIndex = *p_renderingManager->GetFrameIndex();
	if (SUCCEEDED(hr = this->p_commandAllocator[frameIndex]->Reset()))
	{
		if (SUCCEEDED(hr = this->p_commandList->Reset(this->p_commandAllocator[frameIndex], nullptr)))
		{

		}
	}
	return hr;
}

HRESULT IRender::ExecuteCommandList() const
{
	HRESULT hr = 0;
	if (SUCCEEDED(hr = p_commandList->Close()))
	{
		ID3D12CommandList* ppCommandLists[] = { p_commandList };
		p_renderingManager->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}
	return hr;
}

