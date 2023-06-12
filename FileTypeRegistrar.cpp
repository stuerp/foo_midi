
/** $VER: FileTypeRegistrar.h (2022.12.31) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "FileTypeRegistrar.h"

static service_factory_single_t<FileTypeRegistrar> FileTypeRegistrarFactory;
