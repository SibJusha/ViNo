install(
  TARGETS ViNo_exe
  RUNTIME COMPONENT ViNo_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()