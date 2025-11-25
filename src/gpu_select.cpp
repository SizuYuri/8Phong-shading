// Просим дискретную GPU на ноутбуках с Optimus/PowerXpress
#ifdef _WIN32
extern "C" {
	// NVIDIA Optimus
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	// AMD PowerXpress
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
