#include "test.h"
#include "../src/module.h"

TEST(Module, open) {
    Module m("./modfiles/test.mod");
    if(!m.buffer().size()) {
        std::cout << "skipping Module.open test because unable to open input file" << std::endl;
        return;
    }
    Lexer lexer(m.buffer());
    auto t = lexer.parse();
    while(t.type != tok_eof) {
        t = lexer.parse();
        EXPECT_NE(t.type, tok_reserved);
    }
}
