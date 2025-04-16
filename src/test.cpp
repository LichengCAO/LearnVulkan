#include "meshlet_app.h"
//#include "transparent_app.h"
int main() {
	MeshletApp app;
	app.Run(); // easier to debug this way
	//try {
	//	app.Run();
	//}
	//catch (const std::exception& e) {
	//	std::cerr << e.what() << std::endl;
	//	return EXIT_FAILURE;
	//}
	system("pause");
	return EXIT_SUCCESS;
}