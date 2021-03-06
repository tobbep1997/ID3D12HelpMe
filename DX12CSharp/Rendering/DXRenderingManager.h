#pragma once
#include "DXWindow.h"
#include <DirectX/RenderingManager.h>
#include "../Objects/DXCamera.h"

#include "../DirectX/DXDirectX.h"

namespace ID3D12
{
	using namespace DXWrap;
	using namespace System;
	public ref class DXRenderingManager : IManagedObject<RenderingManager>
	{
	public:
		DXRenderingManager();

		bool Init(DXWindow^ window);
		void Flush(DXCamera^ camera);
		void Release();
	};

}
