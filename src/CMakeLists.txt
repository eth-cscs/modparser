set(BASE_SOURCES
    variablerenamer.cpp
    cprinter.cpp
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
#target_link_libraries(unittests.exe LINK_PUBLIC compiler)

