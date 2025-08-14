
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/workspace/esp-idf/components/esp_gdbstub/src/port/xtensa/gdbstub-entry.S" "/workspace/build/esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/port/xtensa/gdbstub-entry.S.obj"
  "/workspace/esp-idf/components/esp_gdbstub/src/port/xtensa/xt_debugexception.S" "/workspace/build/esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/port/xtensa/xt_debugexception.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "ESP_PLATFORM"
  "IDF_VER=\"v5.4\""
  "SOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE"
  "SOC_XTAL_FREQ_MHZ=CONFIG_XTAL_FREQ"
  "_GLIBCXX_HAVE_POSIX_SEMAPHORE"
  "_GLIBCXX_USE_POSIX_SEMAPHORE"
  "_GNU_SOURCE"
  "_POSIX_READER_WRITER_LOCKS"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "config"
  "/workspace/esp-idf/components/esp_gdbstub/include"
  "/workspace/esp-idf/components/esp_gdbstub/private_include"
  "/workspace/esp-idf/components/esp_gdbstub/src/port/xtensa/include"
  "/workspace/esp-idf/components/newlib/platform_include"
  "/workspace/esp-idf/components/freertos/config/include"
  "/workspace/esp-idf/components/freertos/config/include/freertos"
  "/workspace/esp-idf/components/freertos/config/xtensa/include"
  "/workspace/esp-idf/components/freertos/FreeRTOS-Kernel/include"
  "/workspace/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include"
  "/workspace/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos"
  "/workspace/esp-idf/components/freertos/esp_additions/include"
  "/workspace/esp-idf/components/esp_hw_support/include"
  "/workspace/esp-idf/components/esp_hw_support/include/soc"
  "/workspace/esp-idf/components/esp_hw_support/include/soc/esp32s3"
  "/workspace/esp-idf/components/esp_hw_support/dma/include"
  "/workspace/esp-idf/components/esp_hw_support/ldo/include"
  "/workspace/esp-idf/components/esp_hw_support/debug_probe/include"
  "/workspace/esp-idf/components/esp_hw_support/port/esp32s3/."
  "/workspace/esp-idf/components/esp_hw_support/port/esp32s3/include"
  "/workspace/esp-idf/components/heap/include"
  "/workspace/esp-idf/components/heap/tlsf"
  "/workspace/esp-idf/components/log/include"
  "/workspace/esp-idf/components/soc/include"
  "/workspace/esp-idf/components/soc/esp32s3"
  "/workspace/esp-idf/components/soc/esp32s3/include"
  "/workspace/esp-idf/components/soc/esp32s3/register"
  "/workspace/esp-idf/components/hal/platform_port/include"
  "/workspace/esp-idf/components/hal/esp32s3/include"
  "/workspace/esp-idf/components/hal/include"
  "/workspace/esp-idf/components/esp_rom/include"
  "/workspace/esp-idf/components/esp_rom/esp32s3/include"
  "/workspace/esp-idf/components/esp_rom/esp32s3/include/esp32s3"
  "/workspace/esp-idf/components/esp_rom/esp32s3"
  "/workspace/esp-idf/components/esp_common/include"
  "/workspace/esp-idf/components/esp_system/include"
  "/workspace/esp-idf/components/esp_system/port/soc"
  "/workspace/esp-idf/components/esp_system/port/include/private"
  "/workspace/esp-idf/components/xtensa/esp32s3/include"
  "/workspace/esp-idf/components/xtensa/include"
  "/workspace/esp-idf/components/xtensa/deprecated_include"
  "/workspace/esp-idf/components/lwip/include"
  "/workspace/esp-idf/components/lwip/include/apps"
  "/workspace/esp-idf/components/lwip/include/apps/sntp"
  "/workspace/esp-idf/components/lwip/lwip/src/include"
  "/workspace/esp-idf/components/lwip/port/include"
  "/workspace/esp-idf/components/lwip/port/freertos/include"
  "/workspace/esp-idf/components/lwip/port/esp32xx/include"
  "/workspace/esp-idf/components/lwip/port/esp32xx/include/arch"
  "/workspace/esp-idf/components/lwip/port/esp32xx/include/sys"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/workspace/esp-idf/components/esp_gdbstub/src/gdbstub.c" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/gdbstub.c.obj" "gcc" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/gdbstub.c.obj.d"
  "/workspace/esp-idf/components/esp_gdbstub/src/gdbstub_transport.c" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/gdbstub_transport.c.obj" "gcc" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/gdbstub_transport.c.obj.d"
  "/workspace/esp-idf/components/esp_gdbstub/src/packet.c" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/packet.c.obj" "gcc" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/packet.c.obj.d"
  "/workspace/esp-idf/components/esp_gdbstub/src/port/xtensa/gdbstub_xtensa.c" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/port/xtensa/gdbstub_xtensa.c.obj" "gcc" "esp-idf/esp_gdbstub/CMakeFiles/__idf_esp_gdbstub.dir/src/port/xtensa/gdbstub_xtensa.c.obj.d"
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_LINKED_INFO_FILES
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_FORWARD_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
