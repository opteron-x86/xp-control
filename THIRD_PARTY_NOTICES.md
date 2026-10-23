# Third-party notices

XP Control uses GPL-3.0-or-later with the permissions in `EXCEPTIONS.md`. Dependencies retain their respective licenses.

| Dependency | Source | License |
| --- | --- | --- |
| CommonLibSSE-NG 13.1.0 | [alandtse/CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG/tree/12d61f3c39f7d4ba1b1ab543df2bca0e613dc747) | GPL-3.0-or-later with modding and linking exceptions |
| SKSE Menu Framework SDK | [QTR-Modding/SKSE-Menu-Framework-3-API](https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API) | LGPL-2.1 |
| spdlog | [gabime/spdlog](https://github.com/gabime/spdlog) | MIT |
| fmt | [fmtlib/fmt](https://github.com/fmtlib/fmt) | MIT |
| rapidcsv | [d99kris/rapidcsv](https://github.com/d99kris/rapidcsv) | BSD-3-Clause |
| DirectXMath | [microsoft/DirectXMath](https://github.com/microsoft/DirectXMath) | MIT |
| DirectXTK | [microsoft/DirectXTK](https://github.com/microsoft/DirectXTK) | MIT |
| OpenVR | [ValveSoftware/openvr](https://github.com/ValveSoftware/openvr) | BSD-3-Clause |

The SDK header is in `third_party/`. CommonLib and OpenVR source revisions are pinned by the build, and vcpkg defines the remaining dependency versions. License texts are included in `licenses/` and in distribution archives. The package script collects the licenses corresponding to the dependency versions used by the build.
