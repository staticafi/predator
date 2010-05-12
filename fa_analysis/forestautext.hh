#ifndef FOREST_AUT_EXT_H
#define FOREST_AUT_EXT_H

#include <vector>
#include <set>

#include <boost/unordered_map.hpp>

#include "forestaut.hh"
#include "boxman.hh"

using std::vector;
using std::set;
using std::pair;
using std::make_pair;

class FAE : public FA {

	mutable BoxMan& boxMan;

	size_t stateOffset;

	boost::unordered_map<size_t, size_t> root_reference_index;

protected:

	size_t freshState() {
		return this->stateOffset++;
	}

	bool isRootReference(size_t state, size_t& reference) {
		boost::unordered_map<size_t, size_t>::iterator i = this->root_reference_index.find(state);
		if (i == this->root_reference_index.end())
			return false;
		reference = i->second;
		return true;		
	}

	void setRootReference(size_t state, size_t reference) {
		this->root_reference_index[state] = reference;
	}
/*
	void reorderBoxes(vector<const Box*>& boxes, vector<size_t>& indeces) {
		vector<std::pair<const Box*, size_t> > tmp;
		for (size_t i = 0; i< boxes.size(); ++i)
			tmp.push_back(std::make_pair(boxes[i], i));
		std::sort(tmp.begin(), tmp.end());
		indeces.clear();
		for (size_t i = 0; i < tmp.size(); ++i) {
			boxes[i] = tmp[i].first;
			indeces.push_back(tmp[i].second);
		}
	}
*/
	FAE* mergeRoot(size_t dst, size_t src) {
		FAE* result = new FAE(this->taMan, this->labMan, this->boxMan);
		result->stateOffset = this->stateOffset;
		TA<label_type>* ta = result->taMan.alloc();
		size_t refState = (size_t)(-1);
		for (TA<label_type>::iterator i = result->roots[dst]->begin(); i != result->roots[dst]->end(); ++i) {
			if ((i->label()->size() == 1) && (*i->label())[0]->getTemplate()->isReference(src)) {
				assert(refState == (size_t)(-1));
				refState = i->rhs();
			} else {
				ta->addTransition(*i);
			}
		}
		assert(refState != (size_t)(-1));
		result->roots[src]->unfoldAtRoot(ta, refState);
		
		TA<label_type>* ta2 = result->taMan.alloc();
	}

	// try to indetify which roots to merge
	void collect() {
	}

	void reorderHeap(const vector<size_t>& variables) {
	}

public:

	FAE(TAManager<FA::label_type>& taMan, LabMan& labMan, BoxMan& boxMan)
	 : FA(taMan, labMan), boxMan(boxMan), stateOffset(1) {}

/* execution bits */
	size_t newVar() {
		size_t id = this->variables.size();
		this->variables.push_back(var_info(varUndef, 0));
		return id;
	}
	
	void dropVars(size_t count) {
		assert(count <= this->variables.size());
		this->variables.resize(this->variables.size() - count);
	}
	
	bool x_eq_y(size_t x, size_t y) {
		return this->variables[x] == this->variables[y];
	}
	
	void x_ass_new(size_t x, size_t pointerSlots, size_t dataSlots) {
		if (dataSlots > 0)
			throw std::runtime_error("Data handling not implemented! (désolé)");
		// TODO: identify garbage
		TA<label_type>* ta = this->taMan.alloc();
		this->variables[x] = var_info(this->roots.size(), 0);
		this->roots.push_back(ta);
		set<pair<const Box*, size_t> > tmp;
		for (size_t i = 0; i < pointerSlots; ++i) {
			// create fresh state
			size_t s = this->freshState();
			// create leaf rule
			vector<const Box*> label = { &this->boxMan.lookup(Box::createReference(varUndef)) };
			ta->addTransition(vector<size_t>(), &this->labMan.lookup(label) , s);
			this->setRootReference(s, varUndef);
			// prepare the rest
			tmp.insert(make_pair(&this->boxMan.lookup(Box::createPrimitive(i)), s));
		}
		vector<size_t> lhs;
		vector<const Box*> label;
		for (set<pair<const Box*, size_t> >::iterator i = tmp.begin(); i != tmp.end(); ++i) {
			lhs.push_back(i->second);
			label.push_back(i->first);
		}
		size_t f = this->freshState();
		ta->addTransition(lhs, &label, f);
		ta->addFinalState(f);		
		// TODO: reorder
	}
	
	void x_ass_null(size_t x) {
		// TODO: identify garbage
		this->variables[x] = varNull;
		// TODO: reorder
	}
	
	void x_ass_y(size_t x, size_t y) {
		// TODO: identify garbage
		this->variables[x] = this->variables[y];
		// TODO: reorder
	}
	
	void x_ass_y_next(size_t x, size_t y, size_t selector) {
		// TODO: identify garbage
		
	}	

};

#endif
