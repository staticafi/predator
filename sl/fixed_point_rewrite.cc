/*
 * Copyright (C) 2014 Kamil Dudka <kdudka@redhat.com>
 *
 * This file is part of predator.
 *
 * predator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * predator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with predator.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fixed_point_rewrite.hh"

#include <cl/cldebug.hh>
#include <cl/cl_msg.hh>
#include <cl/killer.hh>

namespace FixedPoint {

void MultiRewriter::insertInsn(TLocIdx src, TLocIdx dst, GenericInsn *insn)
{
    BOOST_FOREACH(IStateRewriter *slave, slaveList_)
        slave->insertInsn(src, dst, insn);
}

void MultiRewriter::replaceInsn(TLocIdx at, GenericInsn *insn)
{
    BOOST_FOREACH(IStateRewriter *slave, slaveList_)
        slave->replaceInsn(at, insn);
}

void MultiRewriter::dropInsn(TLocIdx at)
{
    BOOST_FOREACH(IStateRewriter *slave, slaveList_)
        slave->dropInsn(at);
}

void MultiRewriter::appendWriter(IStateRewriter &slave)
{
    slaveList_.push_back(&slave);
}

class IRewriteAction {
    public:
        virtual ~IRewriteAction() { }
        virtual void apply(IStateRewriter &writer) const = 0;
};

struct RecordRewriter::Private {
    std::vector<const IRewriteAction *> actionList;

    ~Private()
    {
        BOOST_FOREACH(const IRewriteAction *action, actionList)
            delete action;
    }
};

RecordRewriter::RecordRewriter():
    d(new Private)
{
}

RecordRewriter::~RecordRewriter()
{
    delete d;
}

class InsertInsnAction: public IRewriteAction {
    public:
        InsertInsnAction(TLocIdx src, TLocIdx dst, GenericInsn *insn):
            src_(src),
            dst_(dst),
            insn_(insn)
        {
        }

        ~InsertInsnAction()
        {
            delete insn_;
        }

        virtual void apply(IStateRewriter &writer) const
        {
            GenericInsn *insn = insn_->clone();
            writer.insertInsn(src_, dst_, insn);
        }

    private:
        TLocIdx                     src_;
        TLocIdx                     dst_;
        GenericInsn                *insn_;
};

void RecordRewriter::insertInsn(TLocIdx src, TLocIdx dst, GenericInsn *insn)
{
    const IRewriteAction *action = new InsertInsnAction(src, dst, insn);
    d->actionList.push_back(action);
}

class ReplaceInsnAction: public IRewriteAction {
    public:
        ReplaceInsnAction(TLocIdx at, GenericInsn *insn):
            at_(at),
            insn_(insn)
        {
        }

        ~ReplaceInsnAction()
        {
            delete insn_;
        }

        virtual void apply(IStateRewriter &writer) const
        {
            GenericInsn *insn = insn_->clone();
            writer.replaceInsn(at_, insn);
        }

    private:
        TLocIdx                     at_;
        GenericInsn                *insn_;
};

void RecordRewriter::replaceInsn(TLocIdx at, GenericInsn *insn)
{
    const IRewriteAction *action = new ReplaceInsnAction(at, insn);
    d->actionList.push_back(action);
}

class DropInsnAction: public IRewriteAction {
    public:
        DropInsnAction(TLocIdx at):
            at_(at)
        {
        }

        virtual void apply(IStateRewriter &writer) const
        {
            writer.dropInsn(at_);
        }

    private:
        TLocIdx                     at_;
};

void RecordRewriter::dropInsn(TLocIdx at)
{
    const IRewriteAction *action = new DropInsnAction(at);
    d->actionList.push_back(action);
}

/// UNSAFE wrt. exceptions falling through IRewriteAction::apply(...)
void RecordRewriter::flush(IStateRewriter *pConsumer)
{
    BOOST_FOREACH(const IRewriteAction *action, d->actionList) {
        action->apply(*pConsumer);
        delete action;
    }

    d->actionList.clear();
}

bool RecordRewriter::empty() const
{
    return d->actionList.empty();
}

void StateRewriter::insertInsn(
        const TLocIdx               src,
        const TLocIdx               dst,
        GenericInsn                *insn)
{
    CL_NOTE("[ADT] inserting " << *insn
            << " between locations #" << src << " -> #" << dst);

    // allocate a new instruction
    LocalState *locState = new LocalState;
    locState->insn = insn;

    // append the instruction to the list
    const TLocIdx at = state_.size();
    state_.stateList_.append(locState);

    // resolve src/dst instructions
    LocalState &srcState = state_[src];
    LocalState &dstState = state_[dst];
    if (!srcState.insn || !dstState.insn)
        CL_BREAK_IF("invalid neighbour detected in insertInsn()");

    bool closesLoop = false;
    BOOST_FOREACH(CfgEdge &oe, srcState.cfgOutEdges) {
        if (dst != oe.targetLoc)
            continue;

        closesLoop = oe.closesLoop;
        oe.targetLoc = at;
        oe.closesLoop = false;
    }

    BOOST_FOREACH(CfgEdge &ie, dstState.cfgInEdges) {
        if (src != ie.targetLoc)
            continue;

        CL_BREAK_IF(closesLoop != ie.closesLoop);
        ie.targetLoc = at;
    }

    const CfgEdge ie(src);
    locState->cfgInEdges.push_back(ie);

    const CfgEdge oe(dst, closesLoop);
    locState->cfgOutEdges.push_back(oe);
}

void StateRewriter::replaceInsn(const TLocIdx at, GenericInsn *insn)
{
    CL_NOTE("[ADT] replacing insn #" << at << " by " << *insn);
    LocalState &locState = state_[at];
    delete locState.insn;
    locState.insn = insn;
}

void StateRewriter::dropInsn(const TLocIdx at)
{
    CL_NOTE("[ADT] removing insn #" << at);
    LocalState &locState = state_[at];
    delete locState.insn;
    locState.insn = 0;

    // iterate through all incoming edges
    BOOST_FOREACH(const CfgEdge &ie, locState.cfgInEdges) {
        LocalState &inState = state_[ie.targetLoc];
        TCfgEdgeList outEdges;

        BOOST_FOREACH(const CfgEdge &be, inState.cfgOutEdges) {
            if (at != be.targetLoc) {
                // keep unrelated CFG edges as they are
                outEdges.push_back(be);
                continue;
            }

            // redirect all edges previously going to 'at'
            BOOST_FOREACH(CfgEdge oe, locState.cfgOutEdges) {
                oe.closesLoop |= ie.closesLoop;
                outEdges.push_back(oe);
            }
        }

        outEdges.swap(inState.cfgOutEdges);
    }

    // iterate through all outgoing edges
    BOOST_FOREACH(const CfgEdge &oe, locState.cfgOutEdges) {
        LocalState &outState = state_[oe.targetLoc];
        TCfgEdgeList inEdges;

        BOOST_FOREACH(const CfgEdge &be, outState.cfgInEdges) {
            if (at != be.targetLoc) {
                // keep unrelated CFG edges as they are
                inEdges.push_back(be);
                continue;
            }

            // redirect all edges previously coming from 'at'
            BOOST_FOREACH(CfgEdge ie, locState.cfgInEdges) {
                ie.closesLoop |= oe.closesLoop;
                inEdges.push_back(ie);
            }
        }

        inEdges.swap(outState.cfgInEdges);
    }

    // finally detach 'at' from the graph completely
    locState.cfgInEdges.clear();
    locState.cfgOutEdges.clear();
}

bool StateRewriter::dedupOutgoingEdges(const TLocIdx at)
{
    LocalState &locState = state_[at];
    bool anyChange = false;

    // iterate through all outgoing edges
    BOOST_FOREACH(const CfgEdge &oe, locState.cfgOutEdges) {
        LocalState &outState = state_[oe.targetLoc];
        TCfgEdgeList inEdges;
        std::set<TLocIdx> inSet;

        BOOST_FOREACH(const CfgEdge &be, outState.cfgInEdges) {
            const TLocIdx dst = be.targetLoc;
            if (dst == at && !insertOnce(inSet, dst)) {
                // duplicate edge detected
                anyChange = true;
                continue;
            }

            // keep other CFG edges as they are
            inEdges.push_back(be);
        }

        inEdges.swap(outState.cfgInEdges);
    }

    if (!anyChange)
        // nothing changed actually
        return false;

    TCfgEdgeList outEdges;
    std::set<TLocIdx> outSet;

    // iterate through all outgoing edges
    BOOST_FOREACH(const CfgEdge &oe, locState.cfgOutEdges) {
        if (insertOnce(outSet, oe.targetLoc))
            outEdges.push_back(oe);
    }

    outEdges.swap(locState.cfgOutEdges);
    return true;
}

void ClInsn::lazyInit() const
{
    using namespace CodeStorage::VarKiller;

    if (done_)
        return;
    done_ = true;

    BlockData data;
    scanInsn(&data, insn_);

    BOOST_FOREACH(const TVar var, data.gen)
        live_.insert(GenericVar(VL_CODE_LISTENER, var));

    BOOST_FOREACH(const TVar var, data.kill)
        kill_.insert(GenericVar(VL_CODE_LISTENER, var));
}

const TGenericVarSet& ClInsn::liveVars() const
{
    this->lazyInit();
    return live_;
}

const TGenericVarSet& ClInsn::killVars() const
{
    this->lazyInit();
    return kill_;
}

void ClInsn::writeToStream(std::ostream &str) const
{
    str << *insn_;
}

void TextInsn::writeToStream(std::ostream &str) const
{
    str << text_;
}

} // namespace FixedPoint
