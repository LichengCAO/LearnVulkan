#include "meshlet_app.h"
#include "transparent_app.h"
#include "ray_tracing_app.h"
#include "ray_tracing_reflect_app.h"
#include "ray_query_app.h"
#include "utility/vdb_loader.h"

int main() {
	//RayQueryApp app;
	//RayTracingApp app;
	//RayTracingThousandsApp app;
	RayTracingReflectApp app;
	MyVDBLoader loader{};
	//TransparentApp app;
	app.Run(); // easier to debug this way
	//try {
	//	app.Run();
	//}
	//catch (const std::exception& e) {
	//	std::cerr << e.what() << std::endl;
	//	return EXIT_FAILURE;
	//}
	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Cumulus Cloud 1.vdb");
	system("pause");
	return EXIT_SUCCESS;
}