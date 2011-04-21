/*
 * Copyright (C) 2009-2010 Kamil Dudka <kdudka@redhat.com>
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

#ifndef H_GUARD_SYMUTIL_H
#define H_GUARD_SYMUTIL_H

/**
 * @file symutil.hh
 * some generic utilities working on top of a symbolic heap
 * @todo API documentation
 */

#include "config.h"

#include <cl/code_listener.h>
#include <cl/clutil.hh>

#include "symheap.hh"

#include <stack>

#include <boost/foreach.hpp>

class LocationWriter;

namespace CodeStorage {
    struct Var;
}

inline TValId boolToVal(const bool b) {
    return (b)
        ? VAL_TRUE
        : VAL_FALSE;
}

void moveKnownValueToLeft(const SymHeapCore &sh, TValId &valA, TValId &valB);

TObjId objDup(SymHeap &sh, const TObjId obj);

bool isHeapObject(const SymHeap &heap, TObjId obj);

void digRootObject(const SymHeap &heap, TValId *pValue);

inline TObjId /* root */ objRoot(const SymHeap &sh, TObjId obj) {
    TObjId root = obj;
    while (OBJ_INVALID != (obj = sh.objParent(root)))
        root = obj;

    return root;
}

inline TObjId /* root */ objRootByVal(const SymHeap &sh, TValId val) {
    if (val <= 0)
        return OBJ_INVALID;

    const TValId rootAt = sh.valRoot(val);
    return sh.pointsTo(rootAt);
}

inline TObjId /* root */ objRootByPtr(const SymHeap &sh, TObjId ptr) {
    const TValId val = sh.valueOf(ptr);
    return objRootByVal(sh, val);
}

inline bool objIsSeg(const SymHeap &sh, TObjId obj, bool anyPart = false) {
    const EObjKind kind = sh.objKind(obj);
    switch (kind) {
        case OK_CONCRETE:
            break;

        case OK_MAY_EXIST:
        case OK_SLS:
        case OK_DLS:
            return true;

        case OK_HEAD:
        case OK_PART:
            return anyPart;
    }

    return false;
}

/// return offset of an object within another object;  -1 if not found
inline TOffset subOffsetIn(const SymHeap &sh, TObjId in, TObjId of) {
    const TOffset off1 = sh.valOffset(sh.placedAt(in));
    const TOffset off2 = sh.valOffset(sh.placedAt(of));
    return off2 - off1;
}

TObjId subSeekByOffset(
        const SymHeap               &sh,
        const TObjId                root,
        const TOffset               offToSeek,
        const struct cl_type        *clt,
        const enum cl_type_e        code = CL_TYPE_UNKNOWN);

TObjId ptrObjByOffset(const SymHeap &sh, TObjId obj, TOffset off);
TObjId compObjByOffset(const SymHeap &sh, TObjId obj, TOffset off);

void getPtrValues(TValList &dst, const SymHeap &heap, TObjId obj);

void skipObj(const SymHeap &sh, TObjId *pObj, TOffset offNext);

void initVariable(SymHeap                       &sh,
                  TObjId                        obj,
                  const CodeStorage::Var        &var);

typedef std::pair<TObjId, TObjId> TObjPair;

#ifndef BUILDING_DOX
// helper template for traverseSubObjs()
template <class TItem> struct TraverseSubObjsHelper { };

// specialisation for TObjId, which means basic implementation of the traversal
template <> struct TraverseSubObjsHelper<TObjId> {
    static const struct cl_type* getItemClt(const SymHeap &sh, TObjId obj) {
        return sh.objType(obj);
    }
    static TObjId getNextItem(const SymHeap &sh, TObjId obj, int nth) {
        return sh.subObj(obj, nth);
    }
};

// specialisation suitable for traversing two composite objects simultaneously
template <> struct TraverseSubObjsHelper<TObjPair> {
    static const struct cl_type* getItemClt(const SymHeap &sh, TObjPair item) {
        const struct cl_type *clt = sh.objType(item.first);
        CL_BREAK_IF(!clt || *clt != *sh.objType(item.second));
        return clt;
    }
    static TObjPair getNextItem(const SymHeap &sh, TObjPair item, int nth) {
        item.first  = sh.subObj(item.first,  nth);
        item.second = sh.subObj(item.second, nth);
        return item;
    }
};
#endif

/// take the given visitor through all live pointers
template <class THeap, class TVisitor, typename TMethod>
bool /* complete */ traverseCore(
        THeap                       &sh,
        const TValId                rootAt,
        TVisitor                    &visitor,
        TMethod                     method)
{
    // check that we got a valid root object
    CL_BREAK_IF(sh.pointsTo(rootAt) <= 0);
    CL_BREAK_IF(sh.pointsTo(rootAt) != objRootByVal(sh, rootAt));

    TObjList objs;
    (sh.*method)(objs, rootAt);
    BOOST_FOREACH(const TObjId obj, objs) {
        if (!visitor(sh, obj))
            // traversal cancelled by visitor
            return false;
    }

    // done
    return true;
}

/// take the given visitor through all live pointers
template <class THeap, class TVisitor>
bool /* complete */ traverseLivePtrs(
        THeap                       &sh,
        const TValId                rootAt,
        TVisitor                    &visitor)
{
    return traverseCore(sh, rootAt, visitor, &SymHeap::gatherLivePointers);
}

/// take the given visitor through all live objects
template <class THeap, class TVisitor>
bool /* complete */ traverseLiveObjs(
        THeap                       &sh,
        const TValId                rootAt,
        TVisitor                    &visitor)
{
    return traverseCore(sh, rootAt, visitor, &SymHeap::gatherLiveObjects);
}

/// take the given visitor through a composite object (or whatever you pass in)
template <class THeap, class TVisitor, class TItem = TObjId>
bool /* complete */ traverseSubObjs(THeap &sh, TItem item, TVisitor &visitor,
                                    bool leavesOnly)
{
    std::stack<TItem> todo;
    todo.push(item);
    while (!todo.empty()) {
        item = todo.top();
        todo.pop();

        typedef TraverseSubObjsHelper<TItem> THelper;
        const struct cl_type *clt = THelper::getItemClt(sh, item);
        CL_BREAK_IF(!clt || !isComposite(clt));

        for (int i = 0; i < clt->item_cnt; ++i) {
            const TItem next = THelper::getNextItem(sh, item, i);

            const struct cl_type *subClt = THelper::getItemClt(sh, next);
            if (subClt && isComposite(subClt)) {
                todo.push(next);

                if (leavesOnly)
                    // do not call the visitor for internal nodes, if requested
                    continue;
            }

            if (!/* continue */visitor(sh, next))
                return false;
        }
    }

    // the traversal is done, without any interruption by visitor
    return true;
}

// only for compatibility with legacy code
#if 1
typedef std::vector<int /* nth */> TFieldIdxChain;
TObjId subObjByChain(const SymHeap &sh, TObjId obj, TFieldIdxChain ic);

#ifndef BUILDING_DOX
template <class TItem>
struct SubTraversalStackItem {
    TItem               item;
    TFieldIdxChain      ic;
};
#endif

/// take the given visitor through a composite object (or whatever you pass in)
template <class THeap, class TVisitor, class TItem = TObjId>
bool /* complete */ traverseSubObjsIc(THeap &sh, TItem item, TVisitor &visitor)
{
    typedef SubTraversalStackItem<TItem> TStackItem;
    TStackItem si;
    si.item = item;
    si.ic.push_back(0);

    std::stack<TStackItem> todo;
    todo.push(si);
    while (!todo.empty()) {
        TStackItem &si = todo.top();
        CL_BREAK_IF(si.ic.empty());

        typedef TraverseSubObjsHelper<TItem> THelper;
        const struct cl_type *clt = THelper::getItemClt(sh, si.item);
        CL_BREAK_IF(!clt || !isComposite(clt));

        typename TFieldIdxChain::reference nth = si.ic.back();
        if (nth == clt->item_cnt) {
            // done at this level
            todo.pop();
            continue;
        }

        TStackItem next = si;
        next.item = THelper::getNextItem(sh, si.item, nth);
        if (!/* continue */visitor(sh, next.item, si.ic))
            return false;

        const struct cl_type *cltNext = THelper::getItemClt(sh, next.item);
        if (!cltNext || !isComposite(cltNext)) {
            // move to the next field at this level
            ++nth;
            continue;
        }

        // nest into a sub-object
        next.ic.push_back(0);
        todo.push(next);
        ++nth;
    }

    // the traversal is done, without any interruption by visitor
    return true;
}
#endif

void gatherPointingObjects(const SymHeap            &sh,
                           TObjList                 &dst,
                           const TObjId             root,
                           bool                     toInsideOnly);

TValId addrQueryByOffset(
        SymHeap                 &sh,
        const TObjId            target,
        const TOffset           offRequested,
        const struct cl_type    *cltPtr,
        const struct cl_loc     *lw = 0);

/// (OBJ_INVALID != pointingFrom) means 'pointing from anywhere'
void redirectInboundEdges(
        SymHeap                 &sh,
        const TObjId            pointingFrom,
        const TObjId            pointingTo,
        const TObjId            redirectTo);

#endif /* H_GUARD_SYMUTIL_H */
