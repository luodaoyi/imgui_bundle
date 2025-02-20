# Part of ImGui Bundle - MIT License - Copyright (c) 2022-2023 Pascal Thomet - https://github.com/pthom/imgui_bundle
import os

import litgen


THIS_DIR = os.path.dirname(__file__)
PYDEF_DIR = THIS_DIR
STUB_DIR = THIS_DIR + "/../../../bindings/imgui_bundle/"

CPP_HEADERS_DIR = THIS_DIR + "/../imoguizmo"


def main():
    print("autogenerate_imoguizmo")
    input_cpp_header = CPP_HEADERS_DIR + "/imoguizmo.hpp"
    output_cpp_pydef_file = PYDEF_DIR + "/pybind_imoguizmo.cpp"
    output_stub_pyi_file = STUB_DIR + "/imoguizmo.pyi"

    # Configure options
    options = litgen.LitgenOptions()
    options.use_nanobind()
    options.fn_params_type_replacements.add_replacements([(r"\bImVec2\b", "ImVec2Like"), (r"\bImVec4\b", "ImVec4Like")])

    options.namespaces_root = ["ImOGuizmo"]
    options.fn_params_output_modifiable_immutable_to_return__regex = r".*"
    options.python_run_black_formatter = True

    litgen.write_generated_code_for_file(
        options,
        input_cpp_header_file=input_cpp_header,
        output_cpp_pydef_file=output_cpp_pydef_file,
        output_stub_pyi_file=output_stub_pyi_file,
        omit_boxed_types=True,
    )


if __name__ == "__main__":
    main()
