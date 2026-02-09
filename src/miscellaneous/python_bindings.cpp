#include <pybind11/pybind11.h>

#include "python_bindings.h"

namespace py = pybind11;

std::string GetArtist() {
    return "add";
}

PYBIND11_MODULE(PAW_python, m) {
    m.doc() = "PerfectAudioWorks Internal Python API";
    m.def("GetArtist", &GetArtist, "Get Current Artist");
}