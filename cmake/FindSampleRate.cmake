find_path(SAMPLERATE_INCLUDE_DIR NAMES samplerate.h HINTS include)
find_library(SAMPLERATE_LIBRARIES NAMES samplerate HINTS lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SampleRate DEFAULT_MSG SAMPLERATE_LIBRARIES
                                  SAMPLERATE_INCLUDE_DIR)

mark_as_advanced(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARIES)
