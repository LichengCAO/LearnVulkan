#pragma once
#include "trig_app.h"

struct PipelineInput
{

};

class GraphicsPipeline
{
public:
	GraphicsPipeline();
	void Init();
	void Do(const PipelineInput& input);
};