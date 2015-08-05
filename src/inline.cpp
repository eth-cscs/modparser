#include <list>

#include "inline.h"
#include "variablerenamer.h"

namespace impl {

std::string
rename(std::string const& name, Scope::symbol_map const& locals) {
    std::string renamed = name;
    int i = 0;
    while(locals.count(renamed)) {
        renamed += "_inl_" + pprintf("%", i);
        ++i;
    }

    return renamed;
}

}

Expression*
inline_procedure(BlockExpression* block, const int recursion_level) {
    // TODO : assert that the block to be inlined has had semantic analysis performed on it
    std::list<Expression*> body;

    // get copy of local variables
    Scope::symbol_map locals = block->scope()->locals();

    for(auto e: block->body()) {
        // ignore local variable declarations
        if(e->is_local_declaration()) continue;

        // an assignment expression can be copied over directly
        if(e->is_assignment()) {
            body.push_back(e->clone());
        }
        else if(e->is_procedure_call()) {
            auto call = e->is_procedure_call();
            auto& proc_name = call->name();
            auto proc = block->scope()->find(proc_name).expression->is_procedure();
            std::cout << "found the function " << (proc!=nullptr ? proc->to_string() : red("error")) << std::endl;

            // recursively inline procedure call
            auto proc_inl
                = inline_procedure(proc->body(), recursion_level+1)->is_procedure();
            auto scp_inl  = proc_inl->scope();

            // iterate over local variables in the inlined call
            for(auto& var : scp_inl->locals()) {
                auto name_base = var.first;
                // if a variable with the same name already exists in this
                // scope, rename the variable and rename all references to it
                // in the code that is to be inlined
                bool exists = locals.find(name_base)==locals.end();
                if(exists) {
                    auto name_inl = impl::rename(name_base, locals);
                    locals[name_inl]
                        = { symbolKind::local, new LocalDeclaration( proc->location(),
                                                          name_inl)};
                    // replace all instances of old name with new symbol
                    auto renamer = new VariableRenamer(name_base, name_inl);
                    proc_inl->accept(renamer);
                    //proc_inl->rename_variables(name_base, locals[name_inl]);
                }
            }
            for(auto e_inl: *(proc_inl->body())) {
                // Careful here, we are pushing back something that was
                // implicitly cloned in the call to inline_procedure(e)
                // above. This approach means that each expression is only
                // cloned once, but risks memory leaks when we implement
                // smart pointers at a later date
                body.push_back(e_inl);
            }
        }
        else {
            std::cerr << red("error ") + " I don't know how to inline this : "
                      << e->to_string() << std::endl;
            assert(false);
        }
    }

    // TODO : arguments have to be renamed?
    auto p = new BlockExpression(block->location(), body, block->is_nested());
    //p->semantic();
    return p;
}

