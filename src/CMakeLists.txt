set(BASE_SOURCES
    #variablerenamer.cpp
    #inline.cpp
    expression.cpp
    textbuffer.cpp
    cprinter.cpp
    cudaprinter.cpp
    cymeprinter.cpp
    expressionclassifier.cpp
    constantfolder.cpp
    errorvisitor.cpp
    lexer.cpp
    module.cpp
    parser.cpp
)

add_library(compiler ${BASE_SOURCES})

add_executable(modcc modcc.cpp)

target_link_libraries(modcc         LINK_PUBLIC compiler)

set_target_properties( modcc
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

