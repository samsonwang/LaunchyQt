# OpenSSL runtime DLLs

Qt Network loads OpenSSL dynamically at runtime. The Qt 5.12 msvc2017_64
build requires OpenSSL **1.1.x** x64 (it looks for `libssl-1_1-x64.dll` /
`libcrypto-1_1-x64.dll`, or the same names without the `-x64` suffix).
Without these DLLs next to Launchy.exe, every https request fails with:

    network error 99, http status 0: TLS initialization failed

(`QSslSocket::supportsSsl()` returns false and
`QSslSocket::sslLibraryVersionString()` is empty; note that
`sslLibraryBuildVersionString()` still reports "OpenSSL 1.1.1g" because it
is a compile-time constant.)

## Contents

| file                  | version      | arch |
|-----------------------|--------------|------|
| libssl-1_1-x64.dll    | OpenSSL 1.1.1s | x64 |
| libcrypto-1_1-x64.dll | OpenSSL 1.1.1s | x64 |

## Source of these binaries

Extracted from the Intel iCLS client driver package installed by Windows
Update:

    C:\Windows\System32\DriverStore\FileRepository\iclsclient.inf_amd64_*\lib

Both DLLs carry a valid Authenticode signature by
"Microsoft Windows Hardware Compatibility Publisher" (WHQL).

## Replacing with official builds (recommended for releases)

Any OpenSSL **1.1.x** win64 build works, keep the same file names:

- conda-forge: https://conda.anaconda.org/conda-forge/win-64/
  (package `openssl`, series 1.1.1*, layout `Library/bin/*.dll`)
- Shining Light Productions: https://slproweb.com/products/Win32OpenSSL.html
  (Win64 OpenSSL 1.1.1x "Light" edition, take the two DLLs from `bin`)

Do **not** use OpenSSL 3.x DLLs, Qt 5.12 cannot load them.

`src/CMakeLists.txt` copies both DLLs into the build output directory
(`bin/<Config>/`) after linking Launchy.

## License

OpenSSL 1.1.1 - OpenSSL License / SSLeay License,
see https://www.openssl.org/source/license.html
