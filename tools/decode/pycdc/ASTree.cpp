#include <cstring>
#include "ASTree.h"
#include "FastStack.h"
#include "pyc_numeric.h"
#include "bytecode.h"

// This must be a triple quote (''' or """), to handle interpolated string literals containing the opposite quote style.
// E.g. f'''{"interpolated "123' literal"}'''    -> valid.
// E.g. f"""{"interpolated "123' literal"}"""    -> valid.
// E.g. f'{"interpolated "123' literal"}'        -> invalid, unescaped quotes in literal.
// E.g. f'{"interpolated \"123\' literal"}'      -> invalid, f-string expression does not allow backslash.
// NOTE: Nested f-strings not supported.
#define F_STRING_QUOTE "'''"

static void append_to_chain_store(PycRef<ASTNode> chainStore, PycRef<ASTNode> item,
        FastStack& stack, PycRef<ASTBlock> curblock);

/* Use this to determine if an error occurred (and therefore, if we should
 * avoid cleaning the output tree) */
static bool cleanBuild;

/* Use this to prevent printing return keywords and newlines in lambdas. */
static bool inLambda = false;

/* Use this to keep track of whether we need to print out any docstring and
 * the list of global variables that we are using (such as inside a function). */
static bool printDocstringAndGlobals = false;

/* Use this to keep track of whether we need to print a class or module docstring */
static bool printClassDocstring = true;

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod)
{
    PycBuffer source(code->code()->value(), code->code()->length());

    FastStack stack((mod->majorVer() == 1) ? 20 : code->stackSize());
    stackhist_t stack_hist;

    std::stack<PycRef<ASTBlock> > blocks;
    PycRef<ASTBlock> defblock = new ASTBlock(ASTBlock::BLK_MAIN);
    defblock->init();
    PycRef<ASTBlock> curblock = defblock;
    blocks.push(defblock);

    int opcode, operand;
    int curpos = 0;
    int pos = 0;
    int unpack = 0;
    bool else_pop = false;
    bool need_try = false;
    bool variable_annotations = false;

    while (!source.atEof()) {
#if defined(BLOCK_DEBUG) || defined(STACK_DEBUG)
        fprintf(stderr, "%-7d", pos);
    #ifdef STACK_DEBUG
        fprintf(stderr, "%-5d", (unsigned int)stack_hist.size() + 1);
    #endif
    #ifdef BLOCK_DEBUG
        for (unsigned int i = 0; i < blocks.size(); i++)
            fprintf(stderr, "    ");
        fprintf(stderr, "%s (%d)", curblock->type_str(), curblock->end());
    #endif
        fprintf(stderr, "\n");
#endif

        curpos = pos;
        bc_next(source, mod, opcode, operand, pos);

        if (need_try && opcode != Pyc::SETUP_EXCEPT_A) {
            need_try = false;

            /* Store the current stack for the except/finally statement(s) */
            stack_hist.push(stack);
            PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, curblock->end(), true);
            blocks.push(tryblock);
            curblock = blocks.top();
        } else if (else_pop
                && opcode != Pyc::JUMP_FORWARD_A
                && opcode != Pyc::JUMP_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_FALSE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_TRUE_A
                && opcode != Pyc::JUMP_IF_TRUE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_TRUE_A
                && opcode != Pyc::POP_BLOCK) {
            else_pop = false;

            PycRef<ASTBlock> prev = curblock;
            while (prev->end() < pos
                    && prev->blktype() != ASTBlock::BLK_MAIN) {
                if (prev->blktype() != ASTBlock::BLK_CONTAINER) {
                    if (prev->end() == 0) {
                        break;
                    }

                    /* We want to keep the stack the same, but we need to pop
                     * a level off the history. */
                    //stack = stack_hist.top();
                    if (!stack_hist.empty())
                        stack_hist.pop();
                }
                blocks.pop();

                if (blocks.empty())
                    break;

                curblock = blocks.top();
                curblock->append(prev.cast<ASTNode>());

                prev = curblock;
            }
        }

        switch (opcode) {
        case Pyc::BINARY_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_ADD));
            }
            break;
        case Pyc::BINARY_AND:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_AND));
            }
            break;
        case Pyc::BINARY_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_FLOOR_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_FLOOR));
            }
            break;
        case Pyc::BINARY_LSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_LSHIFT));
            }
            break;
        case Pyc::BINARY_MODULO:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MODULO));
            }
            break;
        case Pyc::BINARY_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MULTIPLY));
            }
            break;
        case Pyc::BINARY_OR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_OR));
            }
            break;
        case Pyc::BINARY_POWER:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_POWER));
            }
            break;
        case Pyc::BINARY_RSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_RSHIFT));
            }
            break;
        case Pyc::BINARY_SUBSCR:
            {
                PycRef<ASTNode> subscr = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTSubscr(src, subscr));
            }
            break;
        case Pyc::BINARY_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case Pyc::BINARY_TRUE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_XOR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_XOR));
            }
            break;
        case Pyc::BINARY_MATRIX_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MAT_MULTIPLY));
            }
            break;
        case Pyc::BREAK_LOOP:
            curblock->append(new ASTKeyword(ASTKeyword::KW_BREAK));
            break;
        case Pyc::BUILD_CLASS:
            {
                PycRef<ASTNode> class_code = stack.top();
                stack.pop();
                PycRef<ASTNode> bases = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTClass(class_code, bases, name));
            }
            break;
        case Pyc::BUILD_FUNCTION:
            {
                PycRef<ASTNode> fun_code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(fun_code, {}, {}));
            }
            break;
        case Pyc::BUILD_LIST_A:
            {
                ASTList::value_t values;
                for (int i=0; i<operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTList(values));
            }
            break;
        case Pyc::BUILD_MAP_A:
            if (mod->verCompare(3, 5) >= 0) {
                auto map = new ASTMap;
                for (int i=0; i<operand; ++i) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    map->add(key, value);
                }
                stack.push(map);
            } else {
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                stack.push(new ASTMap());
            }
            break;
        case Pyc::BUILD_CONST_KEY_MAP_A:
            // Top of stack will be a tuple of keys.
            // Values will start at TOS - 1.
            {
                PycRef<ASTNode> keys = stack.top();
                stack.pop();

                ASTConstMap::values_t values;
                values.reserve(operand);
                for (int i = 0; i < operand; ++i) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    values.push_back(value);
                }

                stack.push(new ASTConstMap(keys, values));
            }
            break;
        case Pyc::STORE_MAP:
            {
                PycRef<ASTNode> key = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTMap> map = stack.top().cast<ASTMap>();
                map->add(key, value);
            }
            break;
        case Pyc::BUILD_SLICE_A:
            {
                if (operand == 2) {
                    PycRef<ASTNode> end = stack.top();
                    stack.pop();
                    PycRef<ASTNode> start = stack.top();
                    stack.pop();

                    if (start.type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = NULL;
                    }

                    if (end.type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = NULL;
                    }

                    if (start == NULL && end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
                    }
                } else if (operand == 3) {
                    PycRef<ASTNode> step = stack.top();
                    stack.pop();
                    PycRef<ASTNode> end = stack.top();
                    stack.pop();
                    PycRef<ASTNode> start = stack.top();
                    stack.pop();

                    if (start.type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = NULL;
                    }

                    if (end.type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = NULL;
                    }

                    if (step.type() == ASTNode::NODE_OBJECT
                            && step.cast<ASTObject>()->object() == Pyc_None) {
                        step = NULL;
                    }

                    /* We have to do this as a slice where one side is another slice */
                    /* [[a:b]:c] */

                    if (start == NULL && end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
                    }

                    PycRef<ASTNode> lhs = stack.top();
                    stack.pop();

                    if (step == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, lhs, step));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, lhs, step));
                    }
                }
            }
            break;
        case Pyc::BUILD_STRING_A:
            {
                // Nearly identical logic to BUILD_LIST
                ASTList::value_t values;
                for (int i = 0; i < operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTJoinedStr(values));
            }
            break;
        case Pyc::BUILD_TUPLE_A:
            {
                ASTTuple::value_t values;
                values.resize(operand);
                for (int i=0; i<operand; i++) {
                    values[operand-i-1] = stack.top();
                    stack.pop();
                }
                stack.push(new ASTTuple(values));
            }
            break;
        case Pyc::CALL_FUNCTION_A:
            {
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;

                /* Test for the load build class function */
                stack_hist.push(stack);
                int basecnt = 0;
                ASTTuple::value_t bases;
                bases.resize(basecnt);
                PycRef<ASTNode> TOS = stack.top();
                int TOS_type = TOS.type();
                // bases are NODE_NAME at TOS
                while (TOS_type == ASTNode::NODE_NAME) {
                    bases.resize(basecnt + 1);
                    bases[basecnt] = TOS;
                    basecnt++;
                    stack.pop();
                    TOS = stack.top();
                    TOS_type = TOS.type();
                }
                // qualified name is PycString at TOS
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                PycRef<ASTNode> function = stack.top();
                stack.pop();
                PycRef<ASTNode> loadbuild = stack.top();
                stack.pop();
                int loadbuild_type = loadbuild.type();
                if (loadbuild_type == ASTNode::NODE_LOADBUILDCLASS) {
                    PycRef<ASTNode> call = new ASTCall(function, pparamList, kwparamList);
                    stack.push(new ASTClass(call, new ASTTuple(bases), name));
                    stack_hist.pop();
                    break;
                }
                else
                {
                    stack = stack_hist.top();
                    stack_hist.pop();
                }

                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    PycRef<ASTNode> param = stack.top();
                    stack.pop();
                    if (param.type() == ASTNode::NODE_FUNCTION) {
                        PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
                        PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
                        PycRef<PycString> function_name = code_src->name();
                        if (function_name->isEqual("<lambda>")) {
                            pparamList.push_front(param);
                        } else {
                            // Decorator used
                            PycRef<ASTNode> decor_name = new ASTName(function_name);
                            curblock->append(new ASTStore(param, decor_name));

                            pparamList.push_front(decor_name);
                        }
                    } else {
                        pparamList.push_front(param);
                    }
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, pparamList, kwparamList));
            }
            break;
        case Pyc::CALL_FUNCTION_VAR_A:
            {
                PycRef<ASTNode> var = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setVar(var);
                stack.push(call);
            }
            break;
        case Pyc::CALL_FUNCTION_KW_A:
            {
                PycRef<ASTNode> kw = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setKW(kw);
                stack.push(call);
            }
            break;
        case Pyc::CALL_FUNCTION_VAR_KW_A:
            {
                PycRef<ASTNode> kw = stack.top();
                stack.pop();
                PycRef<ASTNode> var = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setKW(kw);
                call.cast<ASTCall>()->setVar(var);
                stack.push(call);
            }
            break;
        case Pyc::CALL_METHOD_A:
            {
                ASTCall::pparam_t pparamList;
                for (int i = 0; i < operand; i++) {
                    PycRef<ASTNode> param = stack.top();
                    stack.pop();
                    if (param.type() == ASTNode::NODE_FUNCTION) {
                        PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
                        PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
                        PycRef<PycString> function_name = code_src->name();
                        if (function_name->isEqual("<lambda>")) {
                            pparamList.push_front(param);
                        } else {
                            // Decorator used
                            PycRef<ASTNode> decor_name = new ASTName(function_name);
                            curblock->append(new ASTStore(param, decor_name));

                            pparamList.push_front(decor_name);
                        }
                    } else {
                        pparamList.push_front(param);
                    }
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, pparamList, ASTCall::kwparam_t()));
            }
            break;
        case Pyc::CONTINUE_LOOP_A:
            curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
            break;
        case Pyc::COMPARE_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, operand));
            }
            break;
        case Pyc::CONTAINS_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                // The operand will be 0 for 'in' and 1 for 'not in'.
                stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_NOT_IN : ASTCompare::CMP_IN));
            }
            break;
        case Pyc::DELETE_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                curblock->append(new ASTDelete(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR)));
            }
            break;
        case Pyc::DELETE_GLOBAL_A:
            code->markGlobal(code->getName(operand));
            /* Fall through */
        case Pyc::DELETE_NAME_A:
            {
                PycRef<PycString> varname = code->getName(operand);

                if (varname->length() >= 2 && varname->value()[0] == '_'
                        && varname->value()[1] == '[') {
                    /* Don't show deletes that are a result of list comps. */
                    break;
                }

                PycRef<ASTNode> name = new ASTName(varname);
                curblock->append(new ASTDelete(name));
            }
            break;
        case Pyc::DELETE_FAST_A:
            {
                PycRef<ASTNode> name;

                if (mod->verCompare(1, 3) < 0)
                    name = new ASTName(code->getName(operand));
                else
                    name = new ASTName(code->getVarName(operand));

                if (name.cast<ASTName>()->name()->value()[0] == '_'
                        && name.cast<ASTName>()->name()->value()[1] == '[') {
                    /* Don't show deletes that are a result of list comps. */
                    break;
                }

                curblock->append(new ASTDelete(name));
            }
            break;
        case Pyc::DELETE_SLICE_0:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE0))));
            }
            break;
        case Pyc::DELETE_SLICE_1:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE1, upper))));
            }
            break;
        case Pyc::DELETE_SLICE_2:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
            }
            break;
        case Pyc::DELETE_SLICE_3:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
            }
            break;
        case Pyc::DELETE_SUBSCR:
            {
                PycRef<ASTNode> key = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, key)));
            }
            break;
        case Pyc::DUP_TOP:
            {
                if (stack.top().type() == PycObject::TYPE_NULL) {
                    stack.push(stack.top());
                } else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    auto chainstore = stack.top();
                    stack.pop();
                    stack.push(stack.top());
                    stack.push(chainstore);
                } else {
                    stack.push(stack.top());
                    ASTNodeList::list_t targets;
                    stack.push(new ASTChainStore(targets, stack.top()));
                }
            }
            break;
        case Pyc::DUP_TOP_TWO:
            {
                PycRef<ASTNode> first = stack.top();
                stack.pop();
                PycRef<ASTNode> second = stack.top();

                stack.push(first);
                stack.push(second);
                stack.push(first);
            }
            break;
        case Pyc::DUP_TOPX_A:
            {
                std::stack<PycRef<ASTNode> > first;
                std::stack<PycRef<ASTNode> > second;

                for (int i = 0; i < operand; i++) {
                    PycRef<ASTNode> node = stack.top();
                    stack.pop();
                    first.push(node);
                    second.push(node);
                }

                while (first.size()) {
                    stack.push(first.top());
                    first.pop();
                }

                while (second.size()) {
                    stack.push(second.top());
                    second.pop();
                }
            }
            break;
        case Pyc::END_FINALLY:
            {
                bool isFinally = false;
                if (curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    PycRef<ASTBlock> final = curblock;
                    blocks.pop();

                    stack = stack_hist.top();
                    stack_hist.pop();

                    curblock = blocks.top();
                    curblock->append(final.cast<ASTNode>());
                    isFinally = true;
                } else if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                    blocks.pop();
                    PycRef<ASTBlock> prev = curblock;

                    bool isUninitAsyncFor = false;
                    if (blocks.top()->blktype() == ASTBlock::BLK_CONTAINER) {
                        auto container = blocks.top();
                        blocks.pop();
                        auto asyncForBlock = blocks.top();
                        isUninitAsyncFor = asyncForBlock->blktype() == ASTBlock::BLK_ASYNCFOR && !asyncForBlock->inited();
                        if (isUninitAsyncFor) {
                            auto tryBlock = container->nodes().front().cast<ASTBlock>();
                            if (!tryBlock->nodes().empty() && tryBlock->blktype() == ASTBlock::BLK_TRY) {
                                auto store = tryBlock->nodes().front().cast<ASTStore>();
                                if (store) {
                                    asyncForBlock.cast<ASTIterBlock>()->setIndex(store->dest());
                                }
                            }
                            curblock = blocks.top();
                            stack = stack_hist.top();
                            stack_hist.pop();
                            if (!curblock->inited())
                                fprintf(stderr, "Error when decompiling 'async for'.\n");
                        } else {
                            blocks.push(container);
                        }
                    }

                    if (!isUninitAsyncFor) {
                        if (curblock->size() != 0) {
                            blocks.top()->append(curblock.cast<ASTNode>());
                        }

                        curblock = blocks.top();

                        /* Turn it into an else statement. */
                        if (curblock->end() != pos || curblock.cast<ASTContainerBlock>()->hasFinally()) {
                            PycRef<ASTBlock> elseblk = new ASTBlock(ASTBlock::BLK_ELSE, prev->end());
                            elseblk->init();
                            blocks.push(elseblk);
                            curblock = blocks.top();
                        }
                        else {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    /* This marks the end of the except block(s). */
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (!cont->hasFinally() || isFinally) {
                        /* If there's no finally block, pop the container. */
                        blocks.pop();
                        curblock = blocks.top();
                        curblock->append(cont.cast<ASTNode>());
                    }
                }
            }
            break;
        case Pyc::EXEC_STMT:
            {
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> loc = stack.top();
                stack.pop();
                PycRef<ASTNode> glob = stack.top();
                stack.pop();
                PycRef<ASTNode> stmt = stack.top();
                stack.pop();

                curblock->append(new ASTExec(stmt, glob, loc));
            }
            break;
        case Pyc::FOR_ITER_A:
            {
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();
                /* Pop it? Don't pop it? */

                bool comprehension = false;
                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                } else {
                    comprehension = true;
                }
                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, top->end(), iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                stack.push(NULL);
            }
            break;
        case Pyc::FOR_LOOP_A:
            {
                PycRef<ASTNode> curidx = stack.top(); // Current index
                stack.pop();
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();

                bool comprehension = false;
                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                } else {
                    comprehension = true;
                }
                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, top->end(), iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                /* Python Docs say:
                      "push the sequence, the incremented counter,
                       and the current item onto the stack." */
                stack.push(iter);
                stack.push(curidx);
                stack.push(NULL); // We can totally hack this >_>
            }
            break;
        case Pyc::GET_AITER:
            {
                // Logic similar to FOR_ITER_A
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();

                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                    PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_ASYNCFOR, top->end(), iter);
                    blocks.push(forblk.cast<ASTBlock>());
                    curblock = blocks.top();
                    stack.push(nullptr);
                } else {
                     fprintf(stderr, "Unsupported use of GET_AITER outside of SETUP_LOOP\n");
                }
            }
            break;
        case Pyc::GET_ANEXT:
            break;
        case Pyc::FORMAT_VALUE_A:
            {
                auto conversion_flag = static_cast<ASTFormattedValue::ConversionFlag>(operand);
                switch (conversion_flag) {
                case ASTFormattedValue::ConversionFlag::NONE:
                case ASTFormattedValue::ConversionFlag::STR:
                case ASTFormattedValue::ConversionFlag::REPR:
                case ASTFormattedValue::ConversionFlag::ASCII:
                    {
                        auto val = stack.top();
                        stack.pop();
                        stack.push(new ASTFormattedValue(val, conversion_flag, nullptr));
                    }
                    break;
                case ASTFormattedValue::ConversionFlag::FMTSPEC:
                    {
                        auto format_spec = stack.top();
                        stack.pop();
                        auto val = stack.top();
                        stack.pop();
                        stack.push(new ASTFormattedValue(val, conversion_flag, format_spec));
                    }
                    break;
                default:
                    fprintf(stderr, "Unsupported FORMAT_VALUE_A conversion flag: %d\n", operand);
                }
            }
            break;
        case Pyc::GET_AWAITABLE:
            {
                PycRef<ASTNode> object = stack.top();
                stack.pop();
                stack.push(new ASTAwaitable(object));
            }
            break;
        case Pyc::GET_ITER:
            /* We just entirely ignore this */
            break;
        case Pyc::IMPORT_NAME_A:
            if (mod->majorVer() == 1) {
                stack.push(new ASTImport(new ASTName(code->getName(operand)), NULL));
            } else {
                PycRef<ASTNode> fromlist = stack.top();
                stack.pop();
                if (mod->verCompare(2, 5) >= 0)
                    stack.pop();    // Level -- we don't care
                stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
            }
            break;
        case Pyc::IMPORT_FROM_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::IMPORT_STAR:
            {
                PycRef<ASTNode> import = stack.top();
                stack.pop();
                curblock->append(new ASTStore(import, NULL));
            }
            break;
        case Pyc::INPLACE_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_ADD));
            }
            break;
        case Pyc::INPLACE_AND:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_AND));
            }
            break;
        case Pyc::INPLACE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_DIVIDE));
            }
            break;
        case Pyc::INPLACE_FLOOR_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_FLOOR));
            }
            break;
        case Pyc::INPLACE_LSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_LSHIFT));
            }
            break;
        case Pyc::INPLACE_MODULO:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_MODULO));
            }
            break;
        case Pyc::INPLACE_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_MULTIPLY));
            }
            break;
        case Pyc::INPLACE_OR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_OR));
            }
            break;
        case Pyc::INPLACE_POWER:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_POWER));
            }
            break;
        case Pyc::INPLACE_RSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_RSHIFT));
            }
            break;
        case Pyc::INPLACE_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_SUBTRACT));
            }
            break;
        case Pyc::INPLACE_TRUE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_DIVIDE));
            }
            break;
        case Pyc::INPLACE_XOR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_XOR));
            }
            break;
        case Pyc::INPLACE_MATRIX_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_MAT_MULTIPLY));
            }
            break;
        case Pyc::IS_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                // The operand will be 0 for 'is' and 1 for 'is not'.
                stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_IS_NOT : ASTCompare::CMP_IS));
            }
            break;
        case Pyc::JUMP_IF_FALSE_A:
        case Pyc::JUMP_IF_TRUE_A:
        case Pyc::JUMP_IF_FALSE_OR_POP_A:
        case Pyc::JUMP_IF_TRUE_OR_POP_A:
        case Pyc::POP_JUMP_IF_FALSE_A:
        case Pyc::POP_JUMP_IF_TRUE_A:
            {
                PycRef<ASTNode> cond = stack.top();
                PycRef<ASTCondBlock> ifblk;
                int popped = ASTCondBlock::UNINITED;

                if (opcode == Pyc::POP_JUMP_IF_FALSE_A
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A) {
                    /* Pop condition before the jump */
                    stack.pop();
                    popped = ASTCondBlock::PRE_POPPED;
                }

                /* Store the current stack for the else statement(s) */
                stack_hist.push(stack);

                if (opcode == Pyc::JUMP_IF_FALSE_OR_POP_A
                        || opcode == Pyc::JUMP_IF_TRUE_OR_POP_A) {
                    /* Pop condition only if condition is met */
                    stack.pop();
                    popped = ASTCondBlock::POPPED;
                }

                /* "Jump if true" means "Jump if not false" */
                bool neg = opcode == Pyc::JUMP_IF_TRUE_A
                        || opcode == Pyc::JUMP_IF_TRUE_OR_POP_A
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A;

                int offs = operand;
                if (opcode == Pyc::JUMP_IF_FALSE_A
                        || opcode == Pyc::JUMP_IF_TRUE_A) {
                    /* Offset is relative in these cases */
                    offs = pos + operand;
                }

                if (cond.type() == ASTNode::NODE_COMPARE
                        && cond.cast<ASTCompare>()->op() == ASTCompare::CMP_EXCEPTION) {
                    if (curblock->blktype() == ASTBlock::BLK_EXCEPT
                            && curblock.cast<ASTCondBlock>()->cond() == NULL) {
                        blocks.pop();
                        curblock = blocks.top();

                        stack_hist.pop();
                    }
                    ifblk = new ASTCondBlock(ASTBlock::BLK_EXCEPT, offs, cond.cast<ASTCompare>()->right(), false);
                } else if (curblock->blktype() == ASTBlock::BLK_ELSE
                           && curblock->size() == 0) {
                    /* Collapse into elif statement */
                    blocks.pop();
                    stack = stack_hist.top();
                    stack_hist.pop();
                    ifblk = new ASTCondBlock(ASTBlock::BLK_ELIF, offs, cond, neg);
                } else if (curblock->size() == 0 && !curblock->inited()
                           && curblock->blktype() == ASTBlock::BLK_WHILE) {
                    /* The condition for a while loop */
                    PycRef<ASTBlock> top = blocks.top();
                    blocks.pop();
                    ifblk = new ASTCondBlock(top->blktype(), offs, cond, neg);

                    /* We don't store the stack for loops! Pop it! */
                    stack_hist.pop();
                } else if (curblock->size() == 0 && curblock->end() <= offs
                           && (curblock->blktype() == ASTBlock::BLK_IF
                           || curblock->blktype() == ASTBlock::BLK_ELIF
                           || curblock->blktype() == ASTBlock::BLK_WHILE)) {
                    PycRef<ASTNode> newcond;
                    PycRef<ASTCondBlock> top = curblock.cast<ASTCondBlock>();
                    PycRef<ASTNode> cond1 = top->cond();
                    blocks.pop();

                    if (curblock->blktype() == ASTBlock::BLK_WHILE) {
                        stack_hist.pop();
                    } else {
                        FastStack s_top = stack_hist.top();
                        stack_hist.pop();
                        stack_hist.pop();
                        stack_hist.push(s_top);
                    }

                    if (curblock->end() == offs
                            || (curblock->end() == curpos && !top->negative())) {
                        /* if blah and blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_AND);
                    } else {
                        /* if blah or blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_OR);
                    }
                    ifblk = new ASTCondBlock(top->blktype(), offs, newcond, neg);
                } else if (curblock->blktype() == ASTBlock::BLK_FOR
                            && curblock.cast<ASTIterBlock>()->isComprehension()
                            && mod->verCompare(2, 7) >= 0) {
                    /* Comprehension condition */
                    curblock.cast<ASTIterBlock>()->setCondition(cond);
                    stack_hist.pop();
                    // TODO: Handle older python versions, where condition
                    // is laid out a little differently.
                    break;
                } else {
                    /* Plain old if statement */
                    ifblk = new ASTCondBlock(ASTBlock::BLK_IF, offs, cond, neg);
                }

                if (popped)
                    ifblk->init(popped);

                blocks.push(ifblk.cast<ASTBlock>());
                curblock = blocks.top();
            }
            break;
        case Pyc::JUMP_ABSOLUTE_A:
            {
                if (operand < pos) {
                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && curblock.cast<ASTIterBlock>()->isComprehension()) {
                        PycRef<ASTNode> top = stack.top();

                        if (top.type() == ASTNode::NODE_COMPREHENSION) {
                            PycRef<ASTComprehension> comp = top.cast<ASTComprehension>();

                            comp->addGenerator(curblock.cast<ASTIterBlock>());
                        }

                        blocks.pop();
                        curblock = blocks.top();
                    } else if (curblock->blktype() == ASTBlock::BLK_ELSE) {
                        stack = stack_hist.top();
                        stack_hist.pop();

                        blocks.pop();
                        blocks.top()->append(curblock.cast<ASTNode>());
                        curblock = blocks.top();

                        if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                                && !curblock.cast<ASTContainerBlock>()->hasFinally()) {
                            blocks.pop();
                            blocks.top()->append(curblock.cast<ASTNode>());
                            curblock = blocks.top();
                        }
                    } else {
                        curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
                    }

                    /* We're in a loop, this jumps back to the start */
                    /* I think we'll just ignore this case... */
                    break; // Bad idea? Probably!
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (cont->hasExcept() && pos < cont->except()) {
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, 0, NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                stack = stack_hist.top();
                stack_hist.pop();

                PycRef<ASTBlock> prev = curblock;
                PycRef<ASTBlock> nil;
                bool push = true;

                do {
                    blocks.pop();

                    blocks.top()->append(prev.cast<ASTNode>());

                    if (prev->blktype() == ASTBlock::BLK_IF
                            || prev->blktype() == ASTBlock::BLK_ELIF) {
                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, blocks.top()->end());
                        if (prev->inited() == ASTCondBlock::PRE_POPPED) {
                            next->init(ASTCondBlock::PRE_POPPED);
                        }

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, blocks.top()->end(), NULL, false);
                        next->init();

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_ELSE) {
                        /* Special case */
                        prev = blocks.top();
                        if (!push) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                        push = false;
                    } else {
                        prev = nil;
                    }

                } while (prev != nil);

                curblock = blocks.top();
            }
            break;
        case Pyc::JUMP_FORWARD_A:
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (cont->hasExcept()) {
                        stack_hist.push(stack);

                        curblock->setEnd(pos+operand);
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                if ((curblock->blktype() == ASTBlock::BLK_WHILE
                            && !curblock->inited())
                        || (curblock->blktype() == ASTBlock::BLK_IF
                            && curblock->size() == 0)) {
                    PycRef<PycObject> fakeint = new PycInt(1);
                    PycRef<ASTNode> truthy = new ASTObject(fakeint);

                    stack.push(truthy);
                    break;
                }

                if (!stack_hist.empty()) {
                    stack = stack_hist.top();
                    stack_hist.pop();
                }

                PycRef<ASTBlock> prev = curblock;
                PycRef<ASTBlock> nil;
                bool push = true;

                do {
                    blocks.pop();

                    if (!blocks.empty())
                        blocks.top()->append(prev.cast<ASTNode>());

                    if (prev->blktype() == ASTBlock::BLK_IF
                            || prev->blktype() == ASTBlock::BLK_ELIF) {
                        if (operand == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, pos+operand);
                        if (prev->inited() == ASTCondBlock::PRE_POPPED) {
                            next->init(ASTCondBlock::PRE_POPPED);
                        }

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (operand == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, NULL, false);
                        next->init();

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_ELSE) {
                        /* Special case */
                        prev = blocks.top();
                        if (!push) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                        push = false;

                        if (prev->blktype() == ASTBlock::BLK_MAIN) {
                            /* Something went out of control! */
                            prev = nil;
                        }
                    } else if (prev->blktype() == ASTBlock::BLK_TRY
                            && prev->end() < pos+operand) {
                        /* Need to add an except/finally block */
                        stack = stack_hist.top();
                        stack.pop();

                        if (blocks.top()->blktype() == ASTBlock::BLK_CONTAINER) {
                            PycRef<ASTContainerBlock> cont = blocks.top().cast<ASTContainerBlock>();
                            if (cont->hasExcept()) {
                                if (push) {
                                    stack_hist.push(stack);
                                }

                                PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, NULL, false);
                                except->init();
                                blocks.push(except);
                            }
                        } else {
                            fprintf(stderr, "Something TERRIBLE happened!!\n");
                        }
                        prev = nil;
                    } else {
                        prev = nil;
                    }

                } while (prev != nil);

                curblock = blocks.top();

                if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                    curblock->setEnd(pos+operand);
                }
            }
            break;
        case Pyc::LIST_APPEND:
        case Pyc::LIST_APPEND_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                PycRef<ASTNode> list = stack.top();


                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()) {
                    stack.pop();
                    stack.push(new ASTComprehension(value));
                } else {
                    stack.push(new ASTSubscr(list, value)); /* Total hack */
                }
            }
            break;
        case Pyc::LIST_EXTEND_A:
            {
                PycRef<ASTNode> rhs = stack.top();
                stack.pop();
                PycRef<ASTList> lhs = stack.top().cast<ASTList>();
                stack.pop();

                if (rhs.type() != ASTNode::NODE_OBJECT) {
                    fprintf(stderr, "Unsupported argument found for LIST_EXTEND\n");
                    break;
                }

                // I've only ever seen this be a SMALL_TUPLE, but let's be careful...
                PycRef<PycObject> obj = rhs.cast<ASTObject>()->object();
                if (obj->type() != PycObject::TYPE_TUPLE && obj->type() != PycObject::TYPE_SMALL_TUPLE) {
                    fprintf(stderr, "Unsupported argument type found for LIST_EXTEND\n");
                    break;
                }

                ASTList::value_t result = lhs->values();
                for (const auto& it : obj.cast<PycTuple>()->values()) {
                    result.push_back(new ASTObject(it));
                }

                stack.push(new ASTList(result));
            }
            break;
        case Pyc::LOAD_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                if (name.type() != ASTNode::NODE_IMPORT) {
                    stack.pop();
                    stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
                }
            }
            break;
        case Pyc::LOAD_BUILD_CLASS:
            stack.push(new ASTLoadBuildClass(new PycObject()));
            break;
        case Pyc::LOAD_CLOSURE_A:
            /* Ignore this */
            break;
        case Pyc::LOAD_CONST_A:
            {
                PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

                if ((t_ob->object().type() == PycObject::TYPE_TUPLE ||
                        t_ob->object().type() == PycObject::TYPE_SMALL_TUPLE) &&
                        !t_ob->object().cast<PycTuple>()->values().size()) {
                    ASTTuple::value_t values;
                    stack.push(new ASTTuple(values));
                } else if (t_ob->object().type() == PycObject::TYPE_NONE) {
                    stack.push(NULL);
                } else {
                    stack.push(t_ob.cast<ASTNode>());
                }
            }
            break;
        case Pyc::LOAD_DEREF_A:
            stack.push(new ASTName(code->getCellVar(operand)));
            break;
        case Pyc::LOAD_FAST_A:
            if (mod->verCompare(1, 3) < 0)
                stack.push(new ASTName(code->getName(operand)));
            else
                stack.push(new ASTName(code->getVarName(operand)));
            break;
        case Pyc::LOAD_GLOBAL_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::LOAD_LOCALS:
            stack.push(new ASTNode(ASTNode::NODE_LOCALS));
            break;
        case Pyc::STORE_LOCALS:
            stack.pop();
            break;
        case Pyc::LOAD_METHOD_A:
            {
                // Behave like LOAD_ATTR
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
            }
            break;
        case Pyc::LOAD_NAME_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::MAKE_CLOSURE_A:
        case Pyc::MAKE_FUNCTION_A:
            {
                PycRef<ASTNode> fun_code = stack.top();
                stack.pop();

                /* Test for the qualified name of the function (at TOS) */
                int tos_type = fun_code.cast<ASTObject>()->object().type();
                if (tos_type != PycObject::TYPE_CODE &&
                    tos_type != PycObject::TYPE_CODE2) {
                    fun_code = stack.top();
                    stack.pop();
                }

                ASTFunction::defarg_t defArgs, kwDefArgs;
                const int defCount = operand & 0xFF;
                const int kwDefCount = (operand >> 8) & 0xFF;
                for (int i = 0; i < defCount; ++i) {
                    defArgs.push_front(stack.top());
                    stack.pop();
                }
                for (int i = 0; i < kwDefCount; ++i) {
                    kwDefArgs.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTFunction(fun_code, defArgs, kwDefArgs));
            }
            break;
        case Pyc::NOP:
            break;
        case Pyc::POP_BLOCK:
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER ||
                        curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    /* These should only be popped by an END_FINALLY */
                    break;
                }

                if (curblock->blktype() == ASTBlock::BLK_WITH) {
                    // This should only be popped by a WITH_CLEANUP
                    break;
                }

                if (curblock->nodes().size() &&
                        curblock->nodes().back().type() == ASTNode::NODE_KEYWORD) {
                    curblock->removeLast();
                }

                if (curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELIF
                        || curblock->blktype() == ASTBlock::BLK_ELSE
                        || curblock->blktype() == ASTBlock::BLK_TRY
                        || curblock->blktype() == ASTBlock::BLK_EXCEPT
                        || curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    if (!stack_hist.empty()) {
                        stack = stack_hist.top();
                        stack_hist.pop();
                    } else {
                        fprintf(stderr, "Warning: Stack history is empty, something wrong might have happened\n");
                    }
                }
                PycRef<ASTBlock> tmp = curblock;
                blocks.pop();

                if (!blocks.empty())
                    curblock = blocks.top();

                if (!(tmp->blktype() == ASTBlock::BLK_ELSE
                        && tmp->nodes().size() == 0)) {
                    curblock->append(tmp.cast<ASTNode>());
                }

                if (tmp->blktype() == ASTBlock::BLK_FOR && tmp->end() >= pos) {
                    stack_hist.push(stack);

                    PycRef<ASTBlock> blkelse = new ASTBlock(ASTBlock::BLK_ELSE, tmp->end());
                    blocks.push(blkelse);
                    curblock = blocks.top();
                }

                if (curblock->blktype() == ASTBlock::BLK_TRY
                        && tmp->blktype() != ASTBlock::BLK_FOR
                        && tmp->blktype() != ASTBlock::BLK_ASYNCFOR
                        && tmp->blktype() != ASTBlock::BLK_WHILE) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    tmp = curblock;
                    blocks.pop();
                    curblock = blocks.top();

                    if (!(tmp->blktype() == ASTBlock::BLK_ELSE
                            && tmp->nodes().size() == 0)) {
                        curblock->append(tmp.cast<ASTNode>());
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();

                    if (tmp->blktype() == ASTBlock::BLK_ELSE && !cont->hasFinally()) {

                        /* Pop the container */
                        blocks.pop();
                        curblock = blocks.top();
                        curblock->append(cont.cast<ASTNode>());

                    } else if ((tmp->blktype() == ASTBlock::BLK_ELSE && cont->hasFinally())
                            || (tmp->blktype() == ASTBlock::BLK_TRY && !cont->hasExcept())) {

                        /* Add the finally block */
                        stack_hist.push(stack);

                        PycRef<ASTBlock> final = new ASTBlock(ASTBlock::BLK_FINALLY, 0, true);
                        blocks.push(final);
                        curblock = blocks.top();
                    }
                }

                if ((curblock->blktype() == ASTBlock::BLK_FOR || curblock->blktype() == ASTBlock::BLK_ASYNCFOR)
                        && curblock->end() == pos) {
                    blocks.pop();
                    blocks.top()->append(curblock.cast<ASTNode>());
                    curblock = blocks.top();
                }
            }
            break;
        case Pyc::POP_EXCEPT:
            /* Do nothing. */
            break;
        case Pyc::POP_TOP:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                if (!curblock->inited()) {
                    if (curblock->blktype() == ASTBlock::BLK_WITH) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                    } else {
                        curblock->init();
                    }
                    break;
                } else if (value == nullptr || value->processed()) {
                    break;
                }

                curblock->append(value);

                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()) {
                    /* This relies on some really uncertain logic...
                     * If it's a comprehension, the only POP_TOP should be
                     * a call to append the iter to the list.
                     */
                    if (value.type() == ASTNode::NODE_CALL) {
                        PycRef<ASTNode> res = value.cast<ASTCall>()->pparams().front();

                        stack.push(new ASTComprehension(res));
                    }
                }
            }
            break;
        case Pyc::PRINT_ITEM:
            {
                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().cast<ASTPrint>();
                if (printNode && printNode->stream() == nullptr && !printNode->eol())
                    printNode->add(stack.top());
                else
                    curblock->append(new ASTPrint(stack.top()));
                stack.pop();
            }
            break;
        case Pyc::PRINT_ITEM_TO:
            {
                PycRef<ASTNode> stream = stack.top();
                stack.pop();

                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().cast<ASTPrint>();
                if (printNode && printNode->stream() == stream && !printNode->eol())
                    printNode->add(stack.top());
                else
                    curblock->append(new ASTPrint(stack.top(), stream));
                stack.pop();
                stream->setProcessed();
            }
            break;
        case Pyc::PRINT_NEWLINE:
            {
                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().cast<ASTPrint>();
                if (printNode && printNode->stream() == nullptr && !printNode->eol())
                    printNode->setEol(true);
                else
                    curblock->append(new ASTPrint(nullptr));
                stack.pop();
            }
            break;
        case Pyc::PRINT_NEWLINE_TO:
            {
                PycRef<ASTNode> stream = stack.top();
                stack.pop();

                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().cast<ASTPrint>();
                if (printNode && printNode->stream() == stream && !printNode->eol())
                    printNode->setEol(true);
                else
                    curblock->append(new ASTPrint(nullptr, stream));
                stack.pop();
                stream->setProcessed();
            }
            break;
        case Pyc::RAISE_VARARGS_A:
            {
                ASTRaise::param_t paramList;
                for (int i = 0; i < operand; i++) {
                    paramList.push_front(stack.top());
                    stack.pop();
                }
                curblock->append(new ASTRaise(paramList));

                if ((curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELSE)
                        && stack_hist.size()
                        && (mod->verCompare(2, 6) >= 0)) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(prev.cast<ASTNode>());

                    bc_next(source, mod, opcode, operand, pos);
                }
            }
            break;
        case Pyc::RETURN_VALUE:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                curblock->append(new ASTReturn(value));

                if ((curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELSE)
                        && stack_hist.size()
                        && (mod->verCompare(2, 6) >= 0)) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(prev.cast<ASTNode>());

                    bc_next(source, mod, opcode, operand, pos);
                }
            }
            break;
        case Pyc::ROT_TWO:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> two = stack.top();
                stack.pop();

                stack.push(one);
                stack.push(two);
            }
            break;
        case Pyc::ROT_THREE:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                PycRef<ASTNode> two = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> three = stack.top();
                stack.pop();
                stack.push(one);
                stack.push(three);
                stack.push(two);
            }
            break;
        case Pyc::ROT_FOUR:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                PycRef<ASTNode> two = stack.top();
                stack.pop();
                PycRef<ASTNode> three = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> four = stack.top();
                stack.pop();
                stack.push(one);
                stack.push(four);
                stack.push(three);
                stack.push(two);
            }
            break;
        case Pyc::SET_LINENO_A:
            // Ignore
            break;
        case Pyc::SETUP_WITH_A:
            {
                PycRef<ASTBlock> withblock = new ASTWithBlock(pos+operand);
                blocks.push(withblock);
                curblock = blocks.top();
            }
            break;
        case Pyc::WITH_CLEANUP:
            {
                // Stack top should be a None. Ignore it.
                PycRef<ASTNode> none = stack.top();
                stack.pop();

                if (none != NULL) {
                    fprintf(stderr, "Something TERRIBLE happened!\n");
                    break;
                }

                if (curblock->blktype() == ASTBlock::BLK_WITH
                        && curblock->end() == curpos) {
                    PycRef<ASTBlock> with = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(with.cast<ASTNode>());
                }
                else {
                    fprintf(stderr, "Something TERRIBLE happened! No matching with block found for WITH_CLEANUP at %d\n", curpos);
                }
            }
            break;
        case Pyc::SETUP_EXCEPT_A:
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    curblock.cast<ASTContainerBlock>()->setExcept(pos+operand);
                } else {
                    PycRef<ASTBlock> next = new ASTContainerBlock(0, pos+operand);
                    blocks.push(next.cast<ASTBlock>());
                }

                /* Store the current stack for the except/finally statement(s) */
                stack_hist.push(stack);
                PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, pos+operand, true);
                blocks.push(tryblock.cast<ASTBlock>());
                curblock = blocks.top();

                need_try = false;
            }
            break;
        case Pyc::SETUP_FINALLY_A:
            {
                PycRef<ASTBlock> next = new ASTContainerBlock(pos+operand);
                blocks.push(next.cast<ASTBlock>());
                curblock = blocks.top();

                need_try = true;
            }
            break;
        case Pyc::SETUP_LOOP_A:
            {
                PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_WHILE, pos+operand, NULL, false);
                blocks.push(next.cast<ASTBlock>());
                curblock = blocks.top();
            }
            break;
        case Pyc::SLICE_0:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE0);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_1:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE1, lower);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_2:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE2, NULL, upper);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_3:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE3, lower, upper);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::STORE_ATTR_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = stack.top();
                    stack.pop();
                    PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(attr);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();
                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> name = stack.top();
                    stack.pop();
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);
                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, attr, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, attr));
                    }
                }
            }
            break;
        case Pyc::STORE_DEREF_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(operand));

                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }
            }
            break;
        case Pyc::STORE_FAST_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name;

                    if (mod->verCompare(1, 3) < 0)
                        name = new ASTName(code->getName(operand));
                    else
                        name = new ASTName(code->getVarName(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name;

                    if (mod->verCompare(1, 3) < 0)
                        name = new ASTName(code->getName(operand));
                    else
                        name = new ASTName(code->getVarName(operand));

                    if (name.cast<ASTName>()->name()->value()[0] == '_'
                            && name.cast<ASTName>()->name()->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                                   && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
                    } else if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }
            }
            break;
        case Pyc::STORE_GLOBAL_A:
            {
                PycRef<ASTNode> name = new ASTName(code->getName(operand));

                if (unpack) {
                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }

                /* Mark the global as used */
                code->markGlobal(name.cast<ASTName>()->name());
            }
            break;
        case Pyc::STORE_NAME_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = new ASTName(code->getName(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();

                    PycRef<PycString> varname = code->getName(operand);
                    if (varname->length() >= 2 && varname->value()[0] == '_'
                            && varname->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    // Return private names back to their original name
                    const std::string class_prefix = std::string("_") + code->name()->strValue();
                    if (varname->startsWith(class_prefix + std::string("__")))
                        varname->setValue(varname->strValue().substr(class_prefix.size()));

                    PycRef<ASTNode> name = new ASTName(varname);

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (stack.top().type() == ASTNode::NODE_IMPORT) {
                        PycRef<ASTImport> import = stack.top().cast<ASTImport>();

                        import->add_store(new ASTStore(value, name));
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                               && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
                    } else if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));

                        if (value.type() == ASTNode::NODE_INVALID)
                            break;
                    }
                }
            }
            break;
        case Pyc::STORE_SLICE_0:
            {
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE0))));
            }
            break;
        case Pyc::STORE_SLICE_1:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE1, upper))));
            }
            break;
        case Pyc::STORE_SLICE_2:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
            }
            break;
        case Pyc::STORE_SLICE_3:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
            }
            break;
        case Pyc::STORE_SUBSCR:
            {
                if (unpack) {
                    PycRef<ASTNode> subscr = stack.top();
                    stack.pop();
                    PycRef<ASTNode> dest = stack.top();
                    stack.pop();

                    PycRef<ASTNode> save = new ASTSubscr(dest, subscr);

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(save);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();
                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> subscr = stack.top();
                    stack.pop();
                    PycRef<ASTNode> dest = stack.top();
                    stack.pop();
                    PycRef<ASTNode> src = stack.top();
                    stack.pop();

                    // If variable annotations are enabled, we'll need to check for them here.
                    // Python handles a varaible annotation by setting:
                    // __annotations__['var-name'] = type
                    const bool found_annotated_var = (variable_annotations && dest->type() == ASTNode::Type::NODE_NAME
                                                      && dest.cast<ASTName>()->name()->isEqual("__annotations__"));

                    if (found_annotated_var) {
                        // Annotations can be done alone or as part of an assignment.
                        // In the case of an assignment, we'll see a NODE_STORE on the stack.
                        if (!curblock->nodes().empty() && curblock->nodes().back()->type() == ASTNode::Type::NODE_STORE) {
                            // Replace the existing NODE_STORE with a new one that includes the annotation.
                            PycRef<ASTStore> store = curblock->nodes().back().cast<ASTStore>();
                            curblock->removeLast();
                            curblock->append(new ASTStore(store->src(),
                                                          new ASTAnnotatedVar(subscr, src)));
                        } else {
                            curblock->append(new ASTAnnotatedVar(subscr, src));
                        }
                    } else {
                        if (dest.type() == ASTNode::NODE_MAP) {
                            dest.cast<ASTMap>()->add(subscr, src);
                        } else if (src.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(src, new ASTSubscr(dest, subscr), stack, curblock);
                        } else {
                            curblock->append(new ASTStore(src, new ASTSubscr(dest, subscr)));
                        }
                    }
                }
            }
            break;
        case Pyc::UNARY_CALL:
            {
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, ASTCall::pparam_t(), ASTCall::kwparam_t()));
            }
            break;
        case Pyc::UNARY_CONVERT:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTConvert(name));
            }
            break;
        case Pyc::UNARY_INVERT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_INVERT));
            }
            break;
        case Pyc::UNARY_NEGATIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NEGATIVE));
            }
            break;
        case Pyc::UNARY_NOT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NOT));
            }
            break;
        case Pyc::UNARY_POSITIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_POSITIVE));
            }
            break;
        case Pyc::UNPACK_LIST_A:
        case Pyc::UNPACK_TUPLE_A:
        case Pyc::UNPACK_SEQUENCE_A:
            {
                unpack = operand;
                if (unpack > 0) {
                    ASTTuple::value_t vals;
                    stack.push(new ASTTuple(vals));
                } else {
                    // Unpack zero values and assign it to top of stack or for loop variable.
                    // E.g. [] = TOS / for [] in X
                    ASTTuple::value_t vals;
                    auto tup = new ASTTuple(vals);
                    if (curblock->blktype() == ASTBlock::BLK_FOR
                        && !curblock->inited()) {
                        tup->setRequireParens(true);
                        curblock.cast<ASTIterBlock>()->setIndex(tup);
                    } else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                        auto chainStore = stack.top();
                        stack.pop();
                        append_to_chain_store(chainStore, tup, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(stack.top(), tup));
                        stack.pop();
                    }
                }
            }
            break;
        case Pyc::YIELD_FROM:
            {
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                // TODO: Support yielding into a non-null destination
                PycRef<ASTNode> value = stack.top();
                if (value) {
                    value->setProcessed();
                    curblock->append(new ASTReturn(value, ASTReturn::YIELD_FROM));
                }
            }
            break;
        case Pyc::YIELD_VALUE:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                curblock->append(new ASTReturn(value, ASTReturn::YIELD));
            }
            break;
        case Pyc::SETUP_ANNOTATIONS:
            variable_annotations = true;
            break;
        default:
            fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
            cleanBuild = false;
            return new ASTNodeList(defblock->nodes());
        }

        else_pop =  ( (curblock->blktype() == ASTBlock::BLK_ELSE)
                      || (curblock->blktype() == ASTBlock::BLK_IF)
                      || (curblock->blktype() == ASTBlock::BLK_ELIF) )
                 && (curblock->end() == pos);
    }

    if (stack_hist.size()) {
        fputs("Warning: Stack history is not empty!\n", stderr);

        while (stack_hist.size()) {
            stack_hist.pop();
        }
    }

    if (blocks.size() > 1) {
        fputs("Warning: block stack is not empty!\n", stderr);

        while (blocks.size() > 1) {
            PycRef<ASTBlock> tmp = blocks.top();
            blocks.pop();

            blocks.top()->append(tmp.cast<ASTNode>());
        }
    }

    cleanBuild = true;
    return new ASTNodeList(defblock->nodes());
}

static void append_to_chain_store(PycRef<ASTNode> chainStore, PycRef<ASTNode> item,
        FastStack& stack, PycRef<ASTBlock> curblock)
{
    stack.pop();    // ignore identical source object.
    chainStore.cast<ASTChainStore>()->append(item);
    if (stack.top().type() == PycObject::TYPE_NULL) {
        curblock->append(chainStore);
    } else {
        stack.push(chainStore);
    }
}

static int cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child)
{
    /* Determine whether the parent has higher precedence than therefore
       child, so we don't flood the source code with extraneous parens.
       Else we'd have expressions like (((a + b) + c) + d) when therefore
       equivalent, a + b + c + d would suffice. */

    if (parent.type() == ASTNode::NODE_UNARY && parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT)
        return 1;   // Always parenthesize not(x)
    if (child.type() == ASTNode::NODE_BINARY) {
        PycRef<ASTBinary> binChild = child.cast<ASTBinary>();
        if (parent.type() == ASTNode::NODE_BINARY)
            return binChild->op() - parent.cast<ASTBinary>()->op();
        else if (parent.type() == ASTNode::NODE_COMPARE)
            return (binChild->op() == ASTBinary::BIN_LOG_AND ||
                    binChild->op() == ASTBinary::BIN_LOG_OR) ? 1 : -1;
        else if (parent.type() == ASTNode::NODE_UNARY)
            return (binChild->op() == ASTBinary::BIN_POWER) ? -1 : 1;
    } else if (child.type() == ASTNode::NODE_UNARY) {
        PycRef<ASTUnary> unChild = child.cast<ASTUnary>();
        if (parent.type() == ASTNode::NODE_BINARY) {
            PycRef<ASTBinary> binParent = parent.cast<ASTBinary>();
            if (binParent->op() == ASTBinary::BIN_LOG_AND ||
                binParent->op() == ASTBinary::BIN_LOG_OR)
                return -1;
            else if (unChild->op() == ASTUnary::UN_NOT)
                return 1;
            else if (binParent->op() == ASTBinary::BIN_POWER)
                return 1;
            else
                return -1;
        } else if (parent.type() == ASTNode::NODE_COMPARE) {
            return (unChild->op() == ASTUnary::UN_NOT) ? 1 : -1;
        } else if (parent.type() == ASTNode::NODE_UNARY) {
            return unChild->op() - parent.cast<ASTUnary>()->op();
        }
    } else if (child.type() == ASTNode::NODE_COMPARE) {
        PycRef<ASTCompare> cmpChild = child.cast<ASTCompare>();
        if (parent.type() == ASTNode::NODE_BINARY)
            return (parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_AND ||
                    parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_OR) ? -1 : 1;
        else if (parent.type() == ASTNode::NODE_COMPARE)
            return cmpChild->op() - parent.cast<ASTCompare>()->op();
        else if (parent.type() == ASTNode::NODE_UNARY)
            return (parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT) ? -1 : 1;
    }

    /* For normal nodes, don't parenthesize anything */
    return -1;
}

static void print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child,
                          PycModule* mod)
{
    if (child.type() == ASTNode::NODE_BINARY ||
        child.type() == ASTNode::NODE_COMPARE) {
        if (cmp_prec(parent, child) > 0) {
            fputs("(", pyc_output);
            print_src(child, mod);
            fputs(")", pyc_output);
        } else {
            print_src(child, mod);
        }
    } else if (child.type() == ASTNode::NODE_UNARY) {
        if (cmp_prec(parent, child) > 0) {
            fputs("(", pyc_output);
            print_src(child, mod);
            fputs(")", pyc_output);
        } else {
            print_src(child, mod);
        }
    } else {
        print_src(child, mod);
    }
}

static void start_line(int indent)
{
    if (inLambda)
        return;
    for (int i=0; i<indent; i++)
        fputs("    ", pyc_output);
}

static void end_line()
{
    if (inLambda)
        return;
    fputs("\n", pyc_output);
}

int cur_indent = -1;
static void print_block(PycRef<ASTBlock> blk, PycModule* mod) {
    ASTBlock::list_t lines = blk->nodes();

    if (lines.size() == 0) {
        PycRef<ASTNode> pass = new ASTKeyword(ASTKeyword::KW_PASS);
        start_line(cur_indent);
        print_src(pass, mod);
    }

    for (auto ln = lines.cbegin(); ln != lines.cend();) {
        if ((*ln).cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
            start_line(cur_indent);
        }
        print_src(*ln, mod);
        if (++ln != lines.end()) {
            end_line();
        }
    }
}

void print_formatted_value(PycRef<ASTFormattedValue> formatted_value, PycModule* mod)
{
    fputs("{", pyc_output);
    print_src(formatted_value->val(), mod);

    switch (formatted_value->conversion()) {
    case ASTFormattedValue::ConversionFlag::NONE:
        break;
    case ASTFormattedValue::ConversionFlag::STR:
        fputs("!s", pyc_output);
        break;
    case ASTFormattedValue::ConversionFlag::REPR:
        fputs("!r", pyc_output);
        break;
    case ASTFormattedValue::ConversionFlag::ASCII:
        fputs("!a", pyc_output);
        break;
    case ASTFormattedValue::ConversionFlag::FMTSPEC:
        fprintf(pyc_output, ":%s", formatted_value->format_spec().cast<ASTObject>()->object().cast<PycString>()->value());
        break;
    default:
        fprintf(stderr, "Unsupported NODE_FORMATTEDVALUE conversion flag: %d\n", formatted_value->conversion());
    }
    fputs("}", pyc_output);
}

void print_src(PycRef<ASTNode> node, PycModule* mod)
{
    if (node == NULL) {
        fputs("None", pyc_output);
        cleanBuild = true;
        return;
    }

    switch (node->type()) {
    case ASTNode::NODE_BINARY:
    case ASTNode::NODE_COMPARE:
        {
            PycRef<ASTBinary> bin = node.cast<ASTBinary>();
            print_ordered(node, bin->left(), mod);
            fprintf(pyc_output, "%s", bin->op_str());
            print_ordered(node, bin->right(), mod);
        }
        break;
    case ASTNode::NODE_UNARY:
        {
            PycRef<ASTUnary> un = node.cast<ASTUnary>();
            fprintf(pyc_output, "%s", un->op_str());
            print_ordered(node, un->operand(), mod);
        }
        break;
    case ASTNode::NODE_CALL:
        {
            PycRef<ASTCall> call = node.cast<ASTCall>();
            print_src(call->func(), mod);
            fputs("(", pyc_output);
            bool first = true;
            for (const auto& param : call->pparams()) {
                if (!first)
                    fputs(", ", pyc_output);
                print_src(param, mod);
                first = false;
            }
            for (const auto& param : call->kwparams()) {
                if (!first)
                    fputs(", ", pyc_output);
                if (param.first.type() == ASTNode::NODE_NAME) {
                    fprintf(pyc_output, "%s = ", param.first.cast<ASTName>()->name()->value());
                } else {
                    PycRef<PycString> str_name = param.first.cast<ASTObject>()->object().require_cast<PycString>();
                    fprintf(pyc_output, "%s = ", str_name->value());
                }
                print_src(param.second, mod);
                first = false;
            }
            if (call->hasVar()) {
                if (!first)
                    fputs(", ", pyc_output);
                fputs("*", pyc_output);
                print_src(call->var(), mod);
                first = false;
            }
            if (call->hasKW()) {
                if (!first)
                    fputs(", ", pyc_output);
                fputs("**", pyc_output);
                print_src(call->kw(), mod);
                first = false;
            }
            fputs(")", pyc_output);
        }
        break;
    case ASTNode::NODE_DELETE:
        {
            fputs("del ", pyc_output);
            print_src(node.cast<ASTDelete>()->value(), mod);
        }
        break;
    case ASTNode::NODE_EXEC:
        {
            PycRef<ASTExec> exec = node.cast<ASTExec>();
            fputs("exec ", pyc_output);
            print_src(exec->statement(), mod);

            if (exec->globals() != NULL) {
                fputs(" in ", pyc_output);
                print_src(exec->globals(), mod);

                if (exec->locals() != NULL
                        && exec->globals() != exec->locals()) {
                    fputs(", ", pyc_output);
                    print_src(exec->locals(), mod);
                }
            }
        }
        break;
    case ASTNode::NODE_FORMATTEDVALUE:
        fputs("f" F_STRING_QUOTE, pyc_output);
        print_formatted_value(node.cast<ASTFormattedValue>(), mod);
        fputs(F_STRING_QUOTE, pyc_output);
        break;
    case ASTNode::NODE_JOINEDSTR:
        fputs("f" F_STRING_QUOTE, pyc_output);
        for (const auto& val : node.cast<ASTJoinedStr>()->values()) {
            switch (val.type()) {
            case ASTNode::NODE_FORMATTEDVALUE:
                print_formatted_value(val.cast<ASTFormattedValue>(), mod);
                break;
            case ASTNode::NODE_OBJECT:
                // When printing a piece of the f-string, keep the quote style consistent.
                // This avoids problems when ''' or """ is part of the string.
                print_const(val.cast<ASTObject>()->object(), mod, F_STRING_QUOTE);
                break;
            default:
                fprintf(stderr, "Unsupported node type %d in NODE_JOINEDSTR\n", val.type());
            }
        }
        fputs(F_STRING_QUOTE, pyc_output);
        break;
    case ASTNode::NODE_KEYWORD:
        fprintf(pyc_output, "%s", node.cast<ASTKeyword>()->word_str());
        break;
    case ASTNode::NODE_LIST:
        {
            fputs("[", pyc_output);
            bool first = true;
            cur_indent++;
            for (const auto& val : node.cast<ASTList>()->values()) {
                if (first)
                    fputs("\n", pyc_output);
                else
                    fputs(",\n", pyc_output);
                start_line(cur_indent);
                print_src(val, mod);
                first = false;
            }
            cur_indent--;
            fputs("]", pyc_output);
        }
        break;
    case ASTNode::NODE_COMPREHENSION:
        {
            PycRef<ASTComprehension> comp = node.cast<ASTComprehension>();

            fputs("[ ", pyc_output);
            print_src(comp->result(), mod);

            for (const auto& gen : comp->generators()) {
                fputs(" for ", pyc_output);
                print_src(gen->index(), mod);
                fputs(" in ", pyc_output);
                print_src(gen->iter(), mod);
                if (gen->condition()) {
                    fprintf(pyc_output, " if ");
                    print_src(gen->condition(), mod);
                }
            }
            fputs(" ]", pyc_output);
        }
        break;
    case ASTNode::NODE_MAP:
        {
            fputs("{", pyc_output);
            bool first = true;
            cur_indent++;
            for (const auto& val : node.cast<ASTMap>()->values()) {
                if (first)
                    fputs("\n", pyc_output);
                else
                    fputs(",\n", pyc_output);
                start_line(cur_indent);
                print_src(val.first, mod);
                fputs(": ", pyc_output);
                print_src(val.second, mod);
                first = false;
            }
            cur_indent--;
            fputs(" }", pyc_output);
        }
        break;
    case ASTNode::NODE_CONST_MAP:
        {
            PycRef<ASTConstMap> const_map = node.cast<ASTConstMap>();
            PycTuple::value_t keys = const_map->keys().cast<ASTObject>()->object().cast<PycTuple>()->values();
            ASTConstMap::values_t values = const_map->values();

            auto map = new ASTMap;
            for (const auto& key : keys) {
                // Values are pushed onto the stack in reverse order.
                PycRef<ASTNode> value = values.back();
                values.pop_back();

                map->add(new ASTObject(key), value);
            }

            print_src(map, mod);
        }
        break;
    case ASTNode::NODE_NAME:
        fprintf(pyc_output, "%s", node.cast<ASTName>()->name()->value());
        break;
    case ASTNode::NODE_NODELIST:
        {
            cur_indent++;
            for (const auto& ln : node.cast<ASTNodeList>()->nodes()) {
                if (ln.cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
                    start_line(cur_indent);
                }
                print_src(ln, mod);
                end_line();
            }
            cur_indent--;
        }
        break;
    case ASTNode::NODE_BLOCK:
        {
            if (node.cast<ASTBlock>()->blktype() == ASTBlock::BLK_ELSE
                    && node.cast<ASTBlock>()->size() == 0)
                break;

            if (node.cast<ASTBlock>()->blktype() == ASTBlock::BLK_CONTAINER) {
                end_line();
                PycRef<ASTBlock> blk = node.cast<ASTBlock>();
                print_block(blk, mod);
                end_line();
                break;
            }

            fprintf(pyc_output, "%s", node.cast<ASTBlock>()->type_str());
            PycRef<ASTBlock> blk = node.cast<ASTBlock>();
            if (blk->blktype() == ASTBlock::BLK_IF
                    || blk->blktype() == ASTBlock::BLK_ELIF
                    || blk->blktype() == ASTBlock::BLK_WHILE) {
                if (blk.cast<ASTCondBlock>()->negative())
                    fputs(" not ", pyc_output);
                else
                    fputs(" ", pyc_output);

                print_src(blk.cast<ASTCondBlock>()->cond(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_FOR || blk->blktype() == ASTBlock::BLK_ASYNCFOR) {
                fputs(" ", pyc_output);
                print_src(blk.cast<ASTIterBlock>()->index(), mod);
                fputs(" in ", pyc_output);
                print_src(blk.cast<ASTIterBlock>()->iter(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_EXCEPT &&
                    blk.cast<ASTCondBlock>()->cond() != NULL) {
                fputs(" ", pyc_output);
                print_src(blk.cast<ASTCondBlock>()->cond(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_WITH) {
                fputs(" ", pyc_output);
                print_src(blk.cast<ASTWithBlock>()->expr(), mod);
                PycRef<ASTNode> var = blk.cast<ASTWithBlock>()->var();
                if (var != NULL) {
                    fputs(" as ", pyc_output);
                    print_src(var, mod);
                }
            }
            fputs(":\n", pyc_output);

            cur_indent++;
            print_block(blk, mod);
            cur_indent--;
        }
        break;
    case ASTNode::NODE_OBJECT:
        {
            PycRef<PycObject> obj = node.cast<ASTObject>()->object();
            if (obj.type() == PycObject::TYPE_CODE) {
                PycRef<PycCode> code = obj.cast<PycCode>();
                decompyle(code, mod);
            } else {
                print_const(obj, mod);
            }
        }
        break;
    case ASTNode::NODE_PRINT:
        {
            fputs("print ", pyc_output);
            bool first = true;
            if (node.cast<ASTPrint>()->stream() != nullptr) {
                fputs(">>", pyc_output);
                print_src(node.cast<ASTPrint>()->stream(), mod);
                first = false;
            }

            for (const auto& val : node.cast<ASTPrint>()->values()) {
                if (!first)
                    fputs(", ", pyc_output);
                print_src(val, mod);
                first = false;
            }
            if (!node.cast<ASTPrint>()->eol())
                fputs(",", pyc_output);
        }
        break;
    case ASTNode::NODE_RAISE:
        {
            PycRef<ASTRaise> raise = node.cast<ASTRaise>();
            fputs("raise ", pyc_output);
            bool first = true;
            for (const auto& param : raise->params()) {
                if (!first)
                    fputs(", ", pyc_output);
                print_src(param, mod);
                first = false;
            }
        }
        break;
    case ASTNode::NODE_RETURN:
        {
            PycRef<ASTReturn> ret = node.cast<ASTReturn>();
            PycRef<ASTNode> value = ret->value();
            if (!inLambda) {
                switch (ret->rettype()) {
                case ASTReturn::RETURN:
                    fputs("return ", pyc_output);
                    break;
                case ASTReturn::YIELD:
                    fputs("yield ", pyc_output);
                    break;
                case ASTReturn::YIELD_FROM:
                    if (value.type() == ASTNode::NODE_AWAITABLE) {
                        fputs("await ", pyc_output);
                        value = value.cast<ASTAwaitable>()->expression();
                    } else {
                        fputs("yield from ", pyc_output);
                    }
                    break;
                }
            }
            print_src(value, mod);
        }
        break;
    case ASTNode::NODE_SLICE:
        {
            PycRef<ASTSlice> slice = node.cast<ASTSlice>();

            if (slice->op() & ASTSlice::SLICE1) {
                print_src(slice->left(), mod);
            }
            fputs(":", pyc_output);
            if (slice->op() & ASTSlice::SLICE2) {
                print_src(slice->right(), mod);
            }
        }
        break;
    case ASTNode::NODE_IMPORT:
        {
            PycRef<ASTImport> import = node.cast<ASTImport>();
            if (import->stores().size()) {
                ASTImport::list_t stores = import->stores();

                fputs("from ", pyc_output);
                if (import->name().type() == ASTNode::NODE_IMPORT)
                    print_src(import->name().cast<ASTImport>()->name(), mod);
                else
                    print_src(import->name(), mod);
                fputs(" import ", pyc_output);

                if (stores.size() == 1) {
                    auto src = stores.front()->src();
                    auto dest = stores.front()->dest();
                    print_src(src, mod);

                    if (src.cast<ASTName>()->name()->value() != dest.cast<ASTName>()->name()->value()) {
                        fputs(" as ", pyc_output);
                        print_src(dest, mod);
                    }
                } else {
                    bool first = true;
                    for (const auto& st : stores) {
                        if (!first)
                            fputs(", ", pyc_output);
                        print_src(st->src(), mod);
                        first = false;

                        if (st->src().cast<ASTName>()->name()->value() != st->dest().cast<ASTName>()->name()->value()) {
                            fputs(" as ", pyc_output);
                            print_src(st->dest(), mod);
                        }
                    }
                }
            } else {
                fputs("import ", pyc_output);
                print_src(import->name(), mod);
            }
        }
        break;
    case ASTNode::NODE_FUNCTION:
        {
            /* Actual named functions are NODE_STORE with a name */
            fputs("(lambda ", pyc_output);
            PycRef<ASTNode> code = node.cast<ASTFunction>()->code();
            PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
            ASTFunction::defarg_t defargs = node.cast<ASTFunction>()->defargs();
            ASTFunction::defarg_t kwdefargs = node.cast<ASTFunction>()->kwdefargs();
            auto da = defargs.cbegin();
            int narg = 0;
            for (int i=0; i<code_src->argCount(); i++) {
                if (narg)
                    fputs(", ", pyc_output);
                fprintf(pyc_output, "%s", code_src->getVarName(narg++)->value());
                if ((code_src->argCount() - i) <= (int)defargs.size()) {
                    fputs(" = ", pyc_output);
                    print_src(*da++, mod);
                }
            }
            da = kwdefargs.cbegin();
            if (code_src->kwOnlyArgCount() != 0) {
                fputs(narg == 0 ? "*" : ", *", pyc_output);
                for (int i = 0; i < code_src->argCount(); i++) {
                    fputs(", ", pyc_output);
                    fprintf(pyc_output, "%s", code_src->getVarName(narg++)->value());
                    if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
                        fputs(" = ", pyc_output);
                        print_src(*da++, mod);
                    }
                }
            }
            fputs(": ", pyc_output);

            inLambda = true;
            print_src(code, mod);
            inLambda = false;

            fputs(")", pyc_output);
        }
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            if (src.type() == ASTNode::NODE_FUNCTION) {
                PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
                PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                bool isLambda = false;

                if (strcmp(code_src->name()->value(), "<lambda>") == 0) {
                    fputs("\n", pyc_output);
                    start_line(cur_indent);
                    print_src(dest, mod);
                    fputs(" = lambda ", pyc_output);
                    isLambda = true;
                } else {
                    fputs("\n", pyc_output);
                    start_line(cur_indent);
                    if (code_src->flags() & PycCode::CO_COROUTINE)
                        fputs("async ", pyc_output);
                    fputs("def ", pyc_output);
                    print_src(dest, mod);
                    fputs("(", pyc_output);
                }

                ASTFunction::defarg_t defargs = src.cast<ASTFunction>()->defargs();
                ASTFunction::defarg_t kwdefargs = src.cast<ASTFunction>()->kwdefargs();
                auto da = defargs.cbegin();
                int narg = 0;
                for (int i = 0; i < code_src->argCount(); ++i) {
                    if (narg)
                        fputs(", ", pyc_output);
                    fprintf(pyc_output, "%s", code_src->getVarName(narg++)->value());
                    if ((code_src->argCount() - i) <= (int)defargs.size()) {
                        fputs(" = ", pyc_output);
                        print_src(*da++, mod);
                    }
                }
                da = kwdefargs.cbegin();
                if (code_src->kwOnlyArgCount() != 0) {
                    fputs(narg == 0 ? "*" : ", *", pyc_output);
                    for (int i = 0; i < code_src->kwOnlyArgCount(); ++i) {
                        fputs(", ", pyc_output);
                        fprintf(pyc_output, "%s", code_src->getVarName(narg++)->value());
                        if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
                            fputs(" = ", pyc_output);
                            print_src(*da++, mod);
                        }
                    }
                }
                if (code_src->flags() & PycCode::CO_VARARGS) {
                    if (narg)
                        fputs(", ", pyc_output);
                    fprintf(pyc_output, "*%s", code_src->getVarName(narg++)->value());
                }
                if (code_src->flags() & PycCode::CO_VARKEYWORDS) {
                    if (narg)
                        fputs(", ", pyc_output);

                    int idx = code_src->argCount();
                    if (code_src->flags() & PycCode::CO_VARARGS) {
                        idx++;
                    }
                    fprintf(pyc_output, "**%s", code_src->getVarName(narg++)->value());
                }

                if (isLambda) {
                    fputs(": ", pyc_output);
                } else {
                    fputs("):\n", pyc_output);
                    printDocstringAndGlobals = true;
                }

                bool preLambda = inLambda;
                inLambda |= isLambda;

                print_src(code, mod);

                inLambda = preLambda;
            } else if (src.type() == ASTNode::NODE_CLASS) {
                fputs("\n", pyc_output);
                start_line(cur_indent);
                fputs("class ", pyc_output);
                print_src(dest, mod);
                PycRef<ASTTuple> bases = src.cast<ASTClass>()->bases().cast<ASTTuple>();
                if (bases->values().size() > 0) {
                    fputs("(", pyc_output);
                    bool first = true;
                    for (const auto& val : bases->values()) {
                        if (!first)
                            fputs(", ", pyc_output);
                        print_src(val, mod);
                        first = false;
                    }
                    fputs("):\n", pyc_output);
                } else {
                    // Don't put parens if there are no base classes
                    fputs(":\n", pyc_output);
                }
                printClassDocstring = true;
                PycRef<ASTNode> code = src.cast<ASTClass>()->code().cast<ASTCall>()
                                       ->func().cast<ASTFunction>()->code();
                print_src(code, mod);
            } else if (src.type() == ASTNode::NODE_IMPORT) {
                PycRef<ASTImport> import = src.cast<ASTImport>();
                if (import->fromlist() != NULL) {
                    PycRef<PycObject> fromlist = import->fromlist().cast<ASTObject>()->object();
                    if (fromlist != Pyc_None) {
                        fputs("from ", pyc_output);
                        if (import->name().type() == ASTNode::NODE_IMPORT)
                            print_src(import->name().cast<ASTImport>()->name(), mod);
                        else
                            print_src(import->name(), mod);
                        fputs(" import ", pyc_output);
                        if (fromlist.type() == PycObject::TYPE_TUPLE ||
                                fromlist.type() == PycObject::TYPE_SMALL_TUPLE) {
                            bool first = true;
                            for (const auto& val : fromlist.cast<PycTuple>()->values()) {
                                if (!first)
                                    fputs(", ", pyc_output);
                                fprintf(pyc_output, "%s", val.cast<PycString>()->value());
                                first = false;
                            }
                        } else {
                            fprintf(pyc_output, "%s", fromlist.cast<PycString>()->value());
                        }
                    } else {
                        fputs("import ", pyc_output);
                        print_src(import->name(), mod);
                    }
                } else {
                    fputs("import ", pyc_output);
                    PycRef<ASTNode> import_name = import->name();
                    print_src(import_name, mod);
                    if (!dest.cast<ASTName>()->name()->isEqual(import_name.cast<ASTName>()->name().cast<PycObject>())) {
                        fputs(" as ", pyc_output);
                        print_src(dest, mod);
                    }
                }
            } else if (src.type() == ASTNode::NODE_BINARY &&
                    src.cast<ASTBinary>()->is_inplace() == true) {
                print_src(src, mod);
            } else {
                print_src(dest, mod);
                fputs(" = ", pyc_output);
                print_src(src, mod);
            }
        }
        break;
    case ASTNode::NODE_CHAINSTORE:
        {
            for (auto& dest : node.cast<ASTChainStore>()->nodes()) {
                print_src(dest, mod);
                fputs(" = ", pyc_output);
            }
            print_src(node.cast<ASTChainStore>()->src(), mod);
        }
        break;
    case ASTNode::NODE_SUBSCR:
        {
            print_src(node.cast<ASTSubscr>()->name(), mod);
            fputs("[", pyc_output);
            print_src(node.cast<ASTSubscr>()->key(), mod);
            fputs("]", pyc_output);
        }
        break;
    case ASTNode::NODE_CONVERT:
        {
            fputs("`", pyc_output);
            print_src(node.cast<ASTConvert>()->name(), mod);
            fputs("`", pyc_output);
        }
        break;
    case ASTNode::NODE_TUPLE:
        {
            PycRef<ASTTuple> tuple = node.cast<ASTTuple>();
            ASTTuple::value_t values = tuple->values();
            if (tuple->requireParens())
                fputc('(', pyc_output);
            bool first = true;
            for (const auto& val : values) {
                if (!first)
                    fputs(", ", pyc_output);
                print_src(val, mod);
                first = false;
            }
            if (values.size() == 1)
                fputc(',', pyc_output);
            if (tuple->requireParens())
                fputc(')', pyc_output);
        }
        break;
    case ASTNode::NODE_ANNOTATED_VAR:
        {
            PycRef<ASTAnnotatedVar> annotated_var = node.cast<ASTAnnotatedVar>();
            PycRef<ASTObject> name = annotated_var->name().cast<ASTObject>();
            PycRef<ASTNode> type = annotated_var->type();

            fputs(name->object().cast<PycString>()->value(), pyc_output);
            fputs(": ", pyc_output);
            print_src(type, mod);
        }
        break;
    default:
        fprintf(pyc_output, "<NODE:%d>", node->type());
        fprintf(stderr, "Unsupported Node type: %d\n", node->type());
        cleanBuild = false;
        return;
    }

    cleanBuild = true;
}

bool print_docstring(PycRef<PycObject> obj, int indent, PycModule* mod)
{
    // docstrings are translated from the bytecode __doc__ = 'string' to simply '''string'''
    signed char prefix = -1;
    switch (obj.type()) {
    case PycObject::TYPE_STRING:
        prefix = mod->strIsUnicode() ? 'b' : 0;
        break;
    case PycObject::TYPE_UNICODE:
        prefix = mod->strIsUnicode() ? 0 : 'u';
        break;
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        if (mod->majorVer() >= 3)
            prefix = 0;
        else
            prefix = mod->strIsUnicode() ? 'b' : 0;
        break;
    }
    if (prefix != -1) {
        start_line(indent);
        OutputString(obj.cast<PycString>(), prefix, true);
        fputs("\n", pyc_output);
        return true;
    } else
        return false;
}

void decompyle(PycRef<PycCode> code, PycModule* mod)
{
    PycRef<ASTNode> source = BuildFromCode(code, mod);

    PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
    if (cleanBuild) {
        // The Python compiler adds some stuff that we don't really care
        // about, and would add extra code for re-compilation anyway.
        // We strip these lines out here, and then add a "pass" statement
        // if the cleaned up code is empty
        if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src().type() == ASTNode::NODE_NAME
                    && store->dest().type() == ASTNode::NODE_NAME) {
                PycRef<ASTName> src = store->src().cast<ASTName>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (src->name()->isEqual("__name__")
                        && dest->name()->isEqual("__module__")) {
                    // __module__ = __name__
                    // Automatically added by Python 2.2.1 and later
                    clean->removeFirst();
                }
            }
        }
        if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src().type() == ASTNode::NODE_OBJECT
                    && store->dest().type() == ASTNode::NODE_NAME) {
                PycRef<ASTObject> src = store->src().cast<ASTObject>();
                PycRef<PycString> srcString = src->object().cast<PycString>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (srcString != nullptr && srcString->isEqual(code->name().cast<PycObject>())
                        && dest->name()->isEqual("__qualname__")) {
                    // __qualname__ = '<Class Name>'
                    // Automatically added by Python 3.3 and later
                    clean->removeFirst();
                }
            }
        }

        // Class and module docstrings may only appear at the beginning of their source
        if (printClassDocstring && clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->dest().type() == ASTNode::NODE_NAME &&
                    store->dest().cast<ASTName>()->name()->isEqual("__doc__") &&
                    store->src().type() == ASTNode::NODE_OBJECT) {
                if (print_docstring(store->src().cast<ASTObject>()->object(),
                        cur_indent + (code->name()->isEqual("<module>") ? 0 : 1), mod))
                    clean->removeFirst();
            }
        }
        if (clean->nodes().back().type() == ASTNode::NODE_RETURN) {
            PycRef<ASTReturn> ret = clean->nodes().back().cast<ASTReturn>();

            if (ret->value() == NULL || ret->value().type() == ASTNode::NODE_LOCALS) {
                clean->removeLast();  // Always an extraneous return statement
            }
        }
    }
    if (printClassDocstring)
        printClassDocstring = false;
    // This is outside the clean check so a source block will always
    // be compilable, even if decompylation failed.
    if (clean->nodes().size() == 0 && !code.isIdent(mod->code()))
        clean->append(new ASTKeyword(ASTKeyword::KW_PASS));

    bool part1clean = cleanBuild;

    if (printDocstringAndGlobals) {
        if (code->consts()->size())
            print_docstring(code->getConst(0), cur_indent + 1, mod);

        PycCode::globals_t globs = code->getGlobals();
        if (globs.size()) {
            start_line(cur_indent + 1);
            fputs("global ", pyc_output);
            bool first = true;
            for (const auto& glob : globs) {
                if (!first)
                    fputs(", ", pyc_output);
                fprintf(pyc_output, "%s", glob->value());
                first = false;
            }
            fputs("\n", pyc_output);
        }
        printDocstringAndGlobals = false;
    }

    print_src(source, mod);

    if (!cleanBuild || !part1clean) {
        start_line(cur_indent);
        fputs("# WARNING: Decompyle incomplete\n", pyc_output);
    }
}
