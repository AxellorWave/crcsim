#include "code.h"
#include "encoder.h"
#include "params.h"

#include <bitset>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<crc::Word> generatorRows(const crc::CyclicCode& code)
{
    std::vector<crc::Word> rows;
    for (int bit = 0; bit < code.dataBitCount(); ++bit)
        rows.push_back(crc::encode(code, crc::Word{1} << bit));
    return rows;
}

std::vector<crc::Word> parityCheckRows(const crc::CyclicCode& code)
{
    const std::vector<crc::Word> generator = generatorRows(code);
    const int h = code.checkBitCount();
    std::vector<crc::Word> rows;
    for (int check = 0; check < h; ++check) {
        crc::Word row = crc::Word{1} << check;
        for (int bit = 0; bit < code.dataBitCount(); ++bit) {
            if ((generator[static_cast<std::size_t>(bit)] >> check) & 1)
                row |= crc::Word{1} << (h + bit);
        }
        rows.push_back(row);
    }
    return rows;
}

std::vector<std::uint64_t> weightDistribution(const std::vector<crc::Word>& basis, int n)
{
    std::vector<std::uint64_t> weights(static_cast<std::size_t>(n) + 1, 0);
    crc::Word word = 0;
    ++weights[0];
    const std::uint64_t combinations = std::uint64_t{1} << basis.size();
    for (std::uint64_t step = 1; step < combinations; ++step) {
        int changed = 0;
        while (((step >> changed) & 1) == 0)
            ++changed;
        word ^= basis[static_cast<std::size_t>(changed)];
        ++weights[std::bitset<64>(word).count()];
    }
    return weights;
}

}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Использование: weight_enumerator n g\n";
        return 1;
    }
    try {
        const crc::CyclicCode code = crc::makeCode({std::stoi(argv[1]), argv[2]});
        const bool enumerateCode = code.dataBitCount() <= code.checkBitCount();
        const std::vector<crc::Word> basis = enumerateCode ? generatorRows(code) : parityCheckRows(code);
        std::cout << (enumerateCode ? "code" : "dual");
        for (std::uint64_t count : weightDistribution(basis, code.n))
            std::cout << ';' << count;
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
