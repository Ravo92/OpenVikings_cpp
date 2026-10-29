#include "GameApplication.h"

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ LPSTR commandLine, _In_ int showCommand)
{

	return NRemaster::RunGame(NRemaster::EGameVariant::Cultures2, instance, previousInstance, commandLine, showCommand);
}