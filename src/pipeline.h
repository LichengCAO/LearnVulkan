#pragma once
#include "trig_app.h"

struct PipelineInput
{
	
};

class GraphicsPipeline
{
private:

public:
	VkPipelineLayout vkPipelineLayout;

	GraphicsPipeline();
	void Init();
	void Do(const PipelineInput& input);
};