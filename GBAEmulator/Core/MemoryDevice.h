#pragma once

class MemoryBus;

class MemoryDevice
{
public:
	virtual void HandleRequest(MemoryBus* memoryBus) = 0;
};