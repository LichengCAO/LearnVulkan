#include "meshlet_app.h"
#include "transparent_app.h"
#include "ray_tracing_app.h"
#include "ray_query_app.h"
#include "shader_reflect.h"
int main() {
	//RayQueryApp app;
	//RayTracingApp app;
	//RayTracingThousandsApp app;
	//MeshletApp app;
	//app.Run(); // easier to debug this way
	//try {
	//	app.Run();
	//}
	//catch (const std::exception& e) {
	//	std::cerr << e.what() << std::endl;
	//	return EXIT_FAILURE;
	//}

	ShaderReflector reflector{};

	reflector.Init({
		"E:/GitStorage/LearnVulkan/bin/shaders/tri_mesh.vert.spv",
		"E:/GitStorage/LearnVulkan/bin/shaders/trig.frag.spv",
		"E:/GitStorage/LearnVulkan/bin/shaders/oit.frag.spv",
		});

	reflector.PrintReflectResult();

	system("pause");
	return EXIT_SUCCESS;
}