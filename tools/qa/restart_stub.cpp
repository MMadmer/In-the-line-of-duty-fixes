#include <Windows.h>

#include <fstream>
#include <string>

// Records the raw restart command line so QA can compare it byte for byte with the launcher's tail.
int main()
{
    const std::wstring line = GetCommandLineW();
    std::string narrow;
    narrow.reserve(line.size());
    for (const auto character : line) narrow.push_back(static_cast<char>(character));
    std::ofstream("restart-proof.txt") << "restarted\n" << narrow << '\n';
    return 0;
}
