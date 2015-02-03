#include "test.h"
#include "../src/module.h"

TEST(Module, open) {
    Module m("./modfiles/test.mod");
    Lexer lexer(m.buffer());
    auto t = lexer.parse();
    while(t.type != tok_eof) {
        t = lexer.parse();
        EXPECT_NE(t.type, tok_reserved);
    }
}

