#include "GLRenderer.h"

void BGLRenderer::Acquire()
{
	atomic_add(&fRefCount, 1);
}

void BGLRenderer::Release()
{
	if (atomic_add(&fRefCount, -1) < 1)
		delete this;
}

void BGLRenderer::LockGL()
{
}

void BGLRenderer::UnlockGL()
{
}

void BGLRenderer::DirectConnected(direct_buffer_info *info)
{
}

void BGLRenderer::EnableDirectMode(bool enabled)
{
}

