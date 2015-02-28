set(BASE_SOURCES
    variablerenamer.cpp
    textbuffer.cpp
    cprinter.cpp
    cudaprinter.cpp
    expressionclassifier.cpp
    constantfolder.cpp
    errorvisitor.cpp
    scope.cpp
    expression.cpp
    lexer.cpp
    module.cpp
    parser.cpp
    inline.cpp
)

add_library(compiler ${BASE_SOURCES})

add_executable(modcc modcc.cpp)

target_link_libraries(modcc         LINK_PUBLIC compiler)

set_target_properties( modcc
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

