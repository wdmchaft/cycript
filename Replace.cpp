/* Cycript - Inlining/Optimizing JavaScript Compiler
 * Copyright (C) 2009  Jay Freeman (saurik)
*/

/* Modified BSD License {{{ */
/*
 *        Redistribution and use in source and binary
 * forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation
 *    and/or other materials provided with the
 *    distribution.
 * 3. The name of the author may not be used to endorse
 *    or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* }}} */

#include "Parser.hpp"
#include "Replace.hpp"

#include <iomanip>

CYExpression *CYAdd::Replace(CYContext &context) {
    CYInfix::Replace(context);

    CYExpression *lhp(lhs_->Primitive(context));
    CYExpression *rhp(rhs_->Primitive(context));

    CYString *lhs(dynamic_cast<CYString *>(lhp));
    CYString *rhs(dynamic_cast<CYString *>(rhp));

    if (lhs != NULL || rhs != NULL) {
        if (lhs == NULL) {
            lhs = lhp->String(context);
            if (lhs == NULL)
                return this;
        } else if (rhs == NULL) {
            rhs = rhp->String(context);
            if (rhs == NULL)
                return this;
        }

        return lhs->Concat(context, rhs);
    }

    if (CYNumber *lhn = lhp->Number(context))
        if (CYNumber *rhn = rhp->Number(context))
            return $D(lhn->Value() + rhn->Value());

    return this;
}

CYExpression *CYAddressOf::Replace(CYContext &context) {
    CYPrefix::Replace(context);
    return $C0($M(rhs_, $S("$cya")));
}

void CYArgument::Replace(CYContext &context) { $T()
    context.Replace(value_);
    next_->Replace(context);
}

CYExpression *CYArray::Replace(CYContext &context) {
    elements_->Replace(context);
    return this;
}

CYExpression *CYArrayComprehension::Replace(CYContext &context) {
    CYVariable *cyv($V("$cyv"));

    return $C0($F(NULL, $P1("$cyv", comprehensions_->Parameters(context)), $$->*
        $E($ CYAssign(cyv, $ CYArray()))->*
        comprehensions_->Replace(context, $E($C1($M(cyv, $S("push")), expression_)))->*
        $ CYReturn(cyv)
    ));
}

CYExpression *CYAssignment::Replace(CYContext &context) {
    context.Replace(lhs_);
    context.Replace(rhs_);
    return this;
}

CYStatement *CYBlock::Replace(CYContext &context) {
    statements_ = statements_->ReplaceAll(context);
    if (statements_ == NULL)
        return $ CYEmpty();
    return this;
}

CYStatement *CYBreak::Replace(CYContext &context) {
    return this;
}

CYExpression *CYCall::Replace(CYContext &context) {
    context.Replace(function_);
    arguments_->Replace(context);
    return this;
}

namespace cy {
namespace Syntax {

void Catch::Replace(CYContext &context) { $T()
    code_.Replace(context);
}

} }

void CYClause::Replace(CYContext &context) { $T()
    context.Replace(case_);
    statements_ = statements_->ReplaceAll(context);
    next_->Replace(context);
}

CYStatement *CYComment::Replace(CYContext &context) {
    return this;
}

CYExpression *CYCompound::Replace(CYContext &context) {
    expressions_ = expressions_->ReplaceAll(context);
    return expressions_ == NULL ? NULL : this;
}

CYFunctionParameter *CYComprehension::Parameters(CYContext &context) const { $T(NULL)
    CYFunctionParameter *next(next_->Parameters(context));
    if (CYFunctionParameter *parameter = Parameter(context)) {
        parameter->SetNext(next);
        return parameter;
    } else
        return next;
}

CYStatement *CYComprehension::Replace(CYContext &context, CYStatement *statement) const {
    return next_ == NULL ? statement : next_->Replace(context, statement);
}

CYExpression *CYCondition::Replace(CYContext &context) {
    context.Replace(test_);
    context.Replace(true_);
    context.Replace(false_);
    return this;
}

CYStatement *CYContinue::Replace(CYContext &context) {
    return this;
}

CYAssignment *CYDeclaration::Assignment(CYContext &context) {
    CYExpression *variable(Replace(context));
    return initialiser_ == NULL ? NULL : $ CYAssign(variable, initialiser_);
}

CYExpression *CYDeclaration::ForEachIn(CYContext &context) {
    return $ CYVariable(identifier_);
}

CYExpression *CYDeclaration::Replace(CYContext &context) {
    context.Replace(identifier_);
    context.scope_->Declare(context, identifier_, CYIdentifierVariable);
    return $ CYVariable(identifier_);
}

CYProperty *CYDeclarations::Property(CYContext &context) { $T(NULL)
    return $ CYProperty(declaration_->identifier_, declaration_->initialiser_ ?: $U, next_->Property(context));
}

CYCompound *CYDeclarations::Replace(CYContext &context) {
    CYCompound *compound;
    if (next_ == NULL) compound:
        compound = $ CYCompound();
    else {
        compound = next_->Replace(context);
        if (compound == NULL)
            goto compound;
    }

    if (CYAssignment *assignment = declaration_->Assignment(context))
        compound->AddPrev(assignment);
    return compound;
}

CYExpression *CYDirectMember::Replace(CYContext &context) {
    Replace_(context);
    return this;
}

CYStatement *CYDoWhile::Replace(CYContext &context) {
    context.Replace(test_);
    context.Replace(code_);
    return this;
}

void CYElement::Replace(CYContext &context) { $T()
    context.Replace(value_);
    next_->Replace(context);
}

CYStatement *CYEmpty::Collapse(CYContext &context) {
    return next_;
}

CYStatement *CYEmpty::Replace(CYContext &context) {
    return this;
}

CYStatement *CYExpress::Collapse(CYContext &context) {
    if (CYExpress *express = dynamic_cast<CYExpress *>(next_)) {
        CYCompound *next(dynamic_cast<CYCompound *>(express->expression_));
        if (next == NULL)
            next = $ CYCompound(express->expression_);
        next->AddPrev(expression_);
        expression_ = next;
        SetNext(express->next_);
    }

    return this;
}

CYStatement *CYExpress::Replace(CYContext &context) {
    context.Replace(expression_);
    if (expression_ == NULL)
        return $ CYEmpty();
    return this;
}

CYExpression *CYExpression::ClassName(CYContext &context, bool object) {
    return this;
}

CYExpression *CYExpression::ForEachIn(CYContext &context) {
    return this;
}

CYExpression *CYExpression::ReplaceAll(CYContext &context) { $T(NULL)
    CYExpression *replace(this);
    context.Replace(replace);

    if (CYExpression *next = next_->ReplaceAll(context))
        replace->SetNext(next);
    else
        replace->SetNext(next_);

    return replace;
}

CYNumber *CYFalse::Number(CYContext &context) {
    return $D(0);
}

CYString *CYFalse::String(CYContext &context) {
    return $S("false");
}

void CYFinally::Replace(CYContext &context) { $T()
    code_.Replace(context);
}

CYStatement *CYFor::Replace(CYContext &context) {
    context.Replace(initialiser_);
    context.Replace(test_);
    context.Replace(increment_);
    context.Replace(code_);
    return this;
}

CYStatement *CYForIn::Replace(CYContext &context) {
    // XXX: this actually might need a prefix statement
    context.Replace(initialiser_);
    context.Replace(set_);
    context.Replace(code_);
    return this;
}

CYFunctionParameter *CYForInComprehension::Parameter(CYContext &context) const {
    return $ CYFunctionParameter(name_);
}

CYStatement *CYForInComprehension::Replace(CYContext &context, CYStatement *statement) const {
    return $ CYForIn($ CYVariable(name_), set_, CYComprehension::Replace(context, statement));
}

CYStatement *CYForEachIn::Replace(CYContext &context) {
    CYVariable *cys($V("$cys")), *cyt($V("$cyt"));

    return $ CYLet($L2($L($I("$cys"), set_), $L($I("$cyt"))), $$->*
        $ CYForIn(cyt, cys, $ CYBlock($$->*
            $E($ CYAssign(initialiser_->ForEachIn(context), $M(cys, cyt)))->*
            code_
        ))
    );
}

CYFunctionParameter *CYForEachInComprehension::Parameter(CYContext &context) const {
    return $ CYFunctionParameter(name_);
}

CYStatement *CYForEachInComprehension::Replace(CYContext &context, CYStatement *statement) const {
    CYVariable *cys($V("$cys")), *name($ CYVariable(name_));

    return $E($C0($F(NULL, $P1("$cys"), $$->*
        $E($ CYAssign(cys, set_))->*
        $ CYForIn(name, cys, $ CYBlock($$->*
            $E($ CYAssign(name, $M(cys, name)))->*
            CYComprehension::Replace(context, statement)
        ))
    )));
}

void CYFunction::Inject(CYContext &context) {
    context.Replace(name_);
    context.scope_->Declare(context, name_, CYIdentifierOther);
}

void CYFunction::Replace_(CYContext &context, bool outer) {
    if (outer)
        Inject(context);

    parent_ = context.scope_;
    context.scope_ = this;

    if (!outer && name_ != NULL)
        Inject(context);

    parameters_->Replace(context);
    code_.Replace(context);

    context.scope_ = parent_;
    Scope(context, code_.statements_);
}

CYExpression *CYFunctionExpression::Replace(CYContext &context) {
    Replace_(context, false);
    return this;
}

void CYFunctionParameter::Replace(CYContext &context) { $T()
    name_ = name_->Replace(context);
    context.scope_->Declare(context, name_, CYIdentifierArgument);
    next_->Replace(context);
}

CYStatement *CYFunctionStatement::Replace(CYContext &context) {
    Replace_(context, true);
    return this;
}

CYIdentifier *CYIdentifier::Replace(CYContext &context) {
    if (replace_ == NULL) {
        replace_ = context.scope_->Lookup(context, this);
        ++replace_->usage_;
    } else if (replace_ != this)
        return replace_->Replace(context);
    return replace_;
}

CYStatement *CYIf::Replace(CYContext &context) {
    context.Replace(test_);
    context.Replace(true_);
    context.Replace(false_);
    return this;
}

CYFunctionParameter *CYIfComprehension::Parameter(CYContext &context) const {
    return NULL;
}

CYStatement *CYIfComprehension::Replace(CYContext &context, CYStatement *statement) const {
    return $ CYIf(test_, CYComprehension::Replace(context, statement));
}

CYExpression *CYIndirect::Replace(CYContext &context) {
    CYPrefix::Replace(context);
    return $M(rhs_, $S("$cyi"));
}

CYExpression *CYIndirectMember::Replace(CYContext &context) {
    Replace_(context);
    return $M($ CYIndirect(object_), property_);
}

CYExpression *CYInfix::Replace(CYContext &context) {
    context.Replace(lhs_);
    context.Replace(rhs_);
    return this;
}

CYStatement *CYLabel::Replace(CYContext &context) {
    context.Replace(statement_);
    return this;
}

CYStatement *CYLet::Replace(CYContext &context) {
    return $ CYWith($ CYObject(declarations_->Property(context)), &code_);
}

void CYMember::Replace_(CYContext &context) {
    context.Replace(object_);
    context.Replace(property_);
}

CYExpression *CYNew::Replace(CYContext &context) {
    context.Replace(constructor_);
    arguments_->Replace(context);
    return this;
}

CYNumber *CYNull::Number(CYContext &context) {
    return $D(0);
}

CYString *CYNull::String(CYContext &context) {
    return $S("null");
}

CYNumber *CYNumber::Number(CYContext &context) {
    return this;
}

CYString *CYNumber::String(CYContext &context) {
    // XXX: there is a precise algorithm for this
    return $S(apr_psprintf(context.pool_, "%.17g", Value()));
}

CYExpression *CYObject::Replace(CYContext &context) {
    properties_->Replace(context);
    return this;
}

CYExpression *CYPostfix::Replace(CYContext &context) {
    context.Replace(lhs_);
    return this;
}

CYExpression *CYPrefix::Replace(CYContext &context) {
    context.Replace(rhs_);
    return this;
}

// XXX: this is evil evil black magic. don't ask, don't tell... don't believe!
#define MappingSet "0etnirsoalfucdphmgyvbxTwSNECAFjDLkMOIBPqzRH$_WXUVGYKQJZ"
//#define MappingSet "0abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$_"

namespace {
    struct IdentifierUsageLess :
        std::binary_function<CYIdentifier *, CYIdentifier *, bool>
    {
        _finline bool operator ()(CYIdentifier *lhs, CYIdentifier *rhs) const {
            if (lhs->usage_ != rhs->usage_)
                return lhs->usage_ > rhs->usage_;
            return lhs < rhs;
        }
    };

    typedef std::set<CYIdentifier *, IdentifierUsageLess> IdentifierUsages;
}

void CYProgram::Replace(CYContext &context) {
    parent_ = context.scope_;
    CYProgram *program(context.program_);

    context.scope_ = this;
    context.program_ = this;

    statements_ = statements_->ReplaceAll(context);

    context.scope_ = parent_;
    context.program_ = program;
    Scope(context, statements_);

    size_t offset(0);

    CYCStringSet external;
    for (CYIdentifierValueSet::const_iterator i(identifiers_.begin()); i != identifiers_.end(); ++i)
        external.insert((*i)->Word());

    IdentifierUsages usages;

    if (offset < rename_.size())
        for (CYIdentifier *i(rename_[offset].identifier_); i != NULL; i = i->next_)
            usages.insert(i);

    // XXX: totalling the probable occurrences and sorting by them would improve the result
    for (CYIdentifierUsageVector::const_iterator i(rename_.begin()); i != rename_.end(); ++i, ++offset) {
        //std::cout << *i << ":" << (*i)->offset_ << std::endl;

        const char *name;

        if (context.options_.verbose_)
            name = apr_psprintf(context.pool_, "$%"APR_SIZE_T_FMT"", offset);
        else {
            char id[8];
            id[7] = '\0';

          id:
            unsigned position(7), local(offset + 1);

            do {
                unsigned index(local % (sizeof(MappingSet) - 1));
                local /= sizeof(MappingSet) - 1;
                id[--position] = MappingSet[index];
            } while (local != 0);

            if (external.find(id + position) != external.end()) {
                ++offset;
                goto id;
            }

            name = apr_pstrmemdup(context.pool_, id + position, 7 - position);
            // XXX: at some point, this could become a keyword
        }

        for (CYIdentifier *identifier(i->identifier_); identifier != NULL; identifier = identifier->next_)
            identifier->Set(name);
    }
}

void CYProperty::Replace(CYContext &context) { $T()
    context.Replace(value_);
    next_->Replace(context);
}

CYStatement *CYReturn::Replace(CYContext &context) {
    context.Replace(value_);
    return this;
}

void CYScope::Declare(CYContext &context, CYIdentifier *identifier, CYIdentifierFlags flags) {
    internal_.insert(CYIdentifierAddressFlagsMap::value_type(identifier, flags));
}

CYIdentifier *CYScope::Lookup(CYContext &context, CYIdentifier *identifier) {
    std::pair<CYIdentifierValueSet::iterator, bool> insert(identifiers_.insert(identifier));
    return *insert.first;
}

void CYScope::Merge(CYContext &context, CYIdentifier *identifier) {
    std::pair<CYIdentifierValueSet::iterator, bool> insert(identifiers_.insert(identifier));
    if (!insert.second) {
        if ((*insert.first)->offset_ < identifier->offset_)
            (*insert.first)->offset_ = identifier->offset_;
        identifier->replace_ = *insert.first;
        (*insert.first)->usage_ += identifier->usage_ + 1;
    }
}

namespace {
    struct IdentifierOffset {
        size_t offset_;
        CYIdentifierFlags flags_;
        size_t usage_;
        CYIdentifier *identifier_;

        IdentifierOffset(CYIdentifier *identifier, CYIdentifierFlags flags) :
            offset_(identifier->offset_),
            flags_(flags),
            usage_(identifier->usage_),
            identifier_(identifier)
        {
        }
    };

    struct IdentifierOffsetLess :
        std::binary_function<const IdentifierOffset &, const IdentifierOffset &, bool>
    {
        _finline bool operator ()(const IdentifierOffset &lhs, const IdentifierOffset &rhs) const {
            if (lhs.offset_ != rhs.offset_)
                return lhs.offset_ < rhs.offset_;
            if (lhs.flags_ != rhs.flags_)
                return lhs.flags_ < rhs.flags_;
            /*if (lhs.usage_ != rhs.usage_)
                return lhs.usage_ > rhs.usage_;*/
            return lhs.identifier_ < rhs.identifier_;
        }
    };

    typedef std::set<IdentifierOffset, IdentifierOffsetLess> IdentifierOffsets;
}

void CYScope::Scope(CYContext &context, CYStatement *&statements) {
    CYDeclarations *last(NULL), *curr(NULL);
    CYProgram *program(context.program_);

    IdentifierOffsets offsets;

    for (CYIdentifierAddressFlagsMap::const_iterator i(internal_.begin()); i != internal_.end(); ++i)
        if (program != NULL && i->second != CYIdentifierMagic)
            offsets.insert(IdentifierOffset(i->first, i->second));

    size_t offset(0);

    for (IdentifierOffsets::const_iterator i(offsets.begin()); i != offsets.end(); ++i) {
        if (i->flags_ == CYIdentifierVariable) {
            CYDeclarations *next($ CYDeclarations($ CYDeclaration(i->identifier_)));
            if (last == NULL)
                last = next;
            if (curr != NULL)
                curr->SetNext(next);
            curr = next;
        }

        if (offset < i->offset_)
            offset = i->offset_;
        if (program->rename_.size() <= offset)
            program->rename_.resize(offset + 1);

        CYIdentifierUsage &rename(program->rename_[offset++]);
        i->identifier_->SetNext(rename.identifier_);
        rename.identifier_ = i->identifier_;
        rename.usage_ += i->identifier_->usage_ + 1;
    }

    if (last != NULL) {
        CYVar *var($ CYVar(last));
        var->SetNext(statements);
        statements = var;
    }

    for (CYIdentifierValueSet::const_iterator i(identifiers_.begin()); i != identifiers_.end(); ++i)
        if (internal_.find(*i) == internal_.end()) {
            //std::cout << *i << '=' << offset << std::endl;
            if ((*i)->offset_ < offset)
                (*i)->offset_ = offset;
            parent_->Merge(context, *i);
        }
}

CYStatement *CYStatement::Collapse(CYContext &context) {
    return this;
}

CYStatement *CYStatement::ReplaceAll(CYContext &context) { $T(NULL)
    CYStatement *replace(this);
    context.Replace(replace);
    replace->SetNext(next_->ReplaceAll(context));
    return replace->Collapse(context);
}

CYString *CYString::Concat(CYContext &context, CYString *rhs) const {
    size_t size(size_ + rhs->size_);
    char *value(new(context.pool_) char[size + 1]);
    memcpy(value, value_, size_);
    memcpy(value + size_, rhs->value_, rhs->size_);
    value[size] = '\0';
    return $S(value, size);
}

CYNumber *CYString::Number(CYContext &context) {
    // XXX: there is a precise algorithm for this
    return NULL;
}

CYString *CYString::String(CYContext &context) {
    return this;
}

CYStatement *CYSwitch::Replace(CYContext &context) {
    context.Replace(value_);
    clauses_->Replace(context);
    return this;
}

CYExpression *CYThis::Replace(CYContext &context) {
    return this;
}

namespace cy {
namespace Syntax {

CYStatement *Throw::Replace(CYContext &context) {
    context.Replace(value_);
    return this;
}

} }

CYExpression *CYTrivial::Replace(CYContext &context) {
    return this;
}

CYNumber *CYTrue::Number(CYContext &context) {
    return $D(1);
}

CYString *CYTrue::String(CYContext &context) {
    return $S("true");
}

namespace cy {
namespace Syntax {

CYStatement *Try::Replace(CYContext &context) {
    code_.Replace(context);
    catch_->Replace(context);
    finally_->Replace(context);
    return this;
}

} }

CYStatement *CYVar::Replace(CYContext &context) {
    return $E(declarations_->Replace(context));
}

CYExpression *CYVariable::Replace(CYContext &context) {
    name_ = name_->Replace(context);
    return this;
}

CYStatement *CYWhile::Replace(CYContext &context) {
    context.Replace(test_);
    context.Replace(code_);
    return this;
}

CYStatement *CYWith::Replace(CYContext &context) {
    context.Replace(scope_);
    context.Replace(code_);
    return this;
}

CYExpression *CYWord::ClassName(CYContext &context, bool object) {
    CYString *name($S(this));
    if (object)
        return $C1($V("objc_getClass"), name);
    else
        return name;
}
