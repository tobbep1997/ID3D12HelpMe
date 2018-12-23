#include "DirectX12EnginePCH.h"
#include "Drawable.h"
#include "DirectX/Render/Template/IRender.h"


Drawable::Drawable()
{
	this->Init();
}


Drawable::~Drawable()
{
}

void Drawable::Init()
{
}

void Drawable::Update()
{
	Transform::Update();
}

void Drawable::Release()
{
}

void Drawable::SetMesh(StaticMesh& mesh)
{
	this->m_mesh = &mesh;
}

const StaticMesh& Drawable::GetMesh() const
{
	return *this->m_mesh;
}

void Drawable::Draw(RenderingManager * renderingManager)
{
	if (this->m_isVisible)
		reinterpret_cast<IRender*>(renderingManager->GetGeometryPass())->Queue(this);
}

void Drawable::SetIsVisible(const BOOL& visible)
{
	this->m_isVisible = visible;
}

const BOOL& Drawable::GetIsVisible() const
{
	return this->m_isVisible;
}
