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
	MyVDBLoader::Level1Data vdbData{};
	//TransparentApp app;
	app.Run(); // easier to debug this way
	//try {
	//	app.Run();
	//}
	//catch (const std::exception& e) {
	//	std::cerr << e.what() << std::endl;
	//	return EXIT_FAILURE;
	//}
	std::cout << "Altocumlus: " << std::endl;
	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Altocumlus.vdb");
	std::cout << "Cumulus Cloud 1: " << std::endl;
	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Cumulus Cloud 1.vdb");
	std::cout << "Cumulus Congestus: " << std::endl;
	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Cumulus Congestus.vdb");
	std::cout << "Stratocumulus 1: " << std::endl;
	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Stratocumulus 1.vdb");
	std::cout << "Try to load compact data..." << std::endl;
	loader.LoadToLevel1("E:/GitStorage/LearnVulkan/res/models/cloud/Stratocumulus 1.vdb", vdbData);
	system("pause");
	return EXIT_SUCCESS;
}