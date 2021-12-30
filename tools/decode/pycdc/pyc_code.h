#ifndef _PYC_CODE_H
#define _PYC_CODE_H

#include "pyc_sequence.h"
#include "pyc_string.h"

class PycCode : public PycObject {
public:
    typedef std::list<PycRef<PycString> > globals_t;
    enum CodeFlags {
        CO_OPTIMIZED = 0x1,
        CO_NEWLOCALS = 0x2,
        CO_VARARGS = 0x4,
        CO_VARKEYWORDS = 0x8,
        CO_NESTED = 0x10,
        CO_GENERATOR = 0x20,
        CO_NOFREE = 0x40,
        CO_COROUTINE = 0x80,
        CO_ITERABLE_COROUTINE = 0x100,
        CO_GENERATOR_ALLOWED = 0x1000,
        CO_FUTURE_DIVISION = 0x2000,
        CO_FUTURE_ABSOLUTE_IMPORT = 0x4000,
        CO_FUTURE_WITH_STATEMENT = 0x8000,
        CO_FUTURE_PRINT_FUNCTION = 0x10000,
        CO_FUTURE_UNICODE_LITERALS = 0x20000,
        CO_FUTURE_BARRY_AS_BDFL = 0x40000,
        CO_FUTURE_GENERATOR_STOP = 0x80000,
    };

    PycCode(int type = TYPE_CODE)
        : PycObject(type), m_argCount(), m_posOnlyArgCount(), m_kwOnlyArgCount(),
          m_numLocals(), m_stackSize(), m_flags(), m_firstLine() { }

    void load(class PycData* stream, class PycModule* mod) override;

    int argCount() const { return m_argCount; }
    int posOnlyArgCount() const { return m_posOnlyArgCount; }
    int kwOnlyArgCount() const { return m_kwOnlyArgCount; }
    int numLocals() const { return m_numLocals; }
    int stackSize() const { return m_stackSize; }
    int flags() const { return m_flags; }
    PycRef<PycString> code() const { return m_code; }
    PycRef<PycSequence> consts() const { return m_consts; }
    PycRef<PycSequence> names() const { return m_names; }
    PycRef<PycSequence> varNames() const { return m_varNames; }
    PycRef<PycSequence> freeVars() const { return m_freeVars; }
    PycRef<PycSequence> cellVars() const { return m_cellVars; }
    PycRef<PycString> fileName() const { return m_fileName; }
    PycRef<PycString> name() const { return m_name; }
    int firstLine() const { return m_firstLine; }
    PycRef<PycString> lnTable() const { return m_lnTable; }

    PycRef<PycObject> getConst(int idx) const
    {
        return m_consts->get(idx);
    }

    PycRef<PycString> getName(int idx) const
    {
        return m_names->get(idx).require_cast<PycString>();
    }

    PycRef<PycString> getVarName(int idx) const
    {
        return m_varNames->get(idx).require_cast<PycString>();
    }

    PycRef<PycString> getCellVar(int idx) const
    {
        return (idx >= m_cellVars->size())
            ? m_freeVars->get(idx - m_cellVars->size()).require_cast<PycString>()
            : m_cellVars->get(idx).require_cast<PycString>();
    }

    const globals_t& getGlobals() const { return m_globalsUsed; }

    void markGlobal(PycRef<PycString> varname)
    {
        m_globalsUsed.push_back(varname);
        m_globalsUsed.unique();
    }

private:
    int m_argCount, m_posOnlyArgCount, m_kwOnlyArgCount, m_numLocals;
    int m_stackSize, m_flags;
    PycRef<PycString> m_code;
    PycRef<PycSequence> m_consts;
    PycRef<PycSequence> m_names;
    PycRef<PycSequence> m_varNames;
    PycRef<PycSequence> m_freeVars;
    PycRef<PycSequence> m_cellVars;
    PycRef<PycString> m_fileName;
    PycRef<PycString> m_name;
    int m_firstLine;
    PycRef<PycString> m_lnTable;
    globals_t m_globalsUsed; /* Global vars used in this code */
};

#endif
