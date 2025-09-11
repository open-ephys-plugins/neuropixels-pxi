from pathlib import Path
from conan import ConanFile
from conan.tools.files import get, copy


class libxdaqnp(ConanFile):
    name = "libxdaqnp"
    version = "0.3.3"
    settings = "os", "compiler", "build_type", "arch"
    build_policy = "missing"

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25.0 <3.30.0]")

    def requirements(self):
        self.requires("fmt/10.2.1")
        self.requires("libxdaq/0.5.0")
        self.requires("spdlog/1.13.0")

    def configure(self):
        pass

    def build(self):
        base_url = "https://xdaq.sgp1.cdn.digitaloceanspaces.com/xdaq_npapi"

        _os = str(self.settings.os).lower()
        _arch = str(self.settings.arch).lower()
        url = f"{base_url}/{_os}-{_arch}-{self.version}.zip"
        get(self, url, strip_root=True)

    def package(self):
        local_bin_folder = Path(self.build_folder, "bin")
        local_include_folder = Path(self.build_folder, "include")
        local_lib_folder = Path(self.build_folder, "lib")
        copy(self, "*", local_bin_folder, Path(self.package_folder, "bin"))
        copy(self, "*", local_include_folder, Path(self.package_folder, "include"))
        copy(self, "*", local_lib_folder, Path(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.libs = ["xdaq_npapi"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib/xdaq"]
        self.cpp_info.builddirs.append(str(Path("lib", "cmake", "libxdaqnp")))