#include "analysis_report.h"

#include "results.h"

#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>

namespace console {

namespace {

constexpr double requiredUndetectedProbability = 1e-6;

std::string scientific(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::scientific << std::setprecision(3) << value;
    return stream.str();
}

void printHeader(std::ostream& output, const crc::CyclicCode& code)
{
    output << "Анализ кода (" << code.n << ", " << code.dataBitCount() << "), g(x) = " << crc::toBinary(code.generator)
           << ' ' << crc::toDegreeList(code.generator) << '\n'
           << "  n = " << code.n << ", k = " << code.dataBitCount() << ", h = " << code.checkBitCount() << '\n';
}

void printSpectrum(std::ostream& output, const crc::WeightSpectrum& spectrum)
{
    output << "\nВесовой спектр (A_w — число кодовых слов веса w, нулевые не показаны):\n";
    for (std::size_t weight = 0; weight < spectrum.size(); ++weight) {
        if (spectrum[weight] > 0)
            output << "  w = " << std::setw(2) << weight << ": " << spectrum[weight] << '\n';
    }
}

void printGuarantees(std::ostream& output, const crc::CodeAnalysis& analysis)
{
    output << "\nГарантированно обнаруживаются:\n";
    if (analysis.spectrum)
        output << "  " << crc::guaranteedErrorsText(*analysis.spectrum) << '\n';
    output << "  " << crc::guaranteedBurstsText(analysis.code) << '\n';
}

void printUndetectedProbability(std::ostream& output, const crc::WeightSpectrum& spectrum, double bitErrorProbability)
{
    const double probability = crc::undetectedProbability(spectrum, bitErrorProbability);
    output << "\nТочная вероятность ложного приёма в ДСК, Pлож = Σ A_w p^w (1-p)^(n-w):\n"
           << "  Pлож(p = " << crc::formatNumber(bitErrorProbability, 6) << ") = " << scientific(probability)
           << (probability <= requiredUndetectedProbability ? "  (требование Pлож ≤ 1e-6 выполнено)\n"
                                                            : "  (требование Pлож ≤ 1e-6 не выполнено)\n");
}

}

void printAnalysis(std::ostream& output, const crc::CodeAnalysis& analysis, double bitErrorProbability)
{
    printHeader(output, analysis.code);
    if (analysis.spectrum) {
        output << "  Минимальное кодовое расстояние d = " << crc::minimumDistance(*analysis.spectrum) << '\n';
        printSpectrum(output, *analysis.spectrum);
    }
    printGuarantees(output, analysis);
    if (analysis.spectrum)
        printUndetectedProbability(output, *analysis.spectrum, bitErrorProbability);
}

}
