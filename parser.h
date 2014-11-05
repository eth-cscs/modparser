#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    Parser(Module& m)
    :   module_(m),
        Lexer(m.buffer())
    {
        // prime the first token
        get_token();
    }

private:
    Module &module_;
};
