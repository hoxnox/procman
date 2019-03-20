from nxtools import NxConanFile
from conans import CMake, tools


class ProcmanSSLConan(NxConanFile):
    name = "procman"
    version = "0.0.2"
    license = "OpenBSD"
    url = "https://github.com/hoxnox/procman"
    license = "https://github.com/hoxnox/procman/blob/master/LICENSE"
    settings = "os", "compiler", "build_type", "arch"
    build_policy = "missing"
    description = "Process Manager (procman) helps to create daemons and applications that is sensible to process properties."
    options = {}

    def do_source(self):
        self.retrieve("d278336f4a902aa9e31d02854b73db3f854c5495d15f91b94cb87a153b7c06db",
                [
                    "vendor://hoxnox/procman/procman-{v}.tar.gz".format(v=self.version),
                    "https://github.com/hoxnox/procman/archive/{v}.tar.gz".format(v=self.version)
                ],
                "procman-{v}.tar.gz".format(v=self.version))

    def do_build(self):
        cmake = CMake(self)
        cmake.build_dir = "{staging_dir}/src".format(staging_dir=self.staging_dir)
        tools.untargz("procman-{v}.tar.gz".format(v=self.version), cmake.build_dir)
        cmake.configure(defs={
                "CMAKE_INSTALL_PREFIX": self.staging_dir,
                "CMAKE_INSTALL_LIBDIR": "lib"
            }, source_dir="procman-{v}".format(v=self.version))
        cmake.build(target="install")

    def do_package_info(self):
            self.cpp_info.libs = ["procman"]

