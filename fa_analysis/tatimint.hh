#ifndef TA_TIM_INT_H
#define TA_TIM_INT_H

#include <string>
#include <map>

#include "timbuk.hh"
#include "treeaut.hh"

using std::string;

class TAReader : public TimbukReader {

	TA<string>* dst;
	string name;

protected:

	virtual void newLabel(const string&, size_t, size_t) {}
	
	virtual void beginModel(const string& name) {
		this->dst->clear();
		this->name = name;
	}
	
	virtual void newState(const string&, size_t) {}
	
	virtual void newFinalState(size_t id) {
		this->dst->addFinalState(id);
	}
	
	virtual void endDeclaration() {}

	virtual void newTransition(const vector<size_t>& lhs, size_t label, size_t rhs) {
		this->dst->addTransition(lhs, this->getLabelName(label), rhs);
	}
	
	virtual void endModel() {}
	
public:

	TAReader(std::istream& input = std::cin, const string& name = "")
		: TimbukReader(input, name), dst(NULL) {}
	
	TA<string>& read(TA<string>& dst) {
		this->dst = &dst;
		this->readOne();
		return dst;
	}

	void readFirst(TA<string>& dst, string& name) {
		this->dst = &dst;
		TimbukReader::readFirst();
		name = this->name;
	}

	bool readNext(TA<string>& dst, string& name) {
		this->dst = &dst;
		if (!TimbukReader::readNext())
			return false;
		name = this->name;
		return true;
	}

};

class TAMultiReader : public TimbukReader {

	TA<string>::Backend& backend;

public:

	vector<TA<string> > automata;
	vector<string> names;

protected:

	virtual void newLabel(const string&, size_t, size_t) {}
	
	virtual void beginModel(const string& name) {
		this->automata.push_back(TA<string>(this->backend));
		this->names.push_back(name);
	}
	
	virtual void newState(const string&, size_t) {}
	
	virtual void newFinalState(size_t id) {
		this->automata.back().addFinalState(id);
	}
	
	virtual void endDeclaration() {}

	virtual void newTransition(const vector<size_t>& lhs, size_t label, size_t rhs) {
		this->automata.back().addTransition(lhs, this->getLabelName(label), rhs);
	}
	
	virtual void endModel() {}
	
public:

	TAMultiReader(TA<string>::Backend& backend, std::istream& input, const string& name = "")
		: TimbukReader(input, name), backend(backend) {}

	void clear() {
		this->automata.clear();
		this->names.clear();
	}

	void read() {
		this->readAll();
	}

};

class TAWriter : public TimbukWriter {

public:

	TAWriter(std::ostream& output) : TimbukWriter(output) {}

	void writeOne(const TA<string>& aut, const string& name = "TreeAutomaton") {
		map<string, size_t> labels;
		set<size_t> states;
		for (TA<string>::iterator i = aut.begin(); i != aut.end(); ++i) {
			labels.insert(make_pair(i->label(), i->lhs().size()));
			states.insert(i->rhs());
			for (size_t j = 0; j < i->lhs().size(); ++j)
				states.insert(i->lhs()[j]);
		}
		this->startAlphabet();
		for (map<string, size_t>::iterator i = labels.begin(); i != labels.end(); ++i)
			this->writeLabel(i->first, i->second);
		this->newModel(name);
		this->startStates();
		for (set<size_t>::iterator i = states.begin(); i != states.end(); ++i)
			this->writeState(*i);
		this->startFinalStates();
		for (set<size_t>::iterator i = aut.getFinalStates().begin(); i != aut.getFinalStates().end(); ++i)
			this->writeState(*i);
		this->startTransitions();
		for (TA<string>::iterator i = aut.begin(); i != aut.end(); ++i)
			this->writeTransition(i->lhs(), i->label(), i->rhs());
		this->terminate();
	}

};

#endif
