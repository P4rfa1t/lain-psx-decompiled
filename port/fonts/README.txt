The Nintendo Switch package includes a Simplified Chinese UI font:

  fonts/ui.ttf

It is WenQuanYi Micro Hei (文泉驿微米黑), dual-licensed under Apache 2.0 and
GPLv3 (see LICENSE_Apache2.txt, LICENSE_GPLv3.txt and README-WQY.txt). The
font is not stored in the repository; port/tools/package_switch.sh downloads
it when packaging, and the client loads it from <data folder>/fonts/ui.ttf.
To use another font, replace fonts/ui.ttf in the package.
