#include <fstream>
int main()
{
    std::ofstream("restart-proof.txt") << "restarted\n";
    return 0;
}
