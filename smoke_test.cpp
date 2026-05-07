// Minimal smoke test: prove that an installed gempba is found via
// find_package(gempba), its headers compile, and a symbol that exists in both
// MT and MP builds links cleanly. Replaced by the real example tree once
// gempba#85 lands (gempba#86).
#include <gempba/gempba.hpp>

int main() {
    return gempba::shutdown();
}
